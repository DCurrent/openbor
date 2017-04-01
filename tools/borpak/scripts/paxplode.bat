:: Name: paxplode
:: Author: Plombo
:: Frontend for borpak that emulates the "paxplode" utility.
:: Put this file in the same directory as the borpak executable.
@echo off

if (%1) == () goto USAGE
if not (%2) == () goto USAGE

%0\..\borpak %1
goto END

:USAGE
echo Usage: %0 pakfile
goto END

:END
