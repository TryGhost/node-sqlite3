ARG NODE_VERSION=18
ARG VARIANT=bullseye

FROM node:$NODE_VERSION-$VARIANT

ARG VARIANT

RUN if case $VARIANT in "alpine"*) true;; *) false;; esac; then apk add build-base python3 --update-cache ; fi

WORKDIR /usr/src/build

COPY . .
RUN npm install --ignore-scripts

ENV CFLAGS="${CFLAGS:-} -include ../src/gcc-preinclude.h"
ENV CXXFLAGS="${CXXFLAGS:-} -include ../src/gcc-preinclude.h"
RUN npm run prebuild

RUN if case $VARIANT in "alpine"*) false;; *) true;; esac; then ldd build/**/node_sqlite3.node; nm build/**/node_sqlite3.node | grep \"GLIBC_\" | c++filt || true ; fi

RUN npm run test

CMD ["sh"]
