import sys
import tarfile
import os

tarball = sys.argv[1]

if not os.path.exists(tarball.replace('.tar.gz','')):
    tarfile.open().extractall('.')