<head>
<title>node-sqlite</title>
<style>
  pre, code { color: #060; font-size: 11pt; }
  pre { margin-left: 2ex; padding: 1ex; background-color: #eee; }
  p { font-size: 12pt; }
body { 
 margin-left: 10%; 
  margin-right: 10%;
  background-color: #fff; 
  color: black; 
  max-width: 800px;
}
  h1,h2,h3,h4 { font-family: helvetica }
h1 { font-size: 36pt; }
h1 { 
  background-color: #58b; 
 color: white;
  padding-left:6px;
}
h2 { 
  color: #28b; 
}
</style>
</head>

<body>
<h1>node-sqlite</h1>

<a href=http://sqlite.org/>SQLite</a> bindings for 
<a //href=http://nodejs.org/>Node</a>.

<p>
The semantics conform somewhat to those of the <a
href=http://dev.w3.org/html5/webdatabase/#sql>HTML5 Web SQL API</a>,
plus some extensions. Also, only the synchronous API is implemented;
the asynchronous API is a big TODO item.

<h2>Download</h2>
<p>
The spiritual home of node-sqlite is at <a href=http://grumdrig.com/node-sqlite/>http://grumdrig.com/node-sqlite/</a>.

<p>
The code lives at <a href=http://code.google.com/p/node-sqlite/>http://code.google.com/p/node-sqlite/</a>.

<? readfile('examples.html') ?>


<h2>Installation</h2>

<p>
<ol>
<li>Install <a //href=http://nodejs.org/>Node</a> and
<a href=http://sqlite.org/>SQLite</a>.

<p>
<li>Get the code

<pre>
$ hg clone https://node-sqlite.googlecode.com/hg/ node-sqlite
</pre>

<li>Configure and build

<pre>
$ cd node-sqlite
$ node-waf configure
$ node-waf build
</pre>

<li>Test:

<pre>
$ node test.js
$ node doc/examples.js
</pre>

</ol>

<p>
  The two files needed to use this library are <code>sqlite.js</code> and 
<code>build/default/sqlite3_bindings.node</code>; copy them where you need 
them.