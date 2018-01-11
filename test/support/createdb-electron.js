
var {app} = require('electron');
var createdb = require('./createdb.js');

createdb(function () {
    app.quit();
});
