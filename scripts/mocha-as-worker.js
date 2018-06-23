// Run the mocha tests in a worker
// Not a clean approach, but is sufficient to verify correctness
const worker_threads = require("worker_threads");
const path = require("path");

if (worker_threads.isMainThread) {
	const worker = new worker_threads.Worker(__filename, { workerData: { windowSize: process.stdout.getWindowSize() } });
	worker.on("error", console.error);
} else {
	process.stdout.getWindowSize = () => worker_threads.workerData.windowSize;
	const mochaPath = path.resolve(require.resolve("mocha"), "../bin/_mocha");
	require(mochaPath);
}
