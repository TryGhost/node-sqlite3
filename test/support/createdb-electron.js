
const {app} = require('electron');
const createdb = require('./createdb.js');

createdb().then(() => {
    setTimeout(function () {
        app.quit();
    }, 20000);
});

