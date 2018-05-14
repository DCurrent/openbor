@rem
@rem OpenBOR - http://www.chronocrash.com
@rem -----------------------------------------------------------------------
@rem All rights reserved, see LICENSE in OpenBOR root for details.
@rem
@rem Copyright (c) 2004 - 2014 OpenBOR Team
@rem

@echo off
@echo -----------------------------------------------------------------------
@echo Building libraries from OpenBOR source...
@echo -----------------------------------------------------------------------
call ndk-build

if defined ANDROID_HOME (
	echo off
) else (
	if exist "copy_and_rename_me_to_local.properties" (
		copy /y "copy_and_rename_me_to_local.properties" "local.properties" 1>NUL
	)
)

@echo -----------------------------------------------------------------------
@echo Compiling APK ...
@echo -----------------------------------------------------------------------
call ant debug
@echo -----------------------------------------------------------------------
pause

