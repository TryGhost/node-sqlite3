# Benchmark Tools

This directory contains benchmark scripts to measure and compare the performance of different SQLite operations in `@homeofthings/sqlite3`.

## Running Benchmarks

### Quick Start

Install the benchmark dependencies:

```bash
cd tools/benchmark
npm install
```

Run all benchmarks:

```bash
node run.js
```

### Manual Execution

You can also run individual benchmark scripts directly:

```bash
node insert.js
node select.js
```

## Benchmark Scripts

### run.js (Recommended)

The main runner script executes all benchmarks and provides detailed timing statistics including:
-.ops/sec (operations per second)
- Mean and margin of error
- Sample variance

### insert.js

Measures performance of different data insertion approaches.

**Benchmark tests:**
- `insert literal file`: Executes SQL from a file using `db.exec()`
- `insert with transaction and two statements`: Uses parallelized statements within a transaction
- `insert with transaction`: Uses a single statement within a transaction
- `insert without transaction`: Uses a single statement without explicit transaction

**Usage:**
```bash
node insert.js
```

**Expected output (when using run.js):**
```
insert: insert literal file x X.XX ops/sec ±X.XX% (XXX runs sampled)
insert: insert with transaction and two statements x X.XX ops/sec ±X.XX% (XXX runs sampled)
insert: insert with transaction x X.XX ops/sec ±X.XX% (XXX runs sampled)
insert: insert without transaction x X.XX ops/sec ±X.XX% (XXX runs sampled)
```

**Key findings:**
- Using transactions (`BEGIN`/`COMMIT`) significantly improves performance
- Parallelized statements can further improve performance for concurrent operations
- Without transactions, each statement is committed individually, which is much slower

### select.js

Measures performance of different data retrieval approaches.

**Benchmark tests:**
- `db.each()`: Iterates through results row-by-row using a callback
- `db.all()`: Retrieves all results into an array at once
- `db.all with reset()`: Retrieves all results after resetting the database

**Usage:**
```bash
node select.js
```

**Expected output (when using run.js):**
```
select: db.each x X.XX ops/sec ±X.XX% (XXX runs sampled)
select: db.all x X.XX ops/sec ±X.XX% (XXX runs sampled)
select: db.all with reset x X.XX ops/sec ±X.XX% (XXX runs sampled)
```

**Key findings:**
- `db.all()` is typically faster for small to medium result sets
- `db.each()` is more memory-efficient for large result sets as it processes rows one at a time

## Benchmark Data Files

### insert-transaction.sql

Contains 10,000 INSERT statements wrapped in a transaction for the "insert literal file" benchmark.

### select-data.sql

Contains 10,000 INSERT statements to populate the test database for the select benchmarks.

## Adding New Benchmarks

1. Create a new JavaScript file in this directory (e.g., `update.js`)
2. Export a `compare` object with your benchmark functions:

```javascript
exports.compare = {
    'benchmark name': function(finished) {
        // Your benchmark code here
        finished(); // Call when done
    }
};
```

3. The `run.js` script will automatically include your new benchmarks
4. Add documentation for your benchmark in this README

## Notes

- Benchmarks run in-memory (`:memory:`) by default for faster execution
- Results may vary based on your hardware and system configuration
- The `benchmark` library automatically determines the number of runs needed for statistical accuracy
- For meaningful comparisons, run benchmarks multiple times and compare the results
