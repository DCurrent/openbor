/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

  .globl  _bios_gdGdcReqCmd, _bios_gdGdcGetCmdStat, _bios_gdGdcExecServer
  .globl  _bios_gdGdcInitSystem, _bios_gdGdcGetDrvStat, _bios_gdGdcG1DmaEnd
  .globl  _bios_gdGdcReqDmaTrans, _bios_gdGdcCheckDmaTrans, _bios_gdGdcReadAbort
  .globl  _bios_gdGdcReset, _bios_gdGdcChangeDataType

  .text

_bios_gdGdcReqCmd:
  bra do_syscall
  mov #0,r7

_bios_gdGdcGetCmdStat:
  bra do_syscall
  mov #1,r7

_bios_gdGdcExecServer:
  bra do_syscall
  mov #2,r7

_bios_gdGdcInitSystem:
  bra do_syscall
  mov #3,r7

_bios_gdGdcGetDrvStat:
  bra do_syscall
  mov #4,r7

_bios_gdGdcG1DmaEnd:
  bra do_syscall
  mov #5,r7

_bios_gdGdcReqDmaTrans:
  bra do_syscall
  mov #6,r7

_bios_gdGdcCheckDmaTrans:
  bra do_syscall
  mov #7,r7

_bios_gdGdcReadAbort:
  bra do_syscall
  mov #8,r7

_bios_gdGdcReset:
  bra do_syscall
  mov #9,r7

_bios_gdGdcChangeDataType:
  mov #10,r7

do_syscall:
  mov.l sysvec_bc,r0
  mov #0,r6
  mov.l @r0,r0
  jmp @r0
  nop

  .align 2,0
sysvec_bc:
  .long 0x8C0000BC

  .end
