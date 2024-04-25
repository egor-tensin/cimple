FROM alpine:3.19 AS base

FROM base AS builder

RUN build_deps='bash bsd-compat-headers build-base clang cmake coreutils git json-c-dev libgit2-dev libsodium-dev py3-pytest sqlite-dev valgrind' && \
    apk add -q --no-cache $build_deps

ARG COMPILER=clang
ARG DEFAULT_HOST=127.0.0.1
ARG DEFAULT_PORT=5556

COPY [".", "/app"]

RUN cd -- "/app" && \
    make release/install \
        "COMPILER=$COMPILER" \
        "DEFAULT_HOST=$DEFAULT_HOST" \
        "DEFAULT_PORT=$DEFAULT_PORT" && \
    ulimit -n 1024 && \
    make release/sanity

FROM base

LABEL maintainer="Egor Tensin <egor@tensin.name>"

RUN runtime_deps='json-c libgit2 libsodium sqlite tini' && \
    apk add -q --no-cache $runtime_deps

COPY --from=builder ["/app/build/release/install", "/app"]

ENV PATH="/app/bin:${PATH}"
WORKDIR "/app/bin"

ENTRYPOINT ["/sbin/tini", "--"]
CMD ["cimple-server"]
