
var {app} = require('electron');
var createdb = require('./createdb.js');

createdb().then(() => {
    setTimeout(function () {
        app.quit();
    }, 20000);
});

