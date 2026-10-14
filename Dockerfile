
FROM gcc:14-bookworm AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    default-libmysqlclient-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

RUN mkdir -p build && \
    g++ -std=c++17 -O2 -Iinclude \
    src/DatabaseManager.cpp \
    src/Models.cpp \
    src/RouteOptimizer.cpp \
    src/UI_Views.cpp \
    src/main.cpp \
    -o build/routemaster \
    $(pkg-config --cflags --libs mysqlclient)

FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    libmariadb3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/routemaster /app/routemaster

CMD ["./routemaster"]
