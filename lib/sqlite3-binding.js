const binary = require('@mapbox/node-pre-gyp');
const path = require('path');
const binding_path = binary.find(path.resolve(path.join(__dirname,'../package.json')));
let binding = require(binding_path);
if (typeof binding === 'function') {
    const emnapiContext = require('@tybys/emnapi-runtime').createContext();
    binding = binding().emnapiInit({ context: emnapiContext });
}
module.exports = exports = binding;
