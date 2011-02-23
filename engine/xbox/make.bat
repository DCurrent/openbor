@SETLOCAL

@if not exist "C:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\bin\vcvars32.bat" goto missing

@echo ------------------------------------------------------
@echo                 XBOX Environment Loaded!
@echo ------------------------------------------------------
@echo.
@echo.                                                     

@call "C:\Program Files\Microsoft Visual Studio .NET 2003\Vc7\bin\vcvars32.bat"
@devenv C:\Projects\OpenBOR\xbox\openbor.sln /clean Release /project C:\Projects\OpenBOR\xbox\OpenBOR.vcproj
@devenv C:\Projects\OpenBOR\xbox\openbor.sln /build Release /project C:\Projects\OpenBOR\xbox\OpenBOR.vcproj

@mkdir C:\Projects\OpenBOR\releases\XBOX
@mkdir C:\Projects\OpenBOR\releases\XBOX\OpenBOR
@mkdir C:\Projects\OpenBOR\releases\XBOX\OpenBOR\Logs
@mkdir C:\Projects\OpenBOR\releases\XBOX\OpenBOR\Paks
@mkdir C:\Projects\OpenBOR\releases\XBOX\OpenBOR\Saves
@mkdir C:\Projects\OpenBOR\releases\XBOX\OpenBOR\ScreenShots

@copy C:\Projects\OpenBOR\xbox\data\menu.pak C:\Projects\OpenBOR\releases\XBOX\OpenBOR\Paks\
@copy C:\Projects\OpenBOR\xbox\Release\default.xbe C:\Projects\OpenBOR\releases\XBOX\OpenBOR\

@devenv C:\Projects\OpenBOR\xbox\openbor.sln /clean Release /project C:\Projects\OpenBOR\xbox\OpenBOR.vcproj

@ENDLOCAL
@echo.
@echo.
@goto :eof

:missing
@echo.
@echo.
@ENDLOCAL
