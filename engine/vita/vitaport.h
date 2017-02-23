/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under a BSD-style license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2017 OpenBOR Team
 */

#ifndef VITAPORT_H_
#define VITAPORT_H_

void borExit(int reset);
void openborMain(int argc, char** argv);

extern char packfile[128];
extern char paksDir[128];
extern char savesDir[128];
extern char logsDir[128];
extern char screenShotsDir[128];
extern char rootDir[128];

#endif
