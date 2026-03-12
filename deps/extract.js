const tar = require("tar");
const fs = require("fs");
const path = require("path");
const tarball = path.resolve(process.argv[2]);
const dirname = path.resolve(process.argv[3]);

tar.extract({
    sync: true,
    file: tarball,
    cwd: dirname,
});

// SQLite >= 3.49 ships a VERSION file that conflicts with the C++20 <version>
// header on case-insensitive filesystems (macOS/Windows).
const base = path.basename(tarball, ".tar.gz");
const versionFile = path.join(dirname, base, "VERSION");
if (fs.existsSync(versionFile)) {
    fs.unlinkSync(versionFile);
}
