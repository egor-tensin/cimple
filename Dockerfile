FROM alpine:3.16 AS base

ARG install_dir="/app/install"

FROM base AS builder

RUN apk add --no-cache bash bsd-compat-headers build-base clang cmake libgit2-dev sqlite-dev

ARG C_COMPILER=clang
ARG BUILD_TYPE=Release
ARG DEFAULT_HOST=127.0.0.1

ARG src_dir="/app/src"
ARG install_dir

COPY [".", "$src_dir"]

RUN cd -- "$src_dir" && \
    make install \
        "C_COMPILER=$C_COMPILER" \
        "BUILD_TYPE=$BUILD_TYPE" \
        "DEFAULT_HOST=$DEFAULT_HOST" \
        "INSTALL_PREFIX=$install_dir"

FROM base

LABEL maintainer="Egor Tensin <Egor.Tensin@gmail.com>"

RUN apk add --no-cache tini libgit2 sqlite-dev

ARG install_dir
COPY --from=builder ["$install_dir", "$install_dir"]

ENV PATH="$install_dir/bin:${PATH}"
WORKDIR "$install_dir/bin"

ENTRYPOINT ["/sbin/tini", "--"]
CMD ["cimple-server", "--sqlite", "/var/lib/cimple/cimple.sqlite"]
