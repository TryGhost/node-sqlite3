var sys = require("sys");

function handler(curr, prev) {
  sys.puts("Handling");
  sys.puts("the current mtime is: " + curr.mtime);
  sys.puts("the previous mtime was: " + prev.mtime);
  sys.exec("clear;rm -f test.db; node-waf build && node test.js");
}

sys.puts(JSON.stringify(process.ARGV));
for (f in process.ARGV) {
  f = process.ARGV[f]
  sys.puts("Watching " + f);
  process.watchFile(f, handler);
}

//var tcp = require("tcp");
//var server = tcp.createServer();
//server.listen(7000, "localhost");

for (;;) {
  sys.exec("sleep 1").wait();
}

//var p = new process.Promise();
//p.wait(); 