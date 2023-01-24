const binary = require('@mapbox/node-pre-gyp');
const path = require('path');
const binding_path = binary.find(path.resolve(path.join(__dirname,'../package.json')), {
    target_arch: process.env.npm_config_target_arch || process.env.npm_config_arch || process.arch
});
let binding = require(binding_path);
if (typeof binding === 'function') {
    const emnapiContext = require('@tybys/emnapi-runtime').createContext();
    const emnapiInitOptions = {
        context: emnapiContext
    };
    try {
        // optional dependency
        // support async_hooks on Node.js
        emnapiInitOptions.nodeBinding = require('@tybys/emnapi-node-binding');
    } catch (_) {
        // ignore
    }
    binding = binding().emnapiInit(emnapiInitOptions);
}
module.exports = exports = binding;
