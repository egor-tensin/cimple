FROM alpine:3.19 AS base

ARG install_dir="/app/install"

FROM base AS builder

RUN build_deps='bash bsd-compat-headers build-base clang cmake coreutils git json-c-dev libgit2-dev libsodium-dev py3-pytest sqlite-dev valgrind' && \
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

LABEL maintainer="Egor Tensin <egor@tensin.name>"

RUN runtime_deps='json-c libgit2 libsodium sqlite tini' && \
    apk add -q --no-cache $runtime_deps

ARG install_dir
COPY --from=builder ["$install_dir", "$install_dir"]

ENV PATH="$install_dir/bin:${PATH}"
WORKDIR "$install_dir/bin"

ENTRYPOINT ["/sbin/tini", "--"]
CMD ["cimple-server"]
