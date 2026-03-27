const { Bench } = require('tinybench');
const path = require('path');

// Set working directory to benchmark folder for relative path resolution
process.chdir(__dirname);

// Load benchmarks
const insertBenchmarks = require('./insert');
const selectBenchmarks = require('./select');

async function runBenchmarks() {
    console.log('Running sqlite3 benchmarks...\n');

    const suite = new Bench({
        time: 500,               // Run each benchmark for 500ms
        warmupTime: 50,          // Warmup time in ms
        warmupIterations: 5,    // Minimum warmup iterations
    });

    // Add insert benchmarks
    console.log('=== INSERT BENCHMARKS ===\n');
    for (const [name, benchmark] of Object.entries(insertBenchmarks.benchmarks)) {
        suite.add(`insert: ${name}`, benchmark.fn, {
            beforeEach: benchmark.beforeEach,
            afterEach: benchmark.afterEach,
        });
    }

    // Add select benchmarks
    console.log('=== SELECT BENCHMARKS ===\n');
    for (const [name, benchmark] of Object.entries(selectBenchmarks.benchmarks)) {
        suite.add(`select: ${name}`, benchmark.fn, {
            beforeEach: benchmark.beforeEach,
            afterEach: benchmark.afterEach,
        });
    }

    // Run all benchmarks
    await suite.run();

    // Output results with full precision
    console.log('\n=== RESULTS ===\n');

    // Header
    const header = [
        'Task Name'.padEnd(45),
        'ops/sec'.padStart(15),
        'Average Time (ns)'.padStart(20),
        'Margin'.padStart(12),
        'Samples'.padStart(10)
    ].join('');
    console.log(header);
    console.log('-'.repeat(102));

    // Results
    for (const task of suite.tasks) {
        if (task.result && !task.result.error) {
            const opsSec = task.result.hz.toFixed(2);
            const avgTime = (task.result.mean * 1e9).toFixed(2);
            const margin = task.result.rme.toFixed(2);
            const samples = task.result.samples.length;

            const row = [
                task.name.padEnd(45),
                opsSec.padStart(15),
                avgTime.padStart(20),
                (margin + '%').padStart(12),
                samples.toString().padStart(10)
            ].join('');
            console.log(row);
        } else if (task.result?.error) {
            console.log(`${task.name.padEnd(45)} ERROR: ${task.result.error.message}`);
        }
    }
}

runBenchmarks().catch((err) => {
    console.error('Benchmark failed:', err);
    process.exit(1);
});
