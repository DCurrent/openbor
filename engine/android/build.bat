@setlocal
@echo off
cd ../
set TOOLS=../tools/bin;../tools/7-Zip;../tools/svn/bin
set PATH=%TOOLS%;%PATH%
bash.exe version.sh
cd android
@endlocal
@echo
cmd /k "gradlew.bat clean & gradlew.bat assembleDebug"
@rem clean
@rem assembleRelease
@rem assembleDebug