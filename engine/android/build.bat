@echo off
setlocal EnableExtensions DisableDelayedExpansion

rem ---------------------------------------------------------------------------
rem Caskey, Damon V.
rem 2026-08-29
rem
rem Provide one Windows entry point for OpenBOR Android setup, test packaging,
rem release signing, verified production packaging, and device installation.
rem ---------------------------------------------------------------------------

cd /d "%~dp0"
title OpenBOR Android Builder

call :locate_java
if errorlevel 1 goto :failed

call :locate_android_sdk
if errorlevel 1 goto :failed

call :check_android_packages
if errorlevel 1 goto :failed

if /i "%~1"=="debug" goto :debug
if /i "%~1"=="release" goto :release
if /i "%~1"=="key" goto :key
if /i "%~1"=="settings" goto :settings
if not "%~1"=="" (
    echo Unknown option: %~1
    echo Valid options: debug, release, key, settings
    goto :failed
)

:menu
cls
echo ============================================================
echo OpenBOR Android Builder
echo ============================================================
echo.
echo 1. Build a test APK
echo 2. Build a signed release APK
echo 3. Create the release signing key
echo 4. Edit game settings
echo 5. Open the output folder
echo 6. Exit
echo.
choice /c 123456 /n /m "Choose an option: "

if errorlevel 6 exit /b 0
if errorlevel 5 goto :open_output
if errorlevel 4 goto :settings
if errorlevel 3 goto :key
if errorlevel 2 goto :release
if errorlevel 1 goto :debug

:debug
call :require_game_settings
if errorlevel 1 goto :failed
call :generate_engine_version
if errorlevel 1 goto :failed

echo.
echo Building and verifying the test APK...
call gradlew.bat --no-daemon --console=plain clean packageStandaloneDebug
if errorlevel 1 goto :failed

echo.
echo Test APK completed successfully.
call :offer_device_install
goto :finished

:release
call :require_game_settings
if errorlevel 1 goto :failed
if not exist "keystore.properties" (
    echo.
    echo No release signing key is configured.
    choice /c YN /n /m "Create one now? [Y/N]: "
    if errorlevel 2 goto :failed
    call :create_release_key
    if errorlevel 1 goto :failed
)
call :generate_engine_version
if errorlevel 1 goto :failed

echo.
echo Building, signing, and verifying the release APK...
call gradlew.bat --no-daemon --console=plain clean packageStandaloneRelease
if errorlevel 1 goto :failed

echo.
echo Release APK completed successfully.
goto :finished

:key
call :create_release_key
if errorlevel 1 goto :failed
goto :finished

:settings
if not exist "game.properties" (
    copy /y "game.properties.example" "game.properties" >nul
    if errorlevel 1 goto :failed
    echo Created game.properties from the supplied example.
)
start "" /wait notepad.exe "%cd%\game.properties"
if "%~1"=="" goto :menu
exit /b 0

:open_output
if not exist "output" mkdir "output"
start "" explorer.exe "%cd%\output"
goto :menu

:locate_java
set "JAVA_EXE="

if defined JAVA_HOME if exist "%JAVA_HOME%\bin\java.exe" (
    set "JAVA_EXE=%JAVA_HOME%\bin\java.exe"
    goto :check_java_version
)

for /d %%D in ("%ProgramFiles%\Eclipse Adoptium\jdk-17*") do if exist "%%~fD\bin\java.exe" (
    set "JAVA_HOME=%%~fD"
    set "JAVA_EXE=%%~fD\bin\java.exe"
    goto :check_java_version
)

for /d %%D in ("%LOCALAPPDATA%\Programs\Eclipse Adoptium\jdk-17*") do if exist "%%~fD\bin\java.exe" (
    set "JAVA_HOME=%%~fD"
    set "JAVA_EXE=%%~fD\bin\java.exe"
    goto :check_java_version
)

if exist "%ProgramFiles%\Android\Android Studio\jbr\bin\java.exe" (
    set "JAVA_HOME=%ProgramFiles%\Android\Android Studio\jbr"
    set "JAVA_EXE=%ProgramFiles%\Android\Android Studio\jbr\bin\java.exe"
    goto :check_java_version
)

for /f "delims=" %%J in ('where java.exe 2^>nul') do if not defined JAVA_EXE set "JAVA_EXE=%%~fJ"
if not defined JAVA_EXE (
    echo JDK 17 was not found.
    echo Install Eclipse Temurin JDK 17, then run this builder again.
    exit /b 1
)
for %%D in ("%JAVA_EXE%\..\..") do set "JAVA_HOME=%%~fD"

:check_java_version
rem ---------------------------------------------------------------------------
rem Caskey, Damon V.
rem 2026-08-30
rem
rem Read version metadata from the JDK release file. This avoids cmd.exe
rem breaking a quoted Java executable path that contains spaces inside FOR /F.
rem ---------------------------------------------------------------------------
set "JAVA_VERSION="
set "JAVA_MAJOR="

if not exist "%JAVA_HOME%\bin\javac.exe" (
    echo JDK 17 is required. javac.exe was not found under:
    echo %JAVA_HOME%
    exit /b 1
)
if not exist "%JAVA_HOME%\release" (
    echo JDK release metadata was not found under:
    echo %JAVA_HOME%
    exit /b 1
)

for /f "tokens=2 delims==" %%V in ('findstr /b /c:"JAVA_VERSION=" "%JAVA_HOME%\release"') do if not defined JAVA_VERSION set "JAVA_VERSION=%%~V"
for /f "tokens=1 delims=." %%M in ("%JAVA_VERSION%") do set "JAVA_MAJOR=%%M"
if not "%JAVA_MAJOR%"=="17" (
    echo JDK 17 is required. Found Java %JAVA_VERSION% at:
    echo %JAVA_EXE%
    exit /b 1
)
set "PATH=%JAVA_HOME%\bin;%PATH%"
exit /b 0

:locate_android_sdk
set "ANDROID_SDK="
if defined ANDROID_SDK_ROOT if exist "%ANDROID_SDK_ROOT%" set "ANDROID_SDK=%ANDROID_SDK_ROOT%"
if not defined ANDROID_SDK if defined ANDROID_HOME if exist "%ANDROID_HOME%" set "ANDROID_SDK=%ANDROID_HOME%"
if not defined ANDROID_SDK if exist "%LOCALAPPDATA%\Android\Sdk" set "ANDROID_SDK=%LOCALAPPDATA%\Android\Sdk"
if not defined ANDROID_SDK if exist "C:\android\sdk" set "ANDROID_SDK=C:\android\sdk"

if not defined ANDROID_SDK (
    echo Android SDK was not found.
    echo Install Android Studio or the Android command-line tools first.
    exit /b 1
)

for %%D in ("%ANDROID_SDK%") do set "ANDROID_SDK=%%~fD"
set "ANDROID_HOME=%ANDROID_SDK%"
set "ANDROID_SDK_ROOT=%ANDROID_SDK%"
exit /b 0

:check_android_packages
set "MISSING_ANDROID_PACKAGES="
if not exist "%ANDROID_SDK%\platforms\android-35\android.jar" set "MISSING_ANDROID_PACKAGES=1"
if not exist "%ANDROID_SDK%\build-tools\36.0.0\aapt2.exe" set "MISSING_ANDROID_PACKAGES=1"
if not exist "%ANDROID_SDK%\ndk\21.4.7075529\ndk-build.cmd" set "MISSING_ANDROID_PACKAGES=1"
if not defined MISSING_ANDROID_PACKAGES exit /b 0

echo Required Android SDK components are missing.
set "SDKMANAGER="
for /r "%ANDROID_SDK%\cmdline-tools" %%F in (sdkmanager.bat) do if not defined SDKMANAGER set "SDKMANAGER=%%~fF"
if not defined SDKMANAGER (
    echo Install these components through Android Studio SDK Manager:
    echo   Android SDK Platform 35
    echo   Android SDK Build-Tools 36.0.0
    echo   NDK 21.4.7075529
    exit /b 1
)

choice /c YN /n /m "Install the missing components now? [Y/N]: "
if errorlevel 2 exit /b 1

call "%SDKMANAGER%" --sdk_root="%ANDROID_SDK%" --licenses
if errorlevel 1 exit /b 1
call "%SDKMANAGER%" --sdk_root="%ANDROID_SDK%" "platforms;android-35" "build-tools;36.0.0" "ndk;21.4.7075529"
if errorlevel 1 exit /b 1
exit /b 0

:require_game_settings
if exist "game.properties" exit /b 0
echo.
echo game.properties does not exist yet.
copy /y "game.properties.example" "game.properties" >nul
if errorlevel 1 exit /b 1
echo Enter the game settings in the Notepad window, then save and close it.
start "" /wait notepad.exe "%cd%\game.properties"
exit /b 0

:generate_engine_version
set "ENGINE_ROOT=%cd%\.."
set "REPOSITORY_ROOT=%cd%\..\.."
if not exist "%REPOSITORY_ROOT%\.git" (
    if not exist "%ENGINE_ROOT%\version.tmp" (
        echo Engine version metadata is missing.
        exit /b 1
    )
    copy /y "%ENGINE_ROOT%\version.tmp" "%ENGINE_ROOT%\version.h" >nul
    if errorlevel 1 exit /b 1
    exit /b 0
)

set "VERSION_PATH=%REPOSITORY_ROOT%\tools\bin;%REPOSITORY_ROOT%\tools\7-Zip;%REPOSITORY_ROOT%\tools\svn\bin"
pushd "%ENGINE_ROOT%"
set "PATH=%VERSION_PATH%;%PATH%"
"%REPOSITORY_ROOT%\tools\bin\bash.exe" version.sh
set "VERSION_EXIT=%errorlevel%"
popd
if not "%VERSION_EXIT%"=="0" exit /b %VERSION_EXIT%
if not exist "%ENGINE_ROOT%\version.h" (
    echo Engine version.h was not generated.
    exit /b 1
)
exit /b 0

:create_release_key
echo.
echo Creating the permanent release signing key...
call gradlew.bat --no-daemon --console=plain createReleaseKey
if errorlevel 1 exit /b 1
echo.
echo Keep release-key.jks and keystore.properties together in secure backups.
echo Losing this key can prevent future updates to the application.
exit /b 0

:offer_device_install
if not exist "%ANDROID_SDK%\platform-tools\adb.exe" exit /b 0
choice /c YN /n /m "Install the test APK on a connected device? [Y/N]: "
if errorlevel 2 exit /b 0

set "TEST_APK="
if exist "%cd%\output\latest-debug-apk.txt" set /p "TEST_APK="<"%cd%\output\latest-debug-apk.txt"
if not defined TEST_APK (
    echo Verified test APK was not found in the output folder.
    exit /b 1
)
if not exist "%TEST_APK%" (
    echo Verified test APK no longer exists: %TEST_APK%
    exit /b 1
)
"%ANDROID_SDK%\platform-tools\adb.exe" install -r "%TEST_APK%"
exit /b %errorlevel%

:finished
echo.
echo Finished. Packages are available in:
echo %cd%\output
if "%~1"=="" (
    echo.
    pause
    goto :menu
)
exit /b 0

:failed
echo.
echo Build stopped. Review the message above.
if "%~1"=="" pause
exit /b 1
