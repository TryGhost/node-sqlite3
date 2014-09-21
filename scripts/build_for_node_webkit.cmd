echo Platform: %1
echo The list of environment variables:
set
if not "%1" == "x86" goto end
if "%nw_version%" == "" goto end
npm install nw-gyp
cinst wget 7zip.commandline
wget http://dl.node-webkit.org/v%nw_version%/node-webkit-v%nw_version%-win-ia32.zip
7z e -onw node-webkit-v%nw_version%-win-ia32.zip
dir nw
set PATH=nw;%PATH%
node-pre-gyp rebuild --runtime=node-webkit --target=%nw_version% --target_arch=ia32
node-pre-gyp package testpackage --runtime=node-webkit --target=%nw_version% --target_arch=ia32
if not "%CM%" == "%CM:[publish binary]=%" node-pre-gyp publish --msvs_version=2013 --runtime=node-webkit --target=%nw_version% --target_arch=ia32
:end