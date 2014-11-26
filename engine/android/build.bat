@rem
@rem OpenBOR - http://www.chronocrash.com
@rem -----------------------------------------------------------------------
@rem All rights reserved, see LICENSE in OpenBOR root for details.
@rem
@rem Copyright (c) 2004 - 2011 OpenBOR Team
@rem

@rem Building libraries from OpenBOR source...
@rem
call ndk-build
@rem Compiling APK ...
@rem
call ant debug
pause

