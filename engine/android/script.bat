@echo off
IF EXIST C:\android\sdk\cmdline-tools\version\bin\ (
C:\android\sdk\cmdline-tools\version\bin\sdkmanager.bat "build-tools;28.0.3" "ndk-bundle" "platform-tools" "platforms;android-28" "tools"
) ELSE IF EXIST C:\android\sdk\tools\bin\ (
C:\android\sdk\tools\bin\sdkmanager.bat "build-tools;28.0.3" "ndk-bundle" "platform-tools" "platforms;android-28" "tools"
) ELSE (
echo can't find "Android command line tools only" folder! && echo. example:"C:\android\sdk\cmdline-tools\version\bin\"
)

@rem sdkmanager.bat --list
