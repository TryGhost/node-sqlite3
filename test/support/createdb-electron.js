
var {app} = require('electron');
var createdb = require('./createdb.js');

createdb(function () {
    setTimeout(function () {
        app.quit();
    }, 20000);
});

