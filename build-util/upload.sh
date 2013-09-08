export ROOTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

export DRY_RUN="--dry-run"

cd ${ROOTDIR}/../stage/
if [ -d Debug ]; then
cd Debug
../../../s3cmd/s3cmd sync --no-check-md5 --acl-public ./*.tar.gz s3://node-sqlite3/Debug/ ${DRY_RUN}
cd ../
fi

if [ -d Release ]; then
cd Release
../../../s3cmd/s3cmd sync --no-check-md5 --acl-public ./*.tar.gz s3://node-sqlite3/Release/ ${DRY_RUN}
cd ../
fi

#../../s3cmd/s3cmd ls s3://node-sqlite3/
