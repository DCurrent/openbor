/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) OpenBOR Team
 */

#ifndef BIOS_H
#define BIOS_H

int  bios_gdGdcReqCmd(int cmd, void *param);
void bios_gdGdcExecServer(void);
int  bios_gdGdcGetCmdStat(int f, int *status);
int  bios_gdGdcGetDrvStat(unsigned int *param);
int  bios_gdGdcChangeDataType(unsigned int *param);
void bios_gdGdcInitSystem(void);
void bios_gdGdcReset(void);

#endif
