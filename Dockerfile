FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        build-essential \
        g++ \
        g++-10 \
        cmake \
        ninja-build \
        pkg-config \
        git \
        gdb \
        valgrind \
        clang-format \
        sqlite3 \
        libsqlite3-dev \
        odb \
        libodb-dev \
        libodb-sqlite-dev \
        ca-certificates && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /app

CMD ["/bin/bash"]