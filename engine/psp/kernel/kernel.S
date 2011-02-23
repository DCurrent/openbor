//
// OpenBOR - http://www.LavaLit.com
// -----------------------------------------------------------------------
// Licensed under the BSD license, see LICENSE in OpenBOR root for details.
//
// Copyright (c) 2004 - 2011 OpenBOR Team
//


	.set noreorder

#include "pspstub.s"

	STUB_START "kernel",0x40090000,0x00030005
	STUB_FUNC  0x84607032,loadexec
	STUB_FUNC  0x4F764650,getDevkitVersion
	STUB_FUNC  0xF1E47173,getHardwareModel
	STUB_END
