FROM alpine:3.18 AS base

ARG install_dir="/app/install"

FROM base AS builder

RUN build_deps='bash bsd-compat-headers build-base clang cmake coreutils git libgit2-dev libsodium-dev py3-pytest sqlite-dev valgrind' && \
    apk add -q --no-cache $build_deps

ARG COMPILER=clang
ARG CONFIGURATION=Release
ARG DEFAULT_HOST=127.0.0.1
ARG DEFAULT_PORT=5556

ARG src_dir="/app/src"
ARG install_dir

COPY [".", "$src_dir"]

RUN cd -- "$src_dir" && \
    make install \
        "COMPILER=$COMPILER" \
        "CONFIGURATION=$CONFIGURATION" \
        "DEFAULT_HOST=$DEFAULT_HOST" \
        "DEFAULT_PORT=$DEFAULT_PORT" \
        "INSTALL_PREFIX=$install_dir" && \
    ulimit -n 1024 && \
    make test/docker

FROM base

LABEL maintainer="Egor Tensin <Egor.Tensin@gmail.com>"

RUN runtime_deps='tini libgit2 libsodium sqlite-dev' && \
    apk add -q --no-cache $runtime_deps

ARG install_dir
COPY --from=builder ["$install_dir", "$install_dir"]

ENV PATH="$install_dir/bin:${PATH}"
WORKDIR "$install_dir/bin"

ENTRYPOINT ["/sbin/tini", "--"]
CMD ["cimple-server"]
