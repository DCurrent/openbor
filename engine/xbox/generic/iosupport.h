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

// IoSupport.h: interface for the CIoSupport class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IOSUPPORT_H__F084A488_BD6E_49D5_8CD3_0BE62149DB40__INCLUDED_)
#define AFX_IOSUPPORT_H__F084A488_BD6E_49D5_8CD3_0BE62149DB40__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <xtl.h>

#define TRAY_OPEN					16
#define TRAY_CLOSED_NO_MEDIA		64
#define TRAY_CLOSED_MEDIA_PRESENT	96

#define DRIVE_OPEN						0 // Open...
#define DRIVE_NOT_READY					1 // Opening.. Closing... 
#define DRIVE_READY						2  
#define DRIVE_CLOSED_NO_MEDIA			3 // CLOSED...but no media in drive
#define DRIVE_CLOSED_MEDIA_PRESENT		4 // Will be send once when the drive just have closed

#define MODE2_DATA_START			24  	// Mode2 raw sector has 24 bytes before the data payload
#define MODE2_DATA_SIZE				2324	// And has 2324 usable bytes
#define RAW_SECTOR_SIZE				2352	// Raw sector size

//Important ATA IDENTIFY Structure offsets..
//As per ATA Spec
#define HDD_SERIAL_OFFSET				0x014
#define HDD_MODEL_OFFSET				0x036
#define HDD_SECURITY_STATUS_OFFSET		0x100

//IDE Port Addresses
#define IDE_PRIMARY_PORT				0x01F0
#define IDE_SECONDARY_PORT				0x0170

//Important ATA Register Values
//As per ATA Spec
#define IDE_DEVICE_MASTER				0x00A0
#define IDE_DEVICE_SLAVE				0x00B0

//Important ATA/ATAPI Commands
//As per ATA Spec
#define IDE_ATAPI_IDENTIFY				0xA1  
#define IDE_ATA_IDENTIFY				0xEC  

#define	IDE_ATA_SECURITY_SETPASSWORD	0xF1
#define IDE_ATA_SECURITY_UNLOCK			0xF2
#define	IDE_ATA_SECURITY_FREEZE			0xF5
#define	IDE_ATA_SECURITY_DISABLE		0xF6 

//Important ATA IDENTIFY Data Structure values
//As per ATA Spec
#define IDE_SECURITY_SUPPORTED			0x0001
#define IDE_SECURITY_ENABLED			0x0002
#define IDE_SECURITY_PASSWORD_SET		0x0004
#define IDE_SECURITY_FROZEN				0x0008
#define IDE_SECURITY_COUNT_EXPIRED		0x0010
#define IDE_SECURITY_LEVEL_MAX			0x0100

//Important ATA Command return register values
//As per ATA Spec
#define IDE_ERROR_SUCCESS				0x0000
#define IDE_ERROR_ABORT					0x0004


//Our SendATACommand needs this to figure our if we should 
//read or write data to IDE registers..
#define	IDE_COMMAND_READ				0x00
#define	IDE_COMMAND_WRITE				0x01

struct ATAPI_PACKET
{
	BYTE bOperationCode ;
	BYTE bReserved1 ;
	BYTE bAddress1 ;
	BYTE bAddress2 ;
	BYTE bAddress3 ;
	BYTE bAddress4 ;
	BYTE bReserved2 ;
	BYTE bTranferLength1 ;
	BYTE bTranferLength2 ;
	BYTE bReserved3 ;
	BYTE bReserved4 ;
	BYTE bReserved5 ;
};

/*
The host waits until BSY and DRQ are 0 and subsequently initializes the ATAPI task file. 
Then it writes the ATAPI PACKET opcode (A0h) into the command register. 

  The device sets BSY and prepares to accept the command packet proper. 
  When it is ready it sets COD and cancels IO. Then it sets DRQ and cancels BSY. 

  As soon as it sees DRQ, the host writes the 12 command bytes into the data register. 
  After having received the 12th byte the device cancels DRQ, sets BSY and reads the 
  features and the byte count from the task file. 

  Let us now assume that we are dealing with a command packet which entails a data transfer 
  to the host. The device executes the command and prepares for the data transfer. 

  The device loads the byte count register, sets IO and cancels COD, sets DRQ and cancels BSY, 
  and finally sets INTRO. 

  As soon as the host sees DRQ, it reads the status register. As a reaction, the device 
  cancels INTRO. The host reads the data register as many times as specified in the byte 
  count register. When all data are read the device negates DRQ. 

  The device writes the final status into the status register, sets COD, IO, and DRDY and 
  cancels BSY and DRQ. Finally it sets INTRQ. 

  This is the signal for the host to read the final status and, if necessary, 
  the error register [3]. 
*/

//http://216.239.33.100/search?q=cache:TGJT7HXOmfUJ:www.pjrc.com/tech/mp3/gallery/cs580/ata_atapi.html+%22atapi+packet%22+ata&hl=en&ie=UTF-8
//http://akrip.sourceforge.net/8020r26.pdf

//IDE ATA Input Registers Structure
struct IP_IDE_REG
{
	BYTE bFeaturesReg;
	BYTE bSectorCountReg;
	BYTE bSectorNumberReg;
	BYTE bCylLowReg;
	BYTE bCylHighReg;
	BYTE bDriveHeadReg;
	BYTE bCommandReg;
};

//IDE ATA Output Registers Structure
struct OP_IDE_REG
{
	BYTE bErrorReg;
	BYTE bSectorCountReg;
	BYTE bSectorNumberReg;
	BYTE bCylLowReg;
	BYTE bCylHighReg;
	BYTE bDriveHeadReg;
	BYTE bStatusReg;
};

//Our own object for issuing commands..
//Includes in/ou register objects and 1 Sector of HDD Data
struct ATA_COMMAND_OBJ
{
	IP_IDE_REG	IPReg;
	OP_IDE_REG	OPReg;
	BYTE		DATA_BUFFER[512];
	ULONG		DATA_BUFFSIZE;
};


//Enum for Devices on SMBus
enum SMBUS_DEVICES
{
	SMBDEV_PIC16L = 0x20,
	SMBDEV_VIDEO_ENCODER = 0x8a,
	SMBDEV_TEMP_MONITOR = 0x98,
	SMBDEV_TEMP_MONITOR_READ = 0x99,
	SMBDEV_EEPROM = 0xA8

};

//Commands that can be sent to the PIC
enum PIC16L_CMD
{
	PIC16L_CMD_POWER = 0x02,
	PIC16L_CMD_LED_MODE = 0x07,
	PIC16L_CMD_LED_REGISTER = 0x08,
	PIC16L_CMD_EJECT = 0x0C,
	PIC16L_CMD_INTERRUPT_REASON = 0x11,
	PIC16L_CMD_RESET_ON_EJECT = 0x19,
	PIC16L_CMD_SCRATCH_REGISTER = 0x1B

};



#include <string>
using namespace std;

#include "wnaspi32.h"

#define PACKED

#pragma pack(1)
typedef struct
{
  BYTE rsvd;
  BYTE ADR;
  BYTE trackNumber;
  BYTE rsvd2;
  BYTE addr[4];
}
PACKED TOCTRACK_SCSI;


typedef struct
{
  WORD tocLen;
  BYTE firstTrack;
  BYTE lastTrack;
  TOCTRACK_SCSI tracks[100];
}
PACKED TOC_SCSI, *PTOC_SCSI, FAR * LPTOC_SCSI;

#pragma pack()

class CIoSupport  
{
public:

	CIoSupport();
	CIoSupport(CIoSupport& other);
	virtual ~CIoSupport();

	HRESULT Mount(const char* szDrive, char* szDevice);
	HRESULT Unmount(const char* szDrive);

	HRESULT GetDeviceFromSymlink(LPCSTR szDrive, LPSTR szDevice);
	HRESULT Remount(const char* szDrive, char* szDevice);
	HRESULT Remap(char* szMapping);

	DWORD	GetTrayState();
	HRESULT EjectTray();
	HRESULT CloseTray();

	string	GetDrive(const string& szPartition);
	VOID	GetPartition(LPCSTR strFilename, LPSTR strPartition);
	VOID	RemountDrive(LPCSTR szDrive);

	VOID	UpdateDvdrom();

	HANDLE	OpenCDROM();
	HANDLE	OpenCDROM2();
	INT		ReadSector(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer);
	INT     ReadSomeSectors(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer, int numsectors);
	INT 	ReadSectorMode2(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer);
	INT 	ReadSectorCDDA(HANDLE hDevice, DWORD dwSector, LPSTR lpczBuffer);
	VOID	CloseCDROM(HANDLE hDevice);
	DWORD   GetSCSITOC(LPTOC_SCSI itoc, HANDLE hcdrom);
	
	BOOL	IsDebug();
	HRESULT Shutdown();

	DWORD   IOCTLSendASPI32Command(HANDLE hIOCTL, LPSRB pSRB);
	DWORD   ReadXASector(HANDLE hIOCTL, unsigned int sectornum, unsigned int numsectors, unsigned char *secbuf);
	DWORD   ReadXASector2(unsigned int sectornum, unsigned int numsectors, unsigned char *secbuf);

private:
	HGLOBAL	m_gmXferBuffer;
	PVOID	m_rawXferBuffer;
	DWORD m_dwTrayState;
	DWORD m_dwTrayCount;
	DWORD m_dwLastTrayState;
};

#endif // !defined(AFX_IOSUPPORT_H__F084A488_BD6E_49D5_8CD3_0BE62149DB40__INCLUDED_)
