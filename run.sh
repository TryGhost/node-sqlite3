#!/bin/bash
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

set -e
set -x

env_()
{
    project="node-sqlite3"
    org="tizenteam"
    branch="master"
    url_suffix="#{branch}"

    # user="rzr" # Update here if forking
    # org="TizenTeam"
    # branch="sandbox/${user}/${branch}"
    # url_suffix="#{branch}"
    url_suffix="" # TODO: For older docker

    url="https://github.com/${org}/${project}.git${url_suffix}"
    run_url="https://raw.githubusercontent.com/${org}/${project}/${branch}/run.sh"

    release="0.0.0"
    src=false
    if [ -d '.git' ] && which git > /dev/null 2>&1 ; then
        src=true
        branch=$(git rev-parse --abbrev-ref HEAD) ||:
        release=$(git describe --tags || echo "$release")
    fi

    SELF="$0"
    [ "$SELF" != "$SHELL" ] || SELF="${PWD}/run.sh"
    [ "$SELF" != "/bin/bash" ] || SELF="${DASH_SOURCE}"
    [ "$SELF" != "/bin/bash" ] || SELF="${BASH_SOURCE}"
    self_basename=$(basename -- "${SELF}")
}


usage_()
{
    cat<<EOF
Usage:
$0
or
curl -sL "${run_url}" | bash -

EOF
}


die_()
{
    errno=$?
    echo "error: [$errno] $@"
    exit $errno
}


setup_debian_()
{
    which git || sudo apt-get install git
    which docker || sudo apt-get install docker.io

    sudo apt-get install qemu qemu-user-static binfmt-support
    sudo update-binfmts --enable qemu-arm
}


setup_()
{
    docker version && return $? ||:

    if [ -r /etc/debian_version ] ; then
        setup_debian_
    else
        cat<<EOF
warning: OS not supported
Please ask for support at:
${url}
EOF
        cat /etc/os-release ||:
    fi

    docker version && return $? ||:
    docker --version || die_ "please install docker"
    groups | grep docker \
        || sudo addgroup ${USER} docker \
        || die_ "${USER} must be in docker group"
    su -l $USER -c "docker version" \
        && { su -l $USER -c "$SHELL $SELF $@" ; exit $?; } \
        || die_ "unexpected error"
}


prep_()
{
    echo "Prepare: "
    cat /etc/os-release
    docker version || setup_
}


build_()
{
    version="latest"
    outdir="${PWD}/tmp/out"
    container="${project}"
    branch_name=$(echo "${branch}" | sed -e 's|/|.|g')
    dir="/usr/local/src/${project}/"
    image="${user}/${project}/${branch}"
    tag="$image:${version}"
    tag="${org}/${project}:${branch_name}"
    tag="${org}/${project}:${branch_name}.${release}"
    tag=$(echo "${tag}" | tr [A-Z] [a-z])
    container="${project}"
    container=$(echo "${container}" | sed -e 's|/|-|g')
    if $src && [ "run.sh" = "${self_basename}" ] ; then
        docker build -t "${tag}" .
    else
        docker build -t "${tag}" "${url}"
    fi
    docker rm "${container}" > /dev/null 2>&1 ||:
    docker create --name "${container}" "${tag}" /bin/true
    rm -rf "${outdir}"
    mkdir -p "${outdir}"
    docker cp "${container}:${dir}" "${outdir}"
    echo "Check Ouput files in:"
    ls "${outdir}/"*
    find "${outdir}" -type f -a -iname "*.tgz" -o -iname "*.tar.*"
}


test_()
{
    curl -sL "${run_url}" | bash -
}


main_()
{
    env_ "$@"
    usage_ "$@"
    prep_ "$@"
    build_ "$@"
}


main_ "$@"
