/*
 * XBoxMediaPlayer
 * Copyright (c) 2002 d7o3g4q and RUNTiME
 * Portions Copyright (c) by the authors of ffmpeg and xvid
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

// IoSupport.cpp: implementation of the CIoSupport class.
//
//////////////////////////////////////////////////////////////////////

#include "IoSupport.h"
#include "Undocumented.h"
#include <conio.h>
//#include "scsidefs.h"

#define CTLCODE(DeviceType, Function, Method, Access) ( ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method)  ) 
#define FSCTL_DISMOUNT_VOLUME  CTLCODE( FILE_DEVICE_FILE_SYSTEM, 0x08, METHOD_BUFFERED, FILE_ANY_ACCESS )


typedef struct 
{
	char szDriveLetter;
	char* szDevice;
} stDriveMapping;

stDriveMapping driveMapping[]=
{
	{ 'C', "Harddisk0\\Partition2"},
	{ 'D', "Cdrom0"},
	{ 'E', "Harddisk0\\Partition1"},
	{ 'F', "Harddisk0\\Partition6"},
	{ 'G', "Harddisk0\\Partition7"},
	{ 'X', "Harddisk0\\Partition3"},
	{ 'Y', "Harddisk0\\Partition4"},
	{ 'Z', "Harddisk0\\Partition5"},
};
#define NUM_OF_DRIVES ( sizeof( driveMapping) / sizeof( driveMapping[0] ) )


#ifdef __cplusplus
extern "C" {
#endif


void sprintfx( const char *fmt, ... ) ;

#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CIoSupport::CIoSupport()
{
	m_dwLastTrayState=0;

	m_gmXferBuffer = GlobalAlloc(GPTR, RAW_SECTOR_SIZE);
	m_rawXferBuffer = NULL;
	if( m_gmXferBuffer )
		m_rawXferBuffer = GlobalLock(m_gmXferBuffer);
}

CIoSupport::CIoSupport(CIoSupport& other)
{
	m_dwTrayState = other.m_dwTrayState;
	m_dwTrayCount = other.m_dwTrayCount;
	m_dwLastTrayState = other.m_dwLastTrayState;

	m_gmXferBuffer = GlobalAlloc(GPTR, RAW_SECTOR_SIZE);
	m_rawXferBuffer = NULL;
	if( m_gmXferBuffer )
		m_rawXferBuffer = GlobalLock(m_gmXferBuffer);
}

CIoSupport::~CIoSupport()
{
	if( m_gmXferBuffer )
	{
		GlobalUnlock(m_gmXferBuffer);
		GlobalFree(m_gmXferBuffer);
	}
}

// szDrive e.g. "D:"
// szDevice e.g. "Cdrom0" or "Harddisk0\Partition6"

HRESULT CIoSupport::Mount(const char* szDrive, char* szDevice)
{
	char szSourceDevice[256];
	char szDestinationDrive[16];

	sprintf(szSourceDevice,"\\Device\\%s",szDevice);
	sprintf(szDestinationDrive,"\\??\\%s",szDrive);

	STRING DeviceName =
	{
		strlen(szSourceDevice),
		strlen(szSourceDevice) + 1,
		szSourceDevice
	};

	STRING LinkName =
	{
		strlen(szDestinationDrive),
		strlen(szDestinationDrive) + 1,
		szDestinationDrive
	};

	IoCreateSymbolicLink(&LinkName, &DeviceName);

	return S_OK;
}



// szDrive e.g. "D:"

HRESULT CIoSupport::Unmount(const char* szDrive)
{
	char szDestinationDrive[16];
	sprintf(szDestinationDrive,"\\??\\%s",szDrive);

	STRING LinkName =
	{
		strlen(szDestinationDrive),
		strlen(szDestinationDrive) + 1,
		szDestinationDrive
	};

	IoDeleteSymbolicLink(&LinkName);
	
	return S_OK;
}





HRESULT CIoSupport::Remount(LPCSTR szDrive, LPSTR szDevice)
{
	char szSourceDevice[48];
	sprintf(szSourceDevice,"\\Device\\%s",szDevice);

	Unmount(szDrive);
	
	ANSI_STRING filename;
	OBJECT_ATTRIBUTES attributes;
	IO_STATUS_BLOCK status;
	HANDLE hDevice;
	NTSTATUS error;
	DWORD dummy;

	RtlInitAnsiString(&filename, szSourceDevice);
	InitializeObjectAttributes(&attributes, &filename, OBJ_CASE_INSENSITIVE, NULL);

	if (NT_SUCCESS(error = NtCreateFile(&hDevice, GENERIC_READ |
		SYNCHRONIZE | FILE_READ_ATTRIBUTES, &attributes, &status, NULL, 0,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_OPEN,
		FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT)))
	{

		if (!DeviceIoControl(hDevice, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &dummy, NULL))
		{
			CloseHandle(hDevice);
			return E_FAIL;
		}

		CloseHandle(hDevice);
	}
	
	Mount(szDrive,szDevice);

	return S_OK;
}

HRESULT CIoSupport::GetDeviceFromSymlink(LPCSTR szDrive, LPSTR szDevice)
{
	char szSourceDevice[48];

	ANSI_STRING filename;
	ANSI_STRING deviceName;
	OBJECT_ATTRIBUTES attributes;
	HANDLE hDevice;
	NTSTATUS error;
	DWORD dummy;

	sprintf(szSourceDevice,"\\??\\%s",szDrive);

	RtlInitAnsiString(&filename, szSourceDevice);
	InitializeObjectAttributes(&attributes, &filename, OBJ_CASE_INSENSITIVE, NULL);

	if (NT_SUCCESS(error = NtOpenSymbolicLinkObject(&hDevice, &attributes ) ) )
	{
		dummy = 50 ;

		//make sure there's enough space to get the \\device\harddisk0\... etc information
		//this is 200 spaces - should be enough - probably cleaner way to do this, but...
		RtlInitAnsiString(&deviceName, "                                                                                " ) ;

		if (NT_SUCCESS(error = NtQuerySymbolicLinkObject( hDevice, &deviceName, &dummy ) ) )
		{
			strcpy( szDevice, deviceName.Buffer ) ;
		}
		else
		{
			strcpy( szDevice, "noquery" ) ;
		}

		CloseHandle(hDevice);
	}
	else
	{
		strcpy( szDevice, "noopen" ) ;
	}

	return S_OK;
}

HRESULT CIoSupport::Remap(char* szMapping)
{
	char szMap[32];
	strcpy(szMap, szMapping );

	char* pComma = strstr(szMap,",");
	if (pComma)
	{
		*pComma = 0;
		
		// map device to drive letter
		Unmount(szMap);
		Mount(szMap,&pComma[1]);
		return S_OK;
	}

	return E_FAIL;
}


HRESULT CIoSupport::EjectTray()
{
	HalWriteSMBusValue(0x20, 0x0C, FALSE, 0);  // eject tray
	return S_OK;
}

HRESULT CIoSupport::CloseTray()
{
	HalWriteSMBusValue(0x20, 0x0C, FALSE, 1);  // close tray
	return S_OK;
}

DWORD CIoSupport::GetTrayState()
{
	HalReadSMCTrayState(&m_dwTrayState,&m_dwTrayCount);

	if(m_dwTrayState == TRAY_CLOSED_MEDIA_PRESENT) 
	{
		if (m_dwLastTrayState != TRAY_CLOSED_MEDIA_PRESENT)
		{
			m_dwLastTrayState = m_dwTrayState;
			return DRIVE_CLOSED_MEDIA_PRESENT;
		}
		else
		{
			return DRIVE_READY;
		}
	}
	else if(m_dwTrayState == TRAY_CLOSED_NO_MEDIA)
	{
		m_dwLastTrayState = m_dwTrayState;
		return DRIVE_CLOSED_NO_MEDIA;
	}
	else if(m_dwTrayState == TRAY_OPEN)
	{
		m_dwLastTrayState = m_dwTrayState;
		return DRIVE_OPEN;
	}
	else
	{
		m_dwLastTrayState = m_dwTrayState;
	}

	return DRIVE_NOT_READY;
}

HRESULT CIoSupport::Shutdown()
{
	// fails assertion on debug bios (symptom lockup unless running dr watson
	// so you can continue past the failed assertion).
	if (IsDebug())
		return E_FAIL;

		HalInitiateShutdown();

	return S_OK;
}


VOID CIoSupport::RemountDrive(LPCSTR szDrive)
{
	// ugly, but it works ;-)
	for (int i=0; i < NUM_OF_DRIVES; i++)
	{
		if (szDrive[0]== driveMapping[i].szDriveLetter)
		{
			Remount(szDrive, driveMapping[i].szDevice);
		}
	}
}

VOID CIoSupport::GetPartition(LPCSTR strFilename, LPSTR strPartition)
{
	strcpy(strPartition,"");
	for (int i=0; i < NUM_OF_DRIVES; i++)
	{
		if ( toupper(strFilename[0]) == driveMapping[i].szDriveLetter)
		{
			strcpy(strPartition, driveMapping[i].szDevice);
			return;
		}
	}
}

string CIoSupport::GetDrive(const string &szPartition)
{
	static string strDrive="E:";
	for (int i=0; i < NUM_OF_DRIVES; i++)
	{
		if ( !strcmp(driveMapping[i].szDevice,szPartition.c_str()))
		{
			char szDrive[3];
			szDrive[0]=driveMapping[i].szDriveLetter;
			szDrive[1]=0;
			strDrive=szDrive;
			return  strDrive;
		}
	}
	return  strDrive;
}

HANDLE CIoSupport::OpenCDROM()
{
	ANSI_STRING filename;
	OBJECT_ATTRIBUTES attributes;
	IO_STATUS_BLOCK status;
	HANDLE hDevice;
	NTSTATUS error;


	Remount("D:","Cdrom0");
	Unmount("D:");

	if( !m_rawXferBuffer )
		return NULL;

	RtlInitAnsiString(&filename,"\\Device\\Cdrom0");
	InitializeObjectAttributes(&attributes, &filename, OBJ_CASE_INSENSITIVE, NULL);
	if (!NT_SUCCESS(error = NtCreateFile(&hDevice, 
																			GENERIC_READ |SYNCHRONIZE | FILE_READ_ATTRIBUTES, 
																			&attributes, 
																			&status, 
																			NULL, 
																			0,
																			FILE_SHARE_READ,
																			FILE_OPEN,	
																			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT)))
	{
		return NULL;
	}
	return hDevice;
}
HANDLE CIoSupport::OpenCDROM2()
{
	ANSI_STRING filename;
	OBJECT_ATTRIBUTES attributes;
	IO_STATUS_BLOCK status;
	HANDLE hDevice;
	NTSTATUS error;


	if( !m_rawXferBuffer )
		return NULL;

	RtlInitAnsiString(&filename,"\\Device\\Cdrom0");
	InitializeObjectAttributes(&attributes, &filename, OBJ_CASE_INSENSITIVE, NULL);
	if (!NT_SUCCESS(error = NtCreateFile(&hDevice, 
																			GENERIC_READ |SYNCHRONIZE | FILE_READ_ATTRIBUTES, 
																			&attributes, 
																			&status, 
																			NULL, 
																			0,
																			FILE_SHARE_READ,
																			FILE_OPEN,	
																			FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT)))
	{
		return NULL;
	}
	return hDevice;
}

INT CIoSupport::ReadSector(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)
{
	DWORD dwRead;
	DWORD dwSectorSize = 2048;
	LARGE_INTEGER dwOffset;
	/*NTSTATUS error;
	IO_STATUS_BLOCK Iosb;
	dwOffset.QuadPart=dwSector*dwSectorSize;
	error = NtReadFile(hDevice,
										 NULL,
										 NULL,
										 NULL,
										 &Iosb,
										 lpczBuffer,
										 2048,
										 &dwOffset);
	if (NT_SUCCESS(error))
	{
		if(NT_SUCCESS(Iosb.Status) )
		{
				return 2048;
		}
	}
	DWORD iErr=GetLastError();
	char szErr[128];
	sprintf(szErr,"ReadSector(%i) returned %i\n", dwSector,iErr);
	OutputDebugString(szErr);
	*/

	dwOffset.QuadPart = ((LONGLONG)2048) * ((LONGLONG)dwSector);
	for (int i=0; i < 5; i++)
	{
		SetFilePointerEx(hDevice, dwOffset, NULL, FILE_BEGIN);

		if (ReadFile(hDevice, m_rawXferBuffer, dwSectorSize, &dwRead, NULL))
		{
			memcpy(lpczBuffer, m_rawXferBuffer, dwSectorSize);
			return dwRead;
		}
/*		else
		{
			int iSectorSize=XGetDiskSectorSize("D:");
			DWORD iErr=GetLastError();
			char szErr[128];
			sprintf(szErr,"ReadSector(%i) returned %i\n", dwSector,iErr);
			OutputDebugString(szErr);
		}*/
	}
	//OutputDebugString("CD Read error\n");
	return -1;
}

INT CIoSupport::ReadSomeSectors(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer, int numsectors)
{
	DWORD dwRead;
	DWORD dwSectorSize = 2048;
	LARGE_INTEGER dwOffset;
	/*NTSTATUS error;
	IO_STATUS_BLOCK Iosb;
	dwOffset.QuadPart=dwSector*dwSectorSize;
	error = NtReadFile(hDevice,
										 NULL,
										 NULL,
										 NULL,
										 &Iosb,
										 lpczBuffer,
										 2048,
										 &dwOffset);
	if (NT_SUCCESS(error))
	{
		if(NT_SUCCESS(Iosb.Status) )
		{
				return 2048;
		}
	}
	DWORD iErr=GetLastError();
	char szErr[128];
	sprintf(szErr,"ReadSector(%i) returned %i\n", dwSector,iErr);
	OutputDebugString(szErr);
	*/
	dwOffset.QuadPart = ((LONGLONG)2048) * ((LONGLONG)dwSector);
	for (int i=0; i < 5; i++)
	{
		SetFilePointerEx(hDevice, dwOffset, NULL, FILE_BEGIN);

		if (ReadFile(hDevice, lpczBuffer, dwSectorSize*numsectors, &dwRead, NULL))
		{
			return dwRead;
		}
/*		else
		{
			int iSectorSize=XGetDiskSectorSize("D:");
			DWORD iErr=GetLastError();
			char szErr[128];
			sprintf(szErr,"ReadSector(%i) returned %i\n", dwSector,iErr);
			OutputDebugString(szErr);
		}*/
	}
	//OutputDebugString("CD Read error\n");
	return -1;
}

INT CIoSupport::ReadSectorMode2(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)
{
	DWORD dwBytesReturned;
	RAW_READ_INFO rawRead;

	// Oddly enough, DiskOffset uses the Red Book sector size
	rawRead.DiskOffset.QuadPart = ((LONGLONG)2048) * ((LONGLONG)dwSector);
	rawRead.SectorCount = 1;
	rawRead.TrackMode = XAForm2;

	for (int i=0; i < 5; i++)
	{
		if( DeviceIoControl( hDevice,
			IOCTL_CDROM_RAW_READ,
			&rawRead,
			sizeof(RAW_READ_INFO),
			m_rawXferBuffer,
			sizeof(RAW_SECTOR_SIZE),
			&dwBytesReturned,
			NULL ) != 0 )
		{
			memcpy(lpczBuffer, (byte*)m_rawXferBuffer + MODE2_DATA_START, MODE2_DATA_SIZE);
			return MODE2_DATA_SIZE;
		}
	}
	return -1;
}

INT CIoSupport::ReadSectorCDDA(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer)
{
	DWORD dwBytesReturned;
	RAW_READ_INFO rawRead;



	// Oddly enough, DiskOffset uses the Red Book sector size
	rawRead.DiskOffset.QuadPart = ((LONGLONG)2048) * ((LONGLONG)dwSector);
	rawRead.SectorCount = 1;
	rawRead.TrackMode = CDDA;

	for (int i=0; i < 5; i++)
	{
		if( DeviceIoControl( hDevice,
			IOCTL_CDROM_RAW_READ,
			&rawRead,
			sizeof(RAW_READ_INFO),
			m_rawXferBuffer,
			sizeof(RAW_SECTOR_SIZE),
			&dwBytesReturned,
			NULL ) != 0 )
		{
			memcpy(lpczBuffer, m_rawXferBuffer, RAW_SECTOR_SIZE);
			return RAW_SECTOR_SIZE;
		}
	}
	return -1;
}

VOID CIoSupport::CloseCDROM(HANDLE hDevice)
{
	CloseHandle(hDevice);
}

VOID CIoSupport::UpdateDvdrom()
{
	OutputDebugString("Starting Dvdrom update.\n");
	BOOL bClosingTray = false;
	BOOL bShouldHaveClosed = false;

	// if the tray is open, close it
	DWORD dwCurrentState;
	do
	{
		dwCurrentState = GetTrayState();
		switch(dwCurrentState)
		{
			case DRIVE_OPEN:

				// drive is open		
				if (!bClosingTray)
				{
					bClosingTray = true;

					OutputDebugString("Drive open, closing tray...\n");
					CloseTray();
				}
				else if (bShouldHaveClosed)
				{
					// the operation failed, we cannot stay in this loop
					OutputDebugString("Dvdrom ended (failed to retract tray).\n");
					return;
				}

				break;
			case DRIVE_NOT_READY:
				// drive is not ready (closing, opening)
				OutputDebugString("Drive transition.\n");			
				bShouldHaveClosed = bClosingTray;
				Sleep(6000);
				break;
			case DRIVE_READY:
				// drive is ready
				OutputDebugString("Drive ready.\n");
				break;
			case DRIVE_CLOSED_NO_MEDIA:
				// nothing in there...
				OutputDebugString("Drive closed no media.\n");
				break;
			case DRIVE_CLOSED_MEDIA_PRESENT:
				// drive has been closed and is ready
				OutputDebugString("Drive closed media present, remounting...\n");
				Remount("D:","Cdrom0");
				break;
		}

	} while (dwCurrentState<DRIVE_READY);

	OutputDebugString("Dvdrom updated.\n");
}


// returns true if this is a debug machine
BOOL CIoSupport::IsDebug()
{
	LPDWORD pdwRegion = (LPDWORD) 0x8005E760;

	switch (*pdwRegion)
	{
		case 0x00400200:
		case 0x00800300:
		case 0x00400100:
			return TRUE;
	}

	return FALSE;
}


DWORD CIoSupport::IOCTLSendASPI32Command(HANDLE hIOCTL, LPSRB pSRB)
{
 LPSRB_ExecSCSICmd pSC;DWORD dwRet;BOOL bStat;
SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptIOCTL;         // global read bufs

 if(!pSRB) return SS_ERR;
    
 if(hIOCTL==NULL ||
    pSRB->SRB_Cmd!=SC_EXEC_SCSI_CMD)                   // we only fake exec aspi scsi commands
  {
   pSRB->SRB_Status=SS_ERR;
   return SS_ERR;
  }

 pSC=(LPSRB_ExecSCSICmd)pSRB;

 memset(&sptIOCTL,0,sizeof(sptIOCTL));

 sptIOCTL.spt.Length             = sizeof(SCSI_PASS_THROUGH_DIRECT);
 sptIOCTL.spt.CdbLength          = pSC->SRB_CDBLen;
 sptIOCTL.spt.DataTransferLength = pSC->SRB_BufLen;
 sptIOCTL.spt.TimeOutValue       = 60;
 sptIOCTL.spt.DataBuffer         = pSC->SRB_BufPointer;
 sptIOCTL.spt.SenseInfoLength    = 32;
 //sptIOCTL.spt.SenseInfoLength    = 14;
 sptIOCTL.spt.TargetId           = pSC->SRB_Target;
 sptIOCTL.spt.Lun                = pSC->SRB_Lun;
 sptIOCTL.spt.SenseInfoOffset    = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf);
 if(pSC->SRB_Flags&SRB_DIR_IN)       sptIOCTL.spt.DataIn = SCSI_IOCTL_DATA_IN;
 else if(pSC->SRB_Flags&SRB_DIR_OUT) sptIOCTL.spt.DataIn = SCSI_IOCTL_DATA_OUT;
 else                                sptIOCTL.spt.DataIn = SCSI_IOCTL_DATA_UNSPECIFIED;
 memcpy(sptIOCTL.spt.Cdb,pSC->CDBByte,pSC->SRB_CDBLen);

   bStat = DeviceIoControl(hIOCTL,
                IOCTL_SCSI_PASS_THROUGH_DIRECT,
                &sptIOCTL,
                sizeof(sptIOCTL),
                &sptIOCTL,
                sizeof(sptIOCTL),
                &dwRet,
                NULL);

 if(!bStat)                                            // some err?
  {
   DWORD dwErrCode;
   dwErrCode=GetLastError();
   if(dwErrCode==ERROR_IO_PENDING)                     // -> pending?
    {
     pSC->SRB_Status=SS_COMP;                          // --> ok
     return SS_PENDING;
    }
   pSC->SRB_Status=SS_ERR;                             // -> else error
   return SS_ERR;
  }

 pSC->SRB_Status=SS_COMP;
 return SS_COMP;
}


DWORD CIoSupport::GetSCSITOC(LPTOC_SCSI itoc, HANDLE hcdrom)
{
 SRB_ExecSCSICmd s;DWORD dwStatus;

 memset(&s,0,sizeof(s));

 s.SRB_Cmd        = SC_EXEC_SCSI_CMD;
 s.SRB_HaId       = 1;
 s.SRB_Target     = 0;
 s.SRB_Lun        = 0;
// s.SRB_HaId       = iCD_AD;
 //s.SRB_Target     = iCD_TA;
 //s.SRB_Lun        = iCD_LU;
 s.SRB_Flags      = SRB_DIR_IN | SRB_EVENT_NOTIFY;
 s.SRB_BufLen     = 0x324;
 s.SRB_BufPointer = (BYTE FAR *)itoc;
 s.SRB_SenseLen   = 0x0E;
 s.SRB_CDBLen     = 0x0A;
 s.SRB_PostProc   = 0;
 s.CDBByte[0]     = 0x43;
 s.CDBByte[1]     = 0x02; // 0x02 for MSF
 //s.CDBByte[1]     = 0x00; // 0x02 for MSF
 s.CDBByte[7]     = 0x03;
 s.CDBByte[8]     = 0x24;

 //ResetEvent(hEvent);
 dwStatus=IOCTLSendASPI32Command(hcdrom,(LPSRB)&s);

// if(dwStatus==SS_PENDING) WaitGenEvent(30000);

 if(s.SRB_Status!=SS_COMP) return SS_ERR;

 return SS_COMP;
}


DWORD CIoSupport::ReadXASector(HANDLE hIOCTL, unsigned int sectornum, unsigned int numsectors, unsigned char *secbuf)
{
	DWORD dwRet;
	BOOL bStat;
	SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptIOCTL;         // global read bufs
	BYTE        CDBByte[16];             // 30/048 SCSI CDB

	memset(&sptIOCTL,0,sizeof(sptIOCTL));

	sptIOCTL.spt.Length             = sizeof(SCSI_PASS_THROUGH_DIRECT);
	sptIOCTL.spt.CdbLength          = 12;
	sptIOCTL.spt.DataTransferLength = 2352*numsectors;
	//sptIOCTL.spt.DataTransferLength = 2048*numsectors;
	sptIOCTL.spt.TimeOutValue       = 999999;
	sptIOCTL.spt.DataBuffer         = secbuf;
	sptIOCTL.spt.SenseInfoLength    = 64;
	sptIOCTL.spt.TargetId           = 0;
	sptIOCTL.spt.Lun                = 0;
	sptIOCTL.spt.SenseInfoOffset    = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf);
	sptIOCTL.spt.DataIn = SCSI_IOCTL_DATA_IN;

	memset(CDBByte, 0, 16 ) ;

	CDBByte[0]     = 0xBE;
	CDBByte[1]     = 0x00;
	CDBByte[3]     = (unsigned char)((sectornum >> 16) & 0xFF);
	CDBByte[4]     = (unsigned char)((sectornum >> 8) & 0xFF);
	CDBByte[5]     = (unsigned char)(sectornum & 0xFF);
	//CDBByte[7]     = (unsigned char)((numsectors >> 8) & 0xFF);
	CDBByte[8]     = (unsigned char)(numsectors & 0xFF);
	CDBByte[9]     = 0xF8;//F0!!!!!!!!!!!  //1111 1000
	//CDBByte[9]     = 0x10;  //flags byte - 0x10 means only return user data portion

	memcpy(sptIOCTL.spt.Cdb,CDBByte,12);

	bStat = DeviceIoControl(hIOCTL,
                IOCTL_SCSI_PASS_THROUGH_DIRECT,
                &sptIOCTL,
                sizeof(sptIOCTL),
                &sptIOCTL,
                sizeof(sptIOCTL),
                &dwRet,
                NULL);

	if(!bStat)                                            // some err?
	{
		return -1 ;
	}

	return dwRet;
}



BOOL HDD_SendATACommand(WORD IDEPort, ATA_COMMAND_OBJ *ATACommandObj, unsigned char ReadWrite)
{
	BOOL retVal = FALSE;
	unsigned char waitcount = 10;
	WORD inVal=0;
	WORD SuccessRet = 0x58;

	DWORD *PIDEDATA = (LPDWORD) &ATACommandObj->DATA_BUFFER ;
		
	//Write IDE Registers to IDE Port.. and in essence Execute the ATA Command..
	_outp(IDEPort + 1, ATACommandObj->IPReg.bFeaturesReg);
//	Sleep(15);
	_outp(IDEPort + 2, ATACommandObj->IPReg.bSectorCountReg);
	//Sleep(15);
	_outp(IDEPort + 3, ATACommandObj->IPReg.bSectorNumberReg);
	//Sleep(15);
	_outp(IDEPort + 4, ATACommandObj->IPReg.bCylLowReg);
	//Sleep(15);
	_outp(IDEPort + 5, ATACommandObj->IPReg.bCylHighReg);
	//Sleep(15);
	_outp(IDEPort + 6, ATACommandObj->IPReg.bDriveHeadReg);
	//Sleep(15);
	_outp(IDEPort + 7, ATACommandObj->IPReg.bCommandReg);
	//Sleep(200);

	//Command Executed, Check Status.. If not success, wait a while..
	inVal = _inp(IDEPort+7); 
	while (((inVal & SuccessRet) != SuccessRet) && (waitcount > 0))
	{
		inVal = _inp(IDEPort+7); //Check Status..
		Sleep(30);
		waitcount--;
	}


	//IF Waitcount reaches 0 it means a TimeOut occured while waiting for command to complete
	//This will return FALSE...


	//Is this a IDE command that Requests Data, if so, Read the from IDE port ...
	if ((waitcount > 0) && (ReadWrite == IDE_COMMAND_READ))
	{
		//Read the command return output Registers
		ATACommandObj->OPReg.bErrorReg =		_inp(IDEPort + 1);
		ATACommandObj->OPReg.bSectorCountReg =	_inp(IDEPort + 2);
		ATACommandObj->OPReg.bSectorNumberReg =	_inp(IDEPort + 3);
		ATACommandObj->OPReg.bCylLowReg =		_inp(IDEPort + 4);
		ATACommandObj->OPReg.bCylHighReg =		_inp(IDEPort + 5);
		ATACommandObj->OPReg.bDriveHeadReg =	_inp(IDEPort + 6);
		ATACommandObj->OPReg.bStatusReg =		_inp(IDEPort + 7);

		ATACommandObj->DATA_BUFFSIZE = 512;
		//Sleep(100);

		//Now read a sector (512 Bytes) from the IDE Port
		ZeroMemory(ATACommandObj->DATA_BUFFER, 512);
		for (int i = 0; i < 128; i++)
		{
			PIDEDATA[i] = _inpd(IDEPort);
			//Sleep(30);
		}

		retVal = TRUE;
	}

	//Is this a IDE command that Sends Data, if so, write the Data to IDE Port..
	if ((waitcount > 0) && (ATACommandObj->DATA_BUFFSIZE > 0) && (ReadWrite == IDE_COMMAND_WRITE))
	{
		//Read the command return output Registers
		ATACommandObj->OPReg.bErrorReg =		_inp(IDEPort + 1);
		ATACommandObj->OPReg.bSectorCountReg =	_inp(IDEPort + 2);
		ATACommandObj->OPReg.bSectorNumberReg =	_inp(IDEPort + 3);
		ATACommandObj->OPReg.bCylLowReg =		_inp(IDEPort + 4);
		ATACommandObj->OPReg.bCylHighReg =		_inp(IDEPort + 5);
		ATACommandObj->OPReg.bDriveHeadReg  =	_inp(IDEPort + 6);
		ATACommandObj->OPReg.bStatusReg =		_inp(IDEPort + 7);

		//Sleep(50);

		//Now Write a sector (512 Bytes) To the IDE Port
		for (int i = 0; i <  128; i++)
		{
			_outpd(IDEPort, PIDEDATA[i]);
				//Sleep(30);

		}
		retVal = TRUE;
	}

	return retVal;
}
/*
void C_HDD::GetIDEModel(unsigned char* IDEData, char *ModelString, DWORD *StrLen)
{
	unsigned char m_length = 0x28;

	m_length = HDD_CleanATAData((unsigned char*)ModelString, IDEData+HDD_MODEL_OFFSET, m_length);
	*StrLen = m_length;

}

C_HDD::C_HDD()
{	
	DWORD len;
	int t;
	unsigned char temp[64];

	for( t=0;t<10;++t )
	{
		memset(&ip_ide_reg,0, sizeof(ip_ide_reg));
 		memset(&ata_command,0, sizeof(ata_command));
		memset(&ata_ide_reg,0, sizeof(ata_ide_reg));
		ata_command.IPReg.bDriveHeadReg = 0xA0;
		ata_command.IPReg.bCommandReg = 0xEC;

		Sleep(100);

		HDD_SendATACommand( IDE_PRIMARY_PORT, &ata_command, IDE_COMMAND_READ);
		memcpy(&PrimaryData,ata_command.DATA_BUFFER,512);
		
		GetIDEModel( ata_command.DATA_BUFFER, Primary, &len );
		Primary[len] = 0;

		OutputDebugString(Primary);
		OutputDebugString("\r\n");
		if( ( PrimaryData.NumHds > 32) || ( PrimaryData.NumHds <= 0 ) )
			continue;
		else
		{
			HDD_CleanATAData(temp, (unsigned char*)PrimaryData.FirmRev, 8);
			memcpy(&PrimaryData.FirmRev, temp, 8 );
			break;
		}
	}
	for( t=0;t<10;++t )
	{
		memset(&ip_ide_reg,0, sizeof(ip_ide_reg));
		memset(&ata_command,0, sizeof(ata_command));
		memset(&ata_ide_reg,0, sizeof(ata_ide_reg));
		ata_command.IPReg.bDriveHeadReg = 0xB0;
		ata_command.IPReg.bCommandReg = 0xA1;

		Sleep(100);
		HDD_SendATACommand( IDE_PRIMARY_PORT, &ata_command, IDE_COMMAND_READ);
		memcpy(&SecondaryData,ata_command.DATA_BUFFER,512);
		GetIDEModel(  ata_command.DATA_BUFFER, Secondary, &len );
		Secondary[len] = 0;
		if( SecondaryData.Extension == 6 )
		{
			t = 10;
			break;
		}
		if( (strlen(Secondary) > 32 )  )
		{
			t--;
			memset(Secondary, 0, 512 );
		}
	}
	if ( !strlen(Secondary) )
	{
		for( t=0;t<10;++t )
		{
			memset(&ip_ide_reg,0, sizeof(ip_ide_reg));
			memset(&ata_command,0, sizeof(ata_command));
			memset(&ata_ide_reg,0, sizeof(ata_ide_reg));
			ata_command.IPReg.bDriveHeadReg = 0xB0;
			ata_command.IPReg.bCommandReg = 0xEC;

			Sleep(100);
			HDD_SendATACommand( IDE_PRIMARY_PORT, &ata_command, IDE_COMMAND_READ);
			memcpy(&SecondaryData,ata_command.DATA_BUFFER,512);
			GetIDEModel(  ata_command.DATA_BUFFER, Secondary, &len );
			Secondary[len] = 0;
			if( ( SecondaryData.NumHds > 32) || ( SecondaryData.NumHds <= 0 ) )
				continue;
			else
			{
				break;			
			}
		}
		memcpy(&SecondaryData,ata_command.DATA_BUFFER,512);
	}		
	HDD_CleanATAData(temp, (unsigned char *)SecondaryData.FirmRev, 8);
	memcpy(&SecondaryData.FirmRev, temp, 8 );
}
*/



DWORD CIoSupport::ReadXASector2(unsigned int sectornum, unsigned int numsectors, unsigned char *secbuf)
{
	WORD IDEPort ;
	BOOL retVal = FALSE;
	unsigned char waitcount = 10;
	WORD inVal=0;
	WORD SuccessRet = 0x08;

	struct IP_IDE_REG		ip_ide_reg;
	struct OP_IDE_REG		ata_ide_reg;
	struct ATA_COMMAND_OBJ	ata_command;

	struct ATAPI_PACKET atapi_packet ;
	BYTE CDBByte[16] ;


	memset( &atapi_packet, 0, sizeof(atapi_packet) ) ;
	memset(&ip_ide_reg,0, sizeof(ip_ide_reg));
	memset(&ata_command,0, sizeof(ata_command));
	memset(&ata_ide_reg,0, sizeof(ata_ide_reg));
	//ata_command.IPReg.bCommandReg = 0xEC;  // IDENTIFY
	ata_command.IPReg.bCommandReg = 0xA0;  // ATAPI_PACKET
	ata_command.IPReg.bSectorCountReg  = 0x01;
	ata_command.IPReg.bSectorNumberReg = ( sectornum & 0xFF );
	ata_command.IPReg.bCylLowReg       = ( ( sectornum>>8 ) & 0xFF ) ;
	ata_command.IPReg.bCylHighReg      = ( ( sectornum>>16) & 0xFF ) ;
	ata_command.IPReg.bDriveHeadReg = 0xB0;  //bit 4 = DEVICE  ( 7 6 5 4 3 2 1 0 )  ( 1 LBA 1 D H H H H )
	

	IDEPort = IDE_PRIMARY_PORT ;


	//Write IDE Registers to IDE Port.. and in essence Execute the ATA Command..
	_outp(IDEPort + 1, 0 ) ;  //ATACommandObj->IPReg.bFeaturesReg);
	_outp(IDEPort + 2, 0 ) ; //ATACommandObj->IPReg.bSectorCountReg);
	_outp(IDEPort + 3, 0 ) ; //ATACommandObj->IPReg.bSectorNumberReg);
	_outp(IDEPort + 4, 0x30 ) ; //ATACommandObj->IPReg.bCylLowReg);
	_outp(IDEPort + 5, 0x09 ) ; //ATACommandObj->IPReg.bCylHighReg);
	  //0x930 = 2352 = size of sector

	_outp(IDEPort + 6, 0xB0 ) ; //ATACommandObj->IPReg.bDriveHeadReg);
	  //0xA0 - hopefully means the CDROM device on the IDEPort to be used

	_outp(IDEPort + 7, 0xA0 ) ; //ATACommandObj->IPReg.bCommandReg);
	//0xA0 command is ATAPI PACKET


	//atapi_packet.bOperationCode = 0xBE ;
	//atapi_packet.bAddress1 
	//atapi_packet.bAddress2 
	//atapi_packet.bAddress3 
	//atapi_packet.bAddress4 
	//atapi_packet.bTranferLength1 
	//atapi_packet.bTranferLength2 
		
	memset(CDBByte, 0, 16 ) ;

	CDBByte[0]     = 0xBE;
	CDBByte[1]     = 0x00;
	CDBByte[3]     = (unsigned char)((sectornum >> 16) & 0xFF);
	CDBByte[4]     = (unsigned char)((sectornum >> 8) & 0xFF);
	CDBByte[5]     = (unsigned char)(sectornum & 0xFF);
	//CDBByte[7]     = (unsigned char)((numsectors >> 8) & 0xFF);
	CDBByte[8]     = (unsigned char)(numsectors & 0xFF);
	CDBByte[9]     = 0xF8;//F0!!!!!!!!!!!  //1111 1000
	//CDBByte[9]     = 0x10;  //flags byte - 0x10 means only return user data portion

	Sleep(200) ;
	//Command Executed, Check Status.. If not success, wait a while..
	inVal = _inp(IDEPort+7); 
	while (((inVal & SuccessRet) != SuccessRet) && (waitcount > 0))
	{
		inVal = _inp(IDEPort+7); //Check Status..
		Sleep(30);
		waitcount--;
	}

	//if (ide_wait_stat (drive, DRQ_STAT, BUSY_STAT, WAIT_READY))
			//return 1;

	/* Arm the interrupt handler. */
	//ide_set_handler (drive, handler, WAIT_CMD);

	/* Send the command to the device. */

	//++bytecount;
	//ide_output_data (drive, buffer, bytecount / 4);

	for ( int i = 0 ; i < 12 ; i++ )
		_outp(IDEPort + 0, CDBByte[i] ) ;  

	Sleep(200) ;


	for ( int i = 0 ; i < 2352 ; i++ )
		*(secbuf+i) = _inp(IDEPort + 0 ) ;  

	//if ((bytecount & 0x03) >= 2) {
		//_outsw (IDEPort,
		       //((byte *)buffer) + (bytecount & ~0x03), 1);
	//}
	
	
	


	//Sleep(100);
	//HDD_SendATACommand( IDE_PRIMARY_PORT, &ata_command, IDE_COMMAND_READ);
	//memcpy(secbuf,ata_command.DATA_BUFFER,512);

	return 1 ;
}

//http://www.angelfire.com/de2/zel/images/ata3r6.doc
