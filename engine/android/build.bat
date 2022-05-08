@setlocal
@echo off
cd ../
set TOOLS=../tools/bin;../tools/7-Zip;../tools/svn/bin
set PATH=%TOOLS%;%PATH%
bash.exe version.sh
cd ./android
@endlocal

set mypath=%~dp0

set "ANDROID_HOME=C:\android\sdk\"

IF NOT EXIST "%ANDROID_HOME%" (
	mkdir "%ANDROID_HOME%"
)

IF NOT EXIST %ANDROID_HOME%cmdline-tools\bin\sdkmanager.bat (
	echo cmdline-tools missing please download and extract to %ANDROID_HOME%
	echo.
	echo example path: %ANDROID_HOME%cmdline-tools\bin\sdkmanager.bat
	pause
	exit
)

IF NOT EXIST %ANDROID_HOME%licenses\ (
	cd %ANDROID_HOME%
	%ANDROID_HOME%cmdline-tools\bin\sdkmanager.bat --sdk_root=%ANDROID_HOME% --licenses
)

cmd /k "cd %mypath% & gradlew.bat clean & gradlew.bat assembleDebug"
@rem clean
@rem assembleRelease
@rem assembleDebug