module.exports = {
    "extends": "eslint:recommended",
    "env": {
        "es2018": true,
        "node": true
    },
    "rules": {
        "no-var": "error",
        "prefer-const": "error",
        "indent": ["error", 4],
        "linebreak-style": ["error", "unix"],
        "semi": ["error", "always"],
        "no-cond-assign": ["error", "always"]
    }
};
