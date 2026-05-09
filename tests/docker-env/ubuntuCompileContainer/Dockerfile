FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

ARG KERNEL_VERSION

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc-12 \
    g++-12 \
    kmod \
    bc \
    flex \
    bison \
    libssl-dev \
    libelf-dev \
    linux-headers-${KERNEL_VERSION} \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

CMD ["/bin/bash"]