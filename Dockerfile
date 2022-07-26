FROM ubuntu:20.04

USER root

# Install all apt-packages needed for the project
ARG DEBIAN_FRONTEND=noninteractive
#ENV TZ=Europe
RUN apt-get update
RUN apt-get -y install \
    git \
    gcc \
    cmake \
    build-essential \
    autoconf \
    libtool \
    pkg-config

# Cloning gRPC
ENV MY_INSTALL_DIR=$HOME/.local
RUN mkdir -p $MY_INSTALL_DIR
RUN export PATH="$MY_INSTALL_DIR/bin:$PATH"
RUN git clone --recurse-submodules -b v1.48.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc

# Building gRPC from source
WORKDIR /grpc/cmake/build
RUN cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR \
      ../..
RUN make -j2
RUN make install
RUN cd ../../..

# Create and populate Hydro project context
RUN mkdir /hydro
ENV HYDRO_HOME /hydro
WORKDIR /hydro