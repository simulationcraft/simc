###
#   SimulationCraft docker image
#
#   Example usage:
#   - creating the image (note the dot!)
#   docker build --build-arg THREADS=2 -t simulationcraft .
#                                    ^ your intended thread count to optimize simc for
#                                         ^ name of the image
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

COPY . /app/SimulationCraft/

# install SimulationCraft
RUN apk --no-cache add --virtual build_dependencies \
    curl-dev \
    g++ \
    git \
    libcurl \
    make && \
    make -C /app/SimulationCraft/engine optimized -j $THREADS && \
    apk del build_dependencies

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
