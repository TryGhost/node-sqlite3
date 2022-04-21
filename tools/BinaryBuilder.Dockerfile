ARG NODE_VERSION=16
ARG VARIANT=bullseye

FROM node:$NODE_VERSION-$VARIANT

ARG VARIANT

RUN if [ "$VARIANT" = "alpine" ] ; then apk add build-base python3 --update-cache ; fi

WORKDIR /usr/src/build

COPY . .
RUN npm install --ignore-scripts

# Workaround for https://github.com/mapbox/node-pre-gyp/issues/644
RUN cd node_modules/\@mapbox/node-pre-gyp \
  && npm install fs-extra@10.0.1 \
  && sed -i -e s/\'fs/\'fs-extra/ -e s/fs\.renameSync/fs.moveSync/ ./lib/util/napi.js

ENV CFLAGS="${CFLAGS:-} -include ../src/gcc-preinclude.h"
ENV CXXFLAGS="${CXXFLAGS:-} -include ../src/gcc-preinclude.h"
RUN npx node-pre-gyp configure
RUN npx node-pre-gyp build

RUN if [ "$VARIANT" != "alpine" ] ; then ldd lib/binding/*/node_sqlite3.node; nm lib/binding/*/node_sqlite3.node | grep "GLIBC_" | c++filt || true ; fi

RUN npm run test
RUN npx node-pre-gyp package

CMD ["sh"]
