#!/bin/echo docker build . -f
# -*- coding: utf-8 -*-
#{
# ISC License
# Copyright (c) 2004-2010 by Internet Systems Consortium, Inc. ("ISC")
# Copyright (c) 1995-2003 by Internet Software Consortium
# Permission to use, copy, modify, and /or distribute this software
# for any purpose with or without fee is hereby granted,
# provided that the above copyright notice
#  and this permission notice appear in all copies.
# THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS.
# IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
# OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
# WHETHER IN AN ACTION OF CONTRACT,
# NEGLIGENCE OR OTHER TORTIOUS ACTION,
# ARISING OUT OF OR IN CONNECTION WITH THE USE
# OR PERFORMANCE OF THIS SOFTWARE.
#}

FROM ubuntu:latest
MAINTAINER Philippe Coval (p.coval@samsung.com)

ENV DEBIAN_FRONTEND noninteractive
ENV LC_ALL en_US.UTF-8
ENV LANG ${LC_ALL}

RUN echo "#log: Configuring locales" \
  && set -x \
  && apt-get update -y \
  && apt-get install -y locales \
  && echo "${LC_ALL} UTF-8" | tee /etc/locale.gen \
  && locale-gen ${LC_ALL} \
  && dpkg-reconfigure locales \
  && sync

ENV project node-sqlite3

RUN echo "#log: ${project}: Setup system" \
  && set -x \
  && apt-get update -y \
  && apt-get install -y \
  curl \
  sudo \
  build-essential \
  python \
  && apt-get clean \
  && NVM_VERSION="v0.33.8" \
  && NODE_VERSION="--lts=carbon" \
  && curl -o- https://raw.githubusercontent.com/creationix/nvm/${NVM_VERSION}/install.sh | bash \
  && which nvm || . ~/.bashrc \
  && nvm install ${NODE_VERSION} \
  && nvm use ${NODE_VERSION} \
  && sync

ADD . /usr/local/${project}/${project}
WORKDIR /usr/local/${project}/${project}
RUN echo "#log: ${project}: Preparing sources" \
  && set -x \
  && which npm || . ~/.bashrc \
  && npm install || cat npm-debug.log \
  && npm install \
  && npm install --unsafe-perm --build-from-source \
  && sync

WORKDIR /usr/local/${project}/${project}
RUN echo "#log: ${project}: Building sources" \
  && set -x \
  && which npm || . ~/.bashrc \
  && npm run pack \
  && npm pack \
  && find build/stage/ -type f \
  && sync

