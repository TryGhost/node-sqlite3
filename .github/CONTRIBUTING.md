# Contributing

General guidelines for contributing to node-sqlite3

## Install Help

If you've landed here due to a failed install of `node-sqlite3` then feel free to create a [new issue](https://github.com/tryghost/node-sqlite3/issues/new) to ask for help. The most likely problem is that we do not yet provide pre-built binaries for your particular platform and so the `node-sqlite3` install attempted a source compile but failed because you are missing the [dependencies for node-gyp](https://github.com/nodejs/node-gyp#installation). Provide as much detail on your problem as possible and we'll try to help. Include:

 - Logs of failed install (preferably from running `npm install sqlite3 --loglevel=info`)
 - Version of `node-sqlite3` you tried to install
 - Node version you are running
 - Operating system and architecture you are running, e.g. `Windows 7 64 bit`.

The release process is documented in the wiki: https://github.com/TryGhost/node-sqlite3/wiki/Release-process
