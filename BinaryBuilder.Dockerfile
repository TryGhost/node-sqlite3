ARG NODE_VERSION=12
ARG VARIANT=stretch
FROM node:$NODE_VERSION-$VARIANT

ARG VARIANT
ARG AWS_ACCESS_KEY_ID=SKIP
ARG AWS_SECRET_ACCESS_KEY=SKIP

RUN if [ "$VARIANT" = "alpine" ] ; then apk add make g++ python ; fi

WORKDIR /usr/src/build

COPY . .
RUN npm install --ignore-scripts
RUN npx node-pre-gyp install --build-from-source
RUN npm run test
RUN npx node-pre-gyp package
RUN if [ "$AWS_ACCESS_KEY_ID" = "SKIP" ] || [ "$AWS_SECRET_ACCESS_KEY" = "SKIP" ] ; then echo "SKIP S3 PUBLISH" ; else npx node-pre-gyp publish ; fi

CMD ["sh"]
