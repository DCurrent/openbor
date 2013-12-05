@rem
@rem OpenBOR - http://www.chronocrash.com
@rem -----------------------------------------------------------------------
@rem All rights reserved, see LICENSE in OpenBOR root for details.
@rem
@rem Copyright (c) 2004 - 2011 OpenBOR Team
@rem

@rem ----------------------- Bash NIX Shell Scripts ------------------------

@setlocal
@echo off
set BUILDBATCH=1
set TOOLS=../tools/bin;../tools/7-Zip;../tools/svn/bin
set PATH=%TOOLS%
bash.exe build.sh all
@endlocal

@rem ----------------------- Batch CMD Line Scripts ------------------------

@setlocal
@echo off
@if not exist "%CD%\releases\PSP\OpenBOR\EBOOT.PBP" goto missing
@if not exist "%CD%\releases\GP2X\OpenBOR\OpenBOR.gpe" goto missing
@if not exist "%CD%\releases\WIZ\OpenBOR\OpenBOR.gpe" goto missing
@if not exist "%CD%\releases\DC\OpenBOR\1ST_READ.BIN" goto missing
@if not exist "%CD%\releases\WINDOWS\OpenBOR\OpenBOR.exe" goto missing

set TOOLS=../tools/bin;../tools/7-Zip;../tools/svn/bin
set PATH=%TOOLS%;%PATH%
call xbox/make.bat
bash.exe version.sh 1
:missing
@endlocal

pause

