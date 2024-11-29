# syntax=docker/dockerfile:1.3

# Build c++ application statically
FROM scratch AS build_stage
ADD alpine-minirootfs-3.20.3-aarch64.tar.gz /

ARG VERSION
LABEL org.opencontainers.image.authors="author"
LABEL org.opencontainers.image.version="$VERSION"

# create and enter working directory
RUN mkdir /app
WORKDIR /app

# install dependencies
RUN apk add boost-dev boost-static g++ musl-dev

ADD src/server.cpp .

# compile server application
RUN g++ -o srv -std=c++20 -lboost_system -static server.cpp

# Build curl for healthcheck
FROM alpine AS build_curl

RUN apk add --no-cache curl

# Run application in a minimal layer
FROM gcr.io/distroless/static AS run_stage

# copy server binary file
COPY --from=build_stage /app/srv ./server_app
COPY --from=build_curl /usr/bin/curl /curl

EXPOSE 80

HEALTHCHECK --interval=5s --timeout=3s --start-period=5s --retries=3 \
	CMD ["/curl", "-f", "0.0.0.0:80/"] || exit 1

# run server
ENTRYPOINT ["./server_app"]
CMD ["0.0.0.0", "80"]
