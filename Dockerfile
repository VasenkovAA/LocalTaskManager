FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        # Существующие пакеты
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
        ca-certificates \
        # Qt6 базовые пакеты для Ubuntu 22.04
        qt6-base-dev \
        qt6-base-private-dev \
        qt6-tools-dev \
        qt6-tools-dev-tools \
        # Qt6 дополнительные модули
        libqt6svg6-dev \
        qtbase5-dev \
        qtbase5-dev-tools \
        # Для сборки и запуска
        make \
        # Для GUI приложений (если нужно запускать в контейнере)
        libgl1-mesa-dev \
        libx11-dev \
        libxext-dev \
        libxcb1-dev \
        libxcb-xinerama0-dev \
        # Для CMake (чтобы находил Qt)
        cmake-data \
        && \
    rm -rf /var/lib/apt/lists/*

# Устанавливаем переменные окружения для Qt
ENV QT_QPA_PLATFORM=offscreen
ENV QT_DEBUG_PLUGINS=0

WORKDIR /app

CMD ["/bin/bash"]