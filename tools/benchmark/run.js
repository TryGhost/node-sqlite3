const benchmark = require('benchmark');
const path = require('path');

// Set working directory to benchmark folder for relative path resolution
process.chdir(__dirname);

// Load benchmarks
const insertBenchmarks = require('./insert');
const selectBenchmarks = require('./select');

// Set up event handlers
const suite = new benchmark.Suite();

// Helper to add a benchmark
function addBenchmark(name, fn) {
    suite.add(name, {
        fn: fn,
        defer: true,
        onComplete: function() {
            console.log(String(this));
        }
    });
}

// Add insert benchmarks
console.log('\n=== INSERT BENCHMARKS ===\n');
Object.keys(insertBenchmarks.compare).forEach(name => {
    addBenchmark(`insert: ${name}`, function(deferred) {
        insertBenchmarks.compare[name](deferred.resolve.bind(deferred));
    });
});

// Add select benchmarks
console.log('\n=== SELECT BENCHMARKS ===\n');
Object.keys(selectBenchmarks.compare).forEach(name => {
    addBenchmark(`select: ${name}`, function(deferred) {
        selectBenchmarks.compare[name](deferred.resolve.bind(deferred));
    });
});

// Run all benchmarks
console.log('Running benchmarks...\n');
suite.run();
