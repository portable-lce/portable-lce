FROM ubuntu:24.04 AS clang

ENV DEBIAN_FRONTEND=noninteractive
ARG LLVM_VERSION=20

RUN apt-get update \
    && apt-get install -y \
        wget \
        lsb-release \
        software-properties-common \
        gnupg

RUN wget https://apt.llvm.org/llvm.sh \
    && chmod +x llvm.sh \
    && ./llvm.sh ${LLVM_VERSION} \
    && rm llvm.sh

RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_VERSION} 100 \
    --slave /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_VERSION}

RUN update-alternatives --install /usr/bin/lld lld /usr/bin/lld-${LLVM_VERSION} 100

RUN apt-get install -y \
        libc++-${LLVM_VERSION}-dev \
        libc++abi-${LLVM_VERSION}-dev \
    && rm -rf /var/lib/apt/lists/*

ENV CC="clang" \
    CXX="clang++" \
    CC_LD="lld" \
    CXX_LD="lld"

FROM clang AS portablelce

ARG MESON_VERSION=1.10.2

RUN apt-get update \
    && apt-get install -y software-properties-common \
    && add-apt-repository ppa:ubuntu-toolchain-r/test \
    && apt-get update \
    && apt-get install -y \
        build-essential \
        ninja-build \
        libsdl2-dev \
        libgl-dev \
        libglm-dev \
        libglu1-mesa-dev \
        libpthread-stubs0-dev \
        libssl-dev \
        gcc-15 g++-15 \
    && update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-15 100 \
    && update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-15 100 \
    && apt-get autoremove -y \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

RUN wget -qO meson.tar.gz https://github.com/mesonbuild/meson/releases/latest/download/meson-${MESON_VERSION}.tar.gz \
    && mkdir /opt/meson \
    && tar xf meson.tar.gz --strip-components=1 -C /opt/meson \
    && mv /opt/meson/meson.py /opt/meson/meson \
    && ln -s /opt/meson/meson /usr/bin/meson \
    && rm -rf meson.tar.gz
