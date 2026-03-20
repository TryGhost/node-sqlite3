# Development and Release Guide

## Version Bumping

This project uses [npm version](https://docs.npmjs.com/cli/v10/commands/npm-version) to manage version releases.

### How to release a new version

1. **Bump the version** (this will create a Git tag automatically):
   ```bash
   npm version <major|minor|patch>
   ```

   For example:
   ```bash
   npm version patch
   ```

2. **Push the changes and tags**:
   ```bash
   git push origin main --tags
   ```

   The CI workflow will automatically:
   - Build binaries for all platforms
   - Upload them as release assets
   - Publish the package to npm

3. ** Create GitHub Release from tag

### Version format

- Versions follow [SemVer](https://semver.org/) format
- Tags should be prefixed with `v`, e.g., `v6.0.2`
- The version in `package.json` must match the Git tag version

## Release process

When you push a tag (e.g., `v6.0.2`), the CI workflow will:

1. Build prebuilt binaries for:
   - macOS (x64, arm64)
   - Linux (x64, arm64)
   - Windows (x64, ia32)

2. Upload binaries to GitHub Release

3. Publish to npm using trusted publishing (no NPM_TOKEN required)

## Checking the release

After releasing, you can verify:
- GitHub Release with binaries: https://github.com/gms1/node-sqlite3/releases
- npm package: https://www.npmjs.com/package/@homeofthings/sqlite3
