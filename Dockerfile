###
#   SimulationCraft docker image
#
#   Available build-arg:
#     - THREADS=[int] Default 1, provide a value for -j
#     - APIKEY=[str] Default '' (empty) SC_DEFAULT_APIKEY used for authentication with blizzard api (armory)
##
#   Example usage:
#   - creating the image (note the dot!)
#   docker build --build-arg THREADS=2 --build-arg NONETWORKING=1 -t simulationcraft .
#                                    ^ your intended thread count to optimize simc for
#                                                                    ^ name of the image
#   - run the image
#   docker run simulationcraft ./simc spell_query=spell.name=frost_shock
#                              ^ start of the command
#
#   To reduce the footprint of this image all SimulationCraft files are
#   removed except for the following files and directories (including
#   their files)
#   - ./simc
#   - ./profiles/*
#

# base image
FROM alpine:latest AS build

ARG THREADS=1
ARG APIKEY=''

COPY . /app/SimulationCraft/

# install Dependencies
RUN apk --no-cache add --virtual build_dependencies \
            compiler-rt-static \
            curl-dev \
            clang-dev \
            llvm \
            g++ \
            make \
            git

# Build
RUN clang++ -v && make -C /app/SimulationCraft/engine release CXX=clang++ -j $THREADS THIN_LTO=1 LLVM_PGO_GENERATE=1 OPTS+="-Os -mtune=generic" SC_DEFAULT_APIKEY={$APIKEY}


# Collect profile guided instrumentation data
RUN cd /app/SimulationCraft/engine && LLVM_PROFILE_FILE="code-%p.profraw" ./simc T26_Raid.simc single_actor_batch=1 iterations=100

# Merge profile guided data
RUN cd /app/SimulationCraft/engine && llvm-profdata merge -output=code.profdata code-*.profraw

# Clean & rebuild with collected profile guided data.
RUN make -C /app/SimulationCraft/engine clean && make -C /app/SimulationCraft/engine release CXX=clang++ -j $THREADS THIN_LTO=1 LLVM_PGO_USE=./code.profdata OPTS+="-Os -mtune=generic" SC_DEFAULT_APIKEY={$APIKEY}

# Cleanup dependencies
RUN apk del build_dependencies

# disable ptr to reduce build size
# sed -i '' -e 's/#define SC_USE_PTR 1/#define SC_USE_PTR 0/g' engine/dbc.hpp

# fresh image to reduce size
FROM alpine:latest

RUN apk --no-cache add --virtual build_dependencies \
        libcurl \
        libgcc \
        libstdc++

# get compiled simc and profiles
COPY --from=build /app/SimulationCraft/engine/simc /app/SimulationCraft/
COPY --from=build /app/SimulationCraft/profiles/ /app/SimulationCraft/profiles/

WORKDIR /app/SimulationCraft
