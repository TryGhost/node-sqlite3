# Project Overview

## Project: @homeofthings/sqlite3

**Description**: Asynchronous, non-blocking SQLite3 bindings for Node.js

**Repository**: https://github.com/gms1/node-sqlite3 (forked from TryGhost/node-sqlite3)

**Package Name**: `@homeofthings/sqlite3`

**Node.js Version**: >= 20.17.0

## Architecture

```
node-sqlite3/
├── lib/                    # JavaScript API layer
│   ├── sqlite3.js          # Main module entry point
│   ├── sqlite3-binding.js  # Native binding loader
│   ├── sqlite3.d.ts        # TypeScript declarations
│   └── trace.js            # Stack trace augmentation for verbose mode
├── src/                    # C++ native addon
│   ├── node_sqlite3.cc     # Main addon entry
│   ├── database.cc/h       # Database class
│   ├── statement.cc/h      # Statement class
│   ├── backup.cc/h         # Backup class
│   ├── async.h             # Async work utilities
│   ├── macros.h            # Helper macros (ASSERT_STATUS, etc.)
│   └── threading.h         # Threading utilities
├── deps/                   # SQLite dependency
│   ├── sqlite3.gyp         # SQLite build config
│   ├── common-sqlite.gypi  # Common build config
│   └── sqlite-autoconf-*.tar.gz  # SQLite source
├── test/                   # Test suite (mocha)
└── binding.gyp             # node-gyp build configuration
```

## Key Components

### JavaScript Layer (lib/)

- **sqlite3.js**: Main module that wraps the native addon with a friendlier API
  - `Database` class with methods: `prepare`, `run`, `get`, `all`, `each`, `map`, `exec`, `close`
  - `Statement` class with methods: `bind`, `get`, `run`, `all`, `each`, `map`, `reset`, `finalize`
  - `Backup` class for database backup operations
  - Cached database support via `sqlite3.cached.Database`
  - Event emitter integration for `trace`, `profile`, `change` events

- **sqlite3-binding.js**: Loads the native addon using the `bindings` package
  - Searches for `node_sqlite3.node` in build/Debug or build/Release

- **trace.js**: Stack trace augmentation for verbose mode
  - Extends error stack traces to include operation context

### Native Layer (src/)

- **node_sqlite3.cc**: Module initialization, exports `Database`, `Statement`, `Backup` classes
- **database.cc/h**: Database implementation with async operations
- **statement.cc/h**: Prepared statement implementation
- **backup.cc/h**: Database backup implementation
- **macros.h**: Contains `ASSERT_STATUS` macro (enabled in DEBUG mode)

### Build System

- **binding.gyp**: node-gyp configuration
  - Builds `node_sqlite3` target
  - Links against SQLite (internal or external)
  - Defines `NAPI_VERSION` and `NAPI_DISABLE_CPP_EXCEPTIONS=1`

- **deps/common-sqlite.gypi**: Common build configurations
  - `Debug` configuration: disables `NDEBUG`, enables debug symbols
  - `Release` configuration: enables `NDEBUG`, optimizations

## Dependencies

### Runtime
- `bindings`: ^1.5.0 - Native addon loader
- `node-addon-api`: ^8.0.0 - C++ NAPI wrapper
- `prebuild-install`: ^7.1.3 - Prebuilt binary downloader
- `tar`: ^7.5.10 - Tarball handling

### Development
- `mocha`: 10.2.0 - Test framework
- `eslint`: 8.56.0 - Linting
- `prebuild`: 13.0.1 - Prebuilt binary builder
- `tinybench`: ^2.9.0 - Benchmarking

### Peer
- `node-gyp`: 12.x - Native addon build tool

## SQLite Configuration

The SQLite build includes these extensions:
- `SQLITE_THREADSAFE=1` - Thread-safe
- `SQLITE_ENABLE_FTS3/4/5` - Full-text search
- `SQLITE_ENABLE_RTREE` - R-Tree index
- `SQLITE_ENABLE_DBSTAT_VTAB=1` - Database stats virtual table
- `SQLITE_ENABLE_MATH_FUNCTIONS` - Math functions

## Debug Mode

### JavaScript Debug Mode

```javascript
const sqlite3 = require('@homeofthings/sqlite3').verbose();
```

Enables extended stack traces for better error messages.

### Native Debug Build

```bash
# Build with debug configuration
node-gyp rebuild --debug

# Output: build/Debug/node_sqlite3.node
```

This enables:
- `DEBUG` and `_DEBUG` preprocessor macros
- `ASSERT_STATUS()` macro checks (see src/macros.h:140)
- Debug symbols
- No optimizations

## Module Resolution

The `bindings` package searches for the native addon in this order:
1. `build/Debug/node_sqlite3.node`
2. `build/Release/node_sqlite3.node`

If both exist, Debug takes precedence.

## Common Commands

```bash
# Install dependencies
yarn install

# Build native addon (uses prebuild or falls back to node-gyp)
yarn install

# Rebuild native addon
yarn rebuild

# Build with debug configuration
node-gyp rebuild --debug

# Run tests
yarn test

# Run benchmarks
node tools/benchmark/run.js
```

## Related Files

- [Build System](build-system.md) - Detailed build configuration
- [Development Workflow](development.md) - Testing and contributing
