@rem
@rem OpenBOR - http://www.LavaLit.com
@rem -----------------------------------------------------------------------
@rem Licensed under the BSD license, see LICENSE in OpenBOR root for details.
@rem
@rem Copyright (c) 2004 - 2011 OpenBOR Team
@rem

@SETLOCAL

@if not exist "C:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\bin\vcvars32.bat" goto missing

@echo ------------------------------------------------------
@echo             Windows VC7 Environment Loaded!
@echo ------------------------------------------------------
@echo.                                                     

set DIR=%CD%
@if not exist "%DIR%\OpenBOR.vcproj" (set DIR=%CD%\mvs\vc7)

@call "C:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\bin\vcvars32.bat"
@devenv "%DIR%\openbor.sln" /clean Release /project "%DIR%\OpenBOR.vcproj"
@devenv "%DIR%\openbor.sln" /build Release /project "%DIR%\OpenBOR.vcproj"

@mkdir "%DIR%\..\..\releases\WINDOWS"
@mkdir "%DIR%\..\..\releases\WINDOWS\OpenBOR"
@mkdir "%DIR%\..\..\releases\WINDOWS\OpenBOR\Logs"
@mkdir "%DIR%\..\..\releases\WINDOWS\OpenBOR\Paks"
@mkdir "%DIR%\..\..\releases\WINDOWS\OpenBOR\Saves"
@mkdir "%DIR%\..\..\releases\WINDOWS\OpenBOR\ScreenShots"

@copy "%DIR%\Release\OpenBOR.exe" "%DIR%\..\..\releases\WINDOWS\OpenBOR\"
@copy C:\WINDOWS\system32\msvcr71.dll "%DIR%\..\..\releases\WINDOWS\OpenBOR\"

@devenv "%DIR%\openbor.sln" /clean Release /project "%DIR%\OpenBOR.vcproj"

@ENDLOCAL
@echo.
@goto :eof

:missing
@echo.
@ENDLOCAL
