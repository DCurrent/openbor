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
@echo -----------------------------------------------------------------------
@echo Compiling APK ...
@echo -----------------------------------------------------------------------
call ant debug
@echo -----------------------------------------------------------------------
pause

