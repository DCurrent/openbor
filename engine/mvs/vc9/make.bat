@rem
@rem OpenBOR - http://www.LavaLit.com
@rem -----------------------------------------------------------------------
@rem Licensed under the BSD license, see LICENSE in OpenBOR root for details.
@rem
@rem Copyright (c) 2004 - 2011 OpenBOR Team
@rem

@SETLOCAL

@if not exist "C:\Program Files\Microsoft Visual Studio 9.0\VC\vcvarsall.bat" goto missing

@echo ------------------------------------------------------
@echo             Windows VC9 Environment Loaded!
@echo ------------------------------------------------------
@echo.                                                     

set DIR=%CD%
@if not exist "%DIR%\OpenBOR.vcproj" (set DIR=%CD%\mvs\vc9)

@call "C:\Program Files\Microsoft Visual Studio 9.0\VC\vcvarsall.bat"
@vcbuild /clean "%DIR%\OpenBOR.vcproj" Release
@vcbuild "%DIR%\OpenBOR.vcproj" Release

@mkdir "%DIR%\..\..\releases\WINDOWS"
@mkdir "%DIR%\..\..\releases\WINDOWS\OpenBOR"
@mkdir "%DIR%\..\..\releases\WINDOWS\OpenBOR\Logs"
@mkdir "%DIR%\..\..\releases\WINDOWS\OpenBOR\Paks"
@mkdir "%DIR%\..\..\releases\WINDOWS\OpenBOR\Saves"
@mkdir "%DIR%\..\..\releases\WINDOWS\OpenBOR\ScreenShots"

@copy "%DIR%\Release\OpenBOR.exe" "%DIR%\..\..\releases\WINDOWS\OpenBOR\"
@copy C:\WINDOWS\system32\msvcr71.dll "%DIR%\..\..\releases\WINDOWS\OpenBOR\"

@vcbuild /clean "%DIR%\OpenBOR.vcproj" Release

@ENDLOCAL
@echo.
@goto :eof

:missing
@echo.
@ENDLOCAL
