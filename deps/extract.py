import sys
import tarfile
import os
import platform

tarball = os.path.abspath(sys.argv[1])
dirname = os.path.abspath(sys.argv[2])
if platform.system() == 'OS/390':
  import tempfile 
  tempfile = tempfile.NamedTemporaryFile()
  cmdtype = ("cd %s"
             " && (iconv -f IBM-1047 -t ISO8859-1 %s | gunzip -c > %s)"
             " && pax -ofrom=ISO8859-1,to=IBM-1047 -rf %s")
  cmd = cmdtype % (dirname, tarball, tempfile.name, tempfile.name)
  os.system(cmd)
else:
  tfile = tarfile.open(tarball,'r:gz');
  tfile.extractall(dirname)
sys.exit(0)

