FROM ubuntu:20.04

USER root

# Install all apt-packages needed for the project   
ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update
RUN apt-get -y install \
    wget \
    git \
    gcc \
    build-essential \
    autoconf \
    libtool \
    pkg-config \
    net-tools \
    libyaml-cpp-dev \
    ntp \
    iproute2

ENV MY_INSTALL_DIR=$HOME/.local
RUN mkdir -p $MY_INSTALL_DIR
ENV PATH="$MY_INSTALL_DIR/bin:$PATH"

# Install CMake
RUN wget -q -O cmake-linux.sh https://github.com/Kitware/CMake/releases/download/v3.22.5/cmake-3.22.5-linux-x86_64.sh
RUN chmod +x cmake-linux.sh
RUN ./cmake-linux.sh -- --skip-license --prefix=$MY_INSTALL_DIR
RUN rm cmake-linux.sh

# Clone and build gRPC
RUN git clone --recurse-submodules -b v1.48.0 --depth 1 --shallow-submodules https://github.com/grpc/grpc
WORKDIR /grpc/cmake/build
RUN cmake -DgRPC_INSTALL=ON \
      -DgRPC_BUILD_TESTS=OFF \
      -DCMAKE_INSTALL_PREFIX=$MY_INSTALL_DIR \
      ../..
RUN make -j2
RUN make install
WORKDIR /

# Install hiredis
RUN git clone https://github.com/redis/hiredis.git
WORKDIR hiredis
RUN make
RUN make install
WORKDIR /

# Install redis-plus-plus
RUN git clone https://github.com/sewenew/redis-plus-plus.git
WORKDIR redis-plus-plus
RUN mkdir build
WORKDIR build
RUN cmake ..
RUN make
RUN make install
WORKDIR /

# Create and populate Hydro project context
RUN mkdir /hydro
ENV HYDRO_HOME /hydro
WORKDIR /hydro