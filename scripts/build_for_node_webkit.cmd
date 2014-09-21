echo Platform: %1
echo The list of environment variables:
set
if not "%1" == "x86" goto end
if "%nw_version%" == "" goto end
call npm install nw-gyp
call cinst wget 7zip.commandline
call wget http://dl.node-webkit.org/v%nw_version%/node-webkit-v%nw_version%-win-ia32.zip
call 7z e -onw node-webkit-v%nw_version%-win-ia32.zip
dir nw
set PATH=nw;%PATH%
call node-pre-gyp rebuild --runtime=node-webkit --target=%nw_version% --target_arch=ia32
call node-pre-gyp package testpackage --runtime=node-webkit --target=%nw_version% --target_arch=ia32
if not "%CM%" == "%CM:[publish binary]=%" call node-pre-gyp publish --msvs_version=2013 --runtime=node-webkit --target=%nw_version% --target_arch=ia32
:end