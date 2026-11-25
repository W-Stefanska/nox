FROM ubuntu:24.04 AS build
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential cmake ninja-build \
    qt6-base-dev qt6-tools-dev libgl1-mesa-dev \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /src
COPY . .
RUN cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build

FROM ubuntu:24.04
RUN apt-get update && apt-get install -y --no-install-recommends \
    libqt6widgets6 libqt6gui6 libqt6core6 libqt6svg6 \
    libxcb-cursor0 libxkbcommon-x11-0 libgl1 libegl1 \
    && rm -rf /var/lib/apt/lists/*
COPY --from=build /src/build/untitled /usr/local/bin/app
ENV QT_QPA_PLATFORM=xcb
CMD ["app"]