# Development Workflow

## Setup

### Prerequisites

- Node.js >= 20.17.0
- Python 3 (for node-gyp)
- C++ compiler (gcc, clang, or MSVC)
- yarn or npm

### Installation

```bash
# Clone repository
git clone https://github.com/gms1/node-sqlite3.git
cd node-sqlite3

# Install dependencies
yarn install

# Build native addon (automatic during install, or manual)
yarn rebuild
```

## Testing

### Run All Tests

```bash
yarn test
```

This runs:
1. `node test/support/createdb.js` - Creates test database
2. `mocha -R spec --timeout 480000` - Runs test suite

### Test Structure

```
test/
├── support/
│   ├── createdb.js      # Test database setup
│   ├── helper.js        # Test utilities
│   └── prepare.db       # Pre-populated test database
├── affected.test.js     # Affected rows tests
├── async_calls.test.js  # Async call tests
├── backup.test.js       # Backup API tests
├── blob.test.js         # BLOB handling tests
├── cache.test.js        # Cached database tests
├── constants.test.js    # Constants tests
├── database_fail.test.js # Database failure tests
├── each.test.js         # each() method tests
├── exec.test.js         # exec() method tests
├── extension.test.js    # Extension loading tests
├── fts-content.test.js  # Full-text search tests
├── interrupt.test.js    # Interrupt tests
├── issue-108.test.js    # Regression tests
├── json.test.js         # JSON handling tests
├── limit.test.js        # LIMIT tests
├── map.test.js          # map() method tests
├── named_columns.test.js # Named columns tests
├── named_params.test.js  # Named parameters tests
├── null_error.test.js   # NULL error handling tests
├── open_close.test.js   # Open/close tests
├── other_objects.test.js # Other object tests
├── parallel_insert.test.js # Parallel insert tests
├── patching.test.js     # Patching tests
├── prepare.test.js      # prepare() method tests
├── profile.test.js      # Profile API tests
├── rerun.test.js        # Rerun tests
├── scheduling.test.js   # Scheduling tests
├── serialization.test.js # Serialization tests
├── trace.test.js        # Trace API tests
├── unicode.test.js      # Unicode handling tests
├── update_hook.test.js  # Update hook tests
├── upsert.test.js       # UPSERT tests
└── verbose.test.js      # Verbose mode tests
```

### Running Specific Tests

```bash
# Run specific test file
npx mocha test/verbose.test.js

# Run with specific reporter
npx mocha -R spec test/each.test.js

# Run with increased timeout
npx mocha --timeout 10000 test/blob.test.js
```

## Debugging

### JavaScript Debug Mode

Enable extended stack traces:

```javascript
const sqlite3 = require('@homeofthings/sqlite3').verbose();
```

This adds context to error stack traces (see lib/trace.js).

### Native Debug Build

```bash
# Build with debug symbols
node-gyp rebuild --debug

# Output: build/Debug/node_sqlite3.node
```

### Debugging with LLDB/GDB

```bash
# Run Node with debugger
lldb -- node test/verbose.test.js

# Set breakpoint on native function
(lldb) b Database::Open
```

### Verbose Logging

The native addon supports trace and profile events:

```javascript
const db = new sqlite3.Database(':memory:');

db.on('trace', (sql) => {
  console.log('SQL:', sql);
});

db.on('profile', (sql, time) => {
  console.log('SQL:', sql, 'Time:', time, 'ms');
});
```

## Code Style

### Linting

```bash
yarn lint
```

Uses ESLint with configuration in `.eslintrc.js`.

### Code Style Guidelines

- Use `const` and `let` (not `var`)
- Use arrow functions for callbacks
- Use async/await where possible
- Follow existing naming conventions
- Add JSDoc comments for public APIs

## Benchmarks

Benchmarks use tinybench with proper setup/teardown separation:

```bash
node tools/benchmark/run.js
```

See [Project Overview](project-overview.md) for benchmark details.

## Making Changes

**IMPORTANT**: After making any code changes, always run:
```bash
yarn lint --fix
yarn test
```

### Native Code Changes

1. Modify files in `src/`
2. Rebuild: `yarn rebuild`
3. Run lint: `yarn lint --fix`
4. Run tests: `yarn test`

### JavaScript Code Changes

1. Modify files in `lib/`
2. No rebuild needed
3. Run lint: `yarn lint --fix`
4. Run tests: `yarn test`

### Adding New Tests

1. Create `test/your-feature.test.js`
2. Follow existing test patterns
3. Import helper from `./support/helper.js`
4. Run: `yarn test`

## Pull Request Guidelines

1. Fork the repository
2. Create a feature branch
3. Make changes with tests
4. Run linting: `yarn lint --fix`
5. Run tests: `yarn test`
6. Submit pull request

## Common Issues

### Build Fails on macOS

Ensure Xcode Command Line Tools are installed:

```bash
xcode-select --install
```

### Build Fails on Linux

Ensure build tools are installed:

```bash
# Ubuntu/Debian
sudo apt-get install build-essential python3

# Fedora
sudo dnf install gcc-c++ make python3
```

### Build Fails on Windows

Ensure Visual Studio Build Tools are installed with:
- Desktop development with C++
- Windows SDK

### Tests Fail with "database is locked"

Tests use in-memory databases by default. If using file-based databases, ensure proper cleanup.

### Native Module Version Mismatch

Rebuild for your Node.js version:

```bash
yarn rebuild
```

## Related Files

- [Project Overview](project-overview.md) - Architecture and components
- [Build System](build-system.md) - Build configuration and options
