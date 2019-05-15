@setlocal
@echo off
cd ../
set BUILDBATCH=1
set TOOLS=../tools/bin;../tools/7-Zip;../tools/svn/bin
set PATH=%TOOLS%
bash.exe version.sh
cd android
gradlew.bat clean
gradlew.bat assembleDebug
@endlocal
@rem assembleRelease
@rem assembleDebug