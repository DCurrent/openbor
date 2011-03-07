/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#ifndef STACKTRACE_H
#define STACKTRACE_H

/* GNU libc platforms: Linux and Pandora.  GP2X, Wiz, Darwin, and Dingoo are 
 * also Unix-like platforms, but the stack trace hasn't been tested on these 
 * platforms.  The code depends on GNU extensions which might or might not be 
 * available on those platforms. It's currently not enabled for Pandora, either,
 * since the code currently assumes it is running on x86. */

#ifdef CUSTOM_SIGNAL_HANDLER
#undef kill
#include <signal.h>

// Prints a stack trace on *nix platforms if the engine crashes with SIGSEGV.
void handleFatalSignal(int sig_num, siginfo_t * info, void * ucontext);
#endif

#endif

