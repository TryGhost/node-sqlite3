ARG NODE_VERSION=16
ARG VARIANT=bullseye

FROM node:$NODE_VERSION-$VARIANT

RUN if [ "$VARIANT" = "alpine" ] ; then apk add make g++ python ; fi

WORKDIR /usr/src/build

COPY . .
RUN npm install --ignore-scripts
RUN npx node-pre-gyp configure
RUN npx node-pre-gyp build
RUN npm run test
RUN npx node-pre-gyp package

CMD ["sh"]
