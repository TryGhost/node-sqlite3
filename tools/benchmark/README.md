# Benchmark Tools

This directory contains benchmark scripts to measure and compare the performance of different SQLite operations in `@homeofthings/sqlite3` using [tinybench](https://github.com/tinylibs/tinybench).

## Key Features

- **Proper setup/teardown separation**: Database creation, table creation, and data population happen in `beforeEach` (not measured)
- **Only actual operations measured**: The benchmark functions contain only the INSERT/SELECT operations
- **Warmup support**: tinybench includes warmup iterations to allow V8 optimization
- **Statistical accuracy**: Multiple iterations with mean, variance, and margin of error

## Running Benchmarks

### Quick Start

Install dependencies (from project root):

```bash
yarn install
```

Run all benchmarks:

```bash
node tools/benchmark/run.js
```

Or from the benchmark directory:

```bash
cd tools/benchmark
node run.js
```

## Benchmark Scripts

### run.js (Recommended)

The main runner script executes all benchmarks and provides detailed timing statistics including:
- ops/sec (operations per second)
- Mean and margin of error
- Sample variance

### insert.js

Measures performance of different data insertion approaches.

**Benchmark tests:**
- `insert: literal file`: Executes SQL from a file using `db.exec()`
- `insert: transaction with two statements`: Uses parallelized statements within a transaction
- `insert: with transaction`: Uses a single statement within a transaction
- `insert: without transaction`: Uses a single statement without explicit transaction

**Usage:**
```bash
node tools/benchmark/run.js
```

**Expected output:**
```
Task Name                                            ops/sec   Average Time (ns)      Margin   Samples
------------------------------------------------------------------------------------------------------
insert: literal file                                   42.34      23620025909.09       3.71%        22
insert: transaction with two statements                44.45      22495146260.87       3.01%        23
insert: with transaction                               43.08      23212004409.09       6.05%        22
insert: without transaction                            44.64      22399535956.52       5.67%        23
```

**Key findings:**
- Using transactions (`BEGIN`/`COMMIT`) significantly improves performance
- Parallelized statements can further improve performance for concurrent operations
- Without transactions, each statement is committed individually, which is much slower

### select.js

Measures performance of different data retrieval approaches.

**Benchmark tests:**
- `select: db.each()`: Iterates through results row-by-row using a callback
- `select: db.all()`: Retrieves all results into an array at once
- `select: db.all with statement reset()`: Retrieves all results after resetting the statement

**Usage:**
```bash
node tools/benchmark/run.js
```

**Expected output:**
```
Task Name                                            ops/sec   Average Time (ns)      Margin   Samples
------------------------------------------------------------------------------------------------------
select: db.each                                         0.49    2057124731700.00       3.65%        10
select: db.all                                          0.41    2457100030700.00       4.77%        10
select: db.all with statement reset                     0.21    4847062019200.00       1.33%        10
```

**Key findings:**
- `db.all()` is typically faster for small to medium result sets
- `db.each()` is more memory-efficient for large result sets as it processes rows one at a time

## Benchmark Data Files

### insert-transaction.sql

Contains 10,000 INSERT statements wrapped in a transaction for the "insert literal file" benchmark.

### select-data.sql

Contains 10,000 INSERT statements to populate the test database for the select benchmarks.

## Architecture

### Setup/Teardown Pattern

Each benchmark follows this pattern to ensure only the actual operation is measured:

```javascript
{
  async beforeEach(ctx) {
    // Setup: Create database, create table, populate data
    // NOT measured
    ctx.db = new sqlite3.Database(':memory:');
    await promisifyRun(ctx.db, 'CREATE TABLE foo ...');
  },

  async fn(ctx) {
    // Benchmark: Only the operation to measure
    // MEASURED
    await insertData(ctx.db);
  },

  async afterEach(ctx) {
    // Teardown: Close database
    // NOT measured
    await promisifyClose(ctx.db);
  },
}
```

### Context Object

A context object (`ctx`) is passed between setup, benchmark, and teardown:
- `ctx.db`: Database instance
- Custom properties can be added as needed

## Adding New Benchmarks

1. Create a new JavaScript file in this directory (e.g., `update.js`)
2. Export a `benchmarks` object with your benchmark functions:

```javascript
module.exports = {
  benchmarks: {
    'benchmark name': {
      async beforeEach(ctx) {
        // Setup - NOT measured
        ctx.db = new sqlite3.Database(':memory:');
        // ... create tables, populate data
      },

      async fn(ctx) {
        // Benchmark - MEASURED
        // ... your operation here
      },

      async afterEach(ctx) {
        // Teardown - NOT measured
        await promisifyClose(ctx.db);
      },
    },
  },
};
```

3. Import and add your benchmarks in `run.js`
4. Add documentation for your benchmark in this README

## Notes

- Benchmarks run in-memory (`:memory:`) by default for faster execution
- Results may vary based on your hardware and system configuration
- tinybench automatically determines the number of runs needed for statistical accuracy
- Warmup iterations allow V8 to optimize the code before measurement
- For meaningful comparisons, run benchmarks multiple times and compare the results

## Dependencies

- `tinybench`: ^2.9.0 - Modern benchmark library with proper setup/teardown support
