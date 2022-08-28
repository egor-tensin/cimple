FROM alpine:3.16 AS base

ARG install_dir="/app/install"

FROM base AS builder

RUN apk add --no-cache bsd-compat-headers build-base clang cmake libgit2-dev

ARG src_dir="/app/src"
ARG build_dir="/app/build"
ARG install_dir

COPY [".", "$src_dir"]

RUN mkdir -- "$build_dir" && \
    cd -- "$build_dir" && \
    cmake \
         -D CMAKE_C_COMPILER=clang \
         -D CMAKE_BUILD_TYPE=Release \
         -D "CMAKE_INSTALL_PREFIX=$install_dir" \
         "$src_dir" && \
    make -j install

FROM base

LABEL maintainer="Egor Tensin <Egor.Tensin@gmail.com>"

RUN apk add --no-cache tini libgit2

ARG install_dir
COPY --from=builder ["$install_dir", "$install_dir"]

WORKDIR "$install_dir/bin"

ENTRYPOINT ["/sbin/tini", "--"]
CMD ["./cimple-server"]
