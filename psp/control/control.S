	.set noreorder

#include "pspstub.s"

	STUB_START "control",0x40090000,0x00010005
	STUB_FUNC  0xF09545E7,getCtrlData
	STUB_END
