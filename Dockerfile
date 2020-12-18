###
#   SimulationCraft docker image
#
#   Available build-arg:
#     - THREADS=[int] Default 1, provide a value for -j
#     - NONETWORKING=[0|1] Default 0, 0 - armory can be used if an api-key is provided, 1 - no import capabilities
#     - APIKEY=[str] Default '' (empty)
#
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
ARG NONETWORKING=0
ARG APIKEY=''

COPY . /app/SimulationCraft/

# install SimulationCraft
RUN apk --no-cache add --virtual build_dependencies \
    curl-dev \
    g++ \
    git \
    libcurl \
    make && \
    if [ $NONETWORKING -eq 0 ] ; then \
        echo "Building networking version" && \
        make -C /app/SimulationCraft/engine optimized -j $THREADS LTO_THIN=1 OPTS+="-Os -mtune=native" SC_DEFAULT_APIKEY=${APIKEY} ;  \
    else \
        echo "Building no-networking version" && \
        make -C /app/SimulationCraft/engine optimized -j $THREADS LTO_THIN=1 SC_NO_NETWORKING=1 OPTS+="-Os -mtune=native" SC_DEFAULT_APIKEY=${APIKEY} ; \
    fi && \
    apk del build_dependencies

# disable ptr to reduce build size
# sed -i '' -e 's/#define SC_USE_PTR 1/#define SC_USE_PTR 0/g' engine/dbc.hpp

# fresh image to reduce size
FROM alpine:latest

ARG NONETWORKING=0

RUN if [ $NONETWORKING -eq 0 ] ; then \
        echo "Preparing for networking" && \
        apk --no-cache add --virtual build_dependencies \
        libcurl \
        libgcc \
        libstdc++ ; \
    else \
        echo "Preparing for no-networking" && \
        apk --no-cache add --virtual build_dependencies \
        libgcc \
        libstdc++ ; \
    fi

# get compiled simc and profiles
COPY --from=build /app/SimulationCraft/engine/simc /app/SimulationCraft/
COPY --from=build /app/SimulationCraft/profiles/ /app/SimulationCraft/profiles/

WORKDIR /app/SimulationCraft
