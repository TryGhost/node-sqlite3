# Contributing

General guidelines for contributing to node-sqlite3

## Developing / Pre-release

Create a milestone for the next release on github. If all anticipated changes are back compatible then a `patch` release is in order. If minor API changes are needed then a `minor` release is in order. And a `major` bump is warranted if major API changes are needed.

Assign tickets and pull requests you are working to the milestone you created.

## Releasing

To release a new version:

**1)** Ensure tests are passing

Before considering a release all the tests need to be passing on appveyor and travis.

**2)** Bump commit

Bump the version in `package.json` like https://github.com/mapbox/node-sqlite3/commit/77d51d5785b047ff40f6a8225051488a0d96f7fd

What if you already committed the `package.json` bump and you have no changes to commit but want to publish binaries? In this case you can do:

```sh
git commit --allow-empty -m "[publish binary]"
```

**3)** Ensure binaries built

Check the travis and appveyor pages to ensure they are all green as an indication that the `[publish binary]` command worked.

If you need to republish binaries you can do this with the command below, however this should not be a common thing for you to do!

```sh
git commit --allow-empty -m "[republish binary]"
```

Note: NEVER republish binaries for an existing released version.

**7)** Officially release

An official release requires:

 - Updating the CHANGELOG.md
 - Create and push github tag like `git tag v3.1.1` -m "v3.1.1" && git push --tags`
 - Ensure you have a clean checkout (no extra files in your check that are not known by git). You need to be careful, for instance, to avoid a large accidental file being packaged by npm. You can get a view of what npm will publish by running `make testpack`
 - Fully rebuild and ensure install from binary works: `make clean && npm install --fallback-to-build=false`
 - Then publish the module to npm repositories by running `npm publish`
