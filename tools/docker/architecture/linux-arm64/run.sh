#!/bin/sh
# -*- coding: utf-8 -*-
# SPDX-License-Identifier: ISC
# Copyright 2019-present Samsung Electronics Co., Ltd. and other contributors
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

set -e
set -x

this_dir=$(dirname -- "$0")
this_dir=$(realpath "${this_dir}")
this_name=$(basename -- "$0")
top_dir="${this_dir}/../../.."

module_name="sqlite3"
project="node-${module_name}"
arch="aarch64" # AKA: arm64, arm64v8 
architecture=$(basename "${this_dir}")
name="${project}-${architecture}"
dir="/usr/local/opt/${project}/"
dist_dir="${dir}/src/${project}/build"
tag=$(git describe --tags || echo v0.0.0)
version=$(echo "${tag}" | cut -dv -f2 | cut -d'-' -f1)

mkdir -p "${this_dir}/local" "${this_dir}/tmp"
cp -a "/usr/bin/qemu-${arch}-static" "${this_dir}/local"
time docker build -t "${name}" -f "${this_dir}/Dockerfile" .
container=$(docker create "${name}")
mkdir -p "${this_dir}/tmp/${dist_dir}"
rm -rf "${this_dir}/tmp/${dist_dir}"
docker cp "${container}:${dist_dir}" "${this_dir}/tmp/${dist_dir}"
file=$(ls "${this_dir}/tmp/${dist_dir}/stage/${module_name}/"*/*".tar.gz" | head -n1 \
               || echo "/tmp/${USER}/failure.tmp")

sha256sum "${file}"
