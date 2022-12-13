###
#   SimulationCraft docker image
#
#   Available build-arg:
#     - THREADS=[int] Default 1, provide a value for -j
#     - APIKEY=[str] Default "" (empty) SC_DEFAULT_APIKEY used for authentication with blizzard api (armory)
#
##
#   Example usage:
#   - creating the image
#   docker build --build-arg THREADS=2 --build-arg NONETWORKING=1 -t simulationcraft .
#                                    ^ your intended thread count to optimize simc for
#
#   - run the image
#   docker run simulationcraft spell_query=spell.name=frost_shock
#                              ^ start of the command
#
#   To reduce the footprint of this image all SimulationCraft files are
#   removed except:
#   - ./simc
#   - ./profiles/*
#

# build image
FROM alpine:latest AS build

ARG THREADS=1
ARG APIKEY=""
ARG PGO_PROFILE="CI.simc"

COPY . /app/SimulationCraft/

WORKDIR /app/SimulationCraft/

RUN \
    # install Dependencies
    echo "Installing dependencies" && \
    apk --no-cache add --virtual build_dependencies \
        compiler-rt \
        curl-dev \
        clang-dev \
        llvm \
        g++ \
        make \
        git && \
    # Build
    echo "Building simc executable" && \
    clang++ -v && \
    make -C /app/SimulationCraft/engine release CXX=clang++ -j ${THREADS} THIN_LTO=1 LLVM_PGO_GENERATE=1 OPTS+="-Os -mtune=generic" SC_DEFAULT_APIKEY=${APIKEY} && \
    # Collect profile guided instrumentation data
    echo "Collecting profile guided instrumentation data" && \
    LLVM_PROFILE_FILE="code-%p.profraw" ./engine/simc ${PGO_PROFILE} single_actor_batch=1 iterations=100 && \
    # Merge profile guided data
    llvm-profdata merge -output=./engine/code.profdata code-*.profraw && \
    # Clean & rebuild with collected profile guided data.
    echo "Rebuilding simc executable with profile data for further optimization" && \
    make -C /app/SimulationCraft/engine clean && \
    make -C /app/SimulationCraft/engine release CXX=clang++ -j ${THREADS} THIN_LTO=1 LLVM_PGO_USE=./code.profdata OPTS+="-Os -mtune=generic" SC_DEFAULT_APIKEY=${APIKEY} && \
    # Cleanup dependencies
    echo "Cleaning up" && \
    apk del build_dependencies

# disable ptr to reduce build size
# sed -i '' -e 's/#define SC_USE_PTR 1/#define SC_USE_PTR 0/g' engine/dbc.hpp

# fresh image to reduce size
FROM alpine:latest

LABEL repository="https://github.com/simulationcraft/simc"

RUN \
    echo "Installing minimal dependencies" && \
    apk --no-cache add --virtual build_dependencies \
        libcurl \
        libgcc \
        libstdc++

# get compiled simc and profiles
COPY --from=build /app/SimulationCraft/engine/simc /app/SimulationCraft/
COPY --from=build /app/SimulationCraft/profiles/ /app/SimulationCraft/profiles/

WORKDIR /app/SimulationCraft

ENTRYPOINT [ "./simc" ]

CMD [ "T29_Raid.simc" ]
