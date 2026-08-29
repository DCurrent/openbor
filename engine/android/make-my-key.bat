@echo off
rem Compatibility entry point for the original OpenBOR Android helper.
call "%~dp0build.bat" key
exit /b %errorlevel%
