@rem
@rem OpenBOR - https://www.chronocrash.com
@rem -----------------------------------------------------------------------
@rem Licensed under the BSD license, see LICENSE in OpenBOR root for details.
@rem
@rem Copyright (c)  OpenBOR Team
@rem

@rem ----------------------- Bash NIX Shell Scripts ------------------------

@setlocal
@echo off
set BUILDBATCH=1
set TOOLS=../../bin;../../win-sdk/bin
set PATH=%TOOLS%;%PATH%
bash.exe build.sh win
@endlocal