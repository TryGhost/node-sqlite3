export ROOTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

export DRY_RUN="--dry-run"
export PATTERN="*.*"
export CHECK_MD5="--no-check-md5"

function make_shas {
	for i in $(ls *.tar.gz); do
	shasum_file="${i//.tar.gz/.sha1.txt}";
	if [ ! -f "${shasum_file}" ]; then
		echo generating "${shasum_file}"
		shasum $i | awk '{print $1}' > "${shasum_file}"
	fi
	done
}

cd ${ROOTDIR}/../stage/
if [ -d Debug ]; then
cd Debug
make_shas
../../../s3cmd/s3cmd sync --acl-public ${CHECK_MD5} ./${PATTERN} s3://node-sqlite3/Debug/ ${DRY_RUN}
cd ../
fi

if [ -d Release ]; then
cd Release
make_shas
../../../s3cmd/s3cmd sync --acl-public ${CHECK_MD5} ./${PATTERN} s3://node-sqlite3/Release/ ${DRY_RUN}
cd ../
fi

#../../s3cmd/s3cmd ls s3://node-sqlite3/
