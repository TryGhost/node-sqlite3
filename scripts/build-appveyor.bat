@ECHO OFF
SETLOCAL
SET EL=0

ECHO ~~~~~~~~~~~~~~~~~~~~~~~~~~~~ %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

SET PATH=%CD%;%PATH%
SET msvs_version=2013
IF "%msvs_toolset"=="14" SET msvs_version=2015

ECHO APPVEYOR^: %APPVEYOR%
ECHO nodejs_version^: %nodejs_version%
ECHO platform^: %platform%
ECHO msvs_toolset^: %msvs_toolset%
ECHO msvs_version^: %msvs_version%
ECHO TOOLSET_ARGS^: %TOOLSET_ARGS%


ECHO activating VS command prompt
:: NOTE this call makes the x64 -> X64
IF /I "%platform%"=="x64" ECHO x64 && CALL "C:\Program Files (x86)\Microsoft Visual Studio %msvs_toolset%.0\VC\vcvarsall.bat" amd64
IF /I "%platform%"=="x86" ECHO x86 && CALL "C:\Program Files (x86)\Microsoft Visual Studio %msvs_toolset%.0\VC\vcvarsall.bat" x86
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO using compiler^: && cl
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO using MSBuild^: && msbuild /version && ECHO.
IF %ERRORLEVEL% NEQ 0 GOTO ERROR


ECHO downloading/installing node
::only use Install-Product when using VS2013
::IF /I "%APPVEYOR%"=="True" IF /I "%msvs_toolset%"=="12" powershell Install-Product node $env:nodejs_version $env:Platform
::TESTING:
::always install (get npm matching node), but delete installed programfiles node.exe afterwards for VS2015 (using custom node.exe)
IF /I "%APPVEYOR%"=="True" powershell Install-Product node $env:nodejs_version $env:Platform
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

IF /I "%msvs_toolset%"=="12" GOTO NODE_INSTALLED


::custom node for VS2015
SET ARCHPATH=
IF "%platform%"=="X64" (SET ARCHPATH=x64/)
IF "%platform%"=="x64" (SET ARCHPATH=x64/)
SET NODE_URL=https://mapbox.s3.amazonaws.com/node-cpp11/v%nodejs_version%/%ARCHPATH%node.exe
ECHO downloading node^: %NODE_URL%
powershell Invoke-WebRequest "${env:NODE_URL}" -OutFile node.exe
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO deleting node ...
SET NODE_EXE_PRG=%ProgramFiles%\nodejs\node.exe
IF EXIST "%NODE_EXE_PRG%" ECHO found %NODE_EXE_PRG%, deleting... && DEL /F "%NODE_EXE_PRG%"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
SET NODE_EXE_PRG=%ProgramFiles(x86)%\nodejs\node.exe
IF EXIST "%NODE_EXE_PRG%" ECHO found %NODE_EXE_PRG%, deleting... && DEL /F "%NODE_EXE_PRG%"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR


:NODE_INSTALLED

ECHO available node.exe^:
where node
ECHO available npm^:
where npm

ECHO node^: && node -v
node -e "console.log(process.argv,process.execPath)"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO npm^: && CALL npm -v
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO ===== where npm puts stuff START ============
ECHO npm root && CALL npm root
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
ECHO npm root -g && CALL npm root -g
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

ECHO npm bin && CALL npm bin
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
ECHO npm bin -g && CALL npm bin -g
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

SET NPM_BIN_DIR=
FOR /F "tokens=*" %%i in ('CALL npm bin -g') DO SET NPM_BIN_DIR=%%i
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
IF /I "%NPM_BIN_DIR%"=="%CD%" ECHO ERROR npm bin -g equals local directory && SET ERRORLEVEL=1 && GOTO ERROR
ECHO ===== where npm puts stuff END ============


ECHO installing node-gyp
CALL npm install -g node-gyp
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

CALL npm install --build-from-source --msvs_version=%msvs_version% %TOOLSET_ARGS% --loglevel=http
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

FOR /F "tokens=*" %%i in ('CALL node_modules\.bin\node-pre-gyp reveal module --silent') DO SET MODULE=%%i
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
FOR /F "tokens=*" %%i in ('node -e "console.log(process.execPath)"') DO SET NODE_EXE=%%i
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

dumpbin /DEPENDENTS "%NODE_EXE%"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR
dumpbin /DEPENDENTS "%MODULE%"
IF %ERRORLEVEL% NEQ 0 GOTO ERROR


::skipping check for errorlevel npm test result when using io.js
::@springmeyer: how to proceed?
IF NOT "%nodejs_version%"=="1.8.1" IF NOT "%nodejs_version%"=="2.0.0" GOTO CHECK_NPM_TEST_ERRORLEVEL

ECHO calling npm test
CALL npm test
ECHO ==========================================
ECHO ==========================================
ECHO ==========================================
ECHO using iojs, not checking test result!!!!!!!!!
ECHO ==========================================
ECHO ==========================================
ECHO ==========================================

GOTO NPM_TEST_FINISHED


:CHECK_NPM_TEST_ERRORLEVEL
ECHO calling npm test
CALL npm test
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

:NPM_TEST_FINISHED


CALL node_modules\.bin\node-pre-gyp package %TOOLSET_ARGS%
::make commit message env var shorter
SET CM=%APPVEYOR_REPO_COMMIT_MESSAGE%
IF NOT "%CM%" == "%CM:[publish binary]=%" (ECHO publishing && CALL node_modules\.bin\node-pre-gyp --msvs_version=%msvs_version% publish %TOOLSET_ARGS%) ELSE (ECHO not publishing)
IF %ERRORLEVEL% NEQ 0 GOTO ERROR

GOTO DONE



:ERROR
ECHO ~~~~~~~~~~~~~~~~~~~~~~ ERROR %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
ECHO ERRORLEVEL^: %ERRORLEVEL%
SET EL=%ERRORLEVEL%

:DONE
ECHO ~~~~~~~~~~~~~~~~~~~~~~ DONE %~f0 ~~~~~~~~~~~~~~~~~~~~~~~~~~~~

EXIT /b %EL%
