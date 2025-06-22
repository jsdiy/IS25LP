//	IS25LP040E - SPI/QPI SERIAL FLASH MEMORY	��{���C�u����
//	�w����t�]�x�H�쎺	https://github.com/jsdiy
//	2025/05	@jsdiy

#include <avr/io.h>
#include "spilib.h"
#include "IS25LPlib.h"

#define MANUFACTURER_ID	0x9D

//Instruction Set
typedef	enum
{
	Inst_NORD	= 0x03,	//Normal Read Mode
	Inst_FRD	= 0x0B,	//Fast Read Mode
	Inst_FRDIO	= 0xBB,	//Fast Read Dual I/O
	Inst_FRDO	= 0x3B,	//Fast Read Dual Output
	Inst_FRQO	= 0x6B,	//Fast Read Quad Output
	Inst_FRQIO	= 0xEB,	//Fast Read Quad I/O
	Inst_PP		= 0x02,	//Input Page Program
	Inst_PPQ	= 0x32,	//Quad Input Page Program	(32h/38h)
	Inst_SER	= 0xD7,	//Sector Erase	(D7h/20h)
	Inst_BER32	= 0x52,	//Block Erase 32Kbyte	//In 256Kb/512Kb, both 52h and D8h command are for 32KB ERASE only.
	Inst_BER64	= 0xD8,	//Block Erase 64Kbyte	//In 256Kb/512Kb, both 52h and D8h command are for 32KB ERASE only.
	Inst_CER	= 0xC7,	//Chip Erase	(C7h/60h)
	Inst_WREN	= 0x06,	//Write Enable
	Inst_WRDI	= 0x04,	//Write Disable
	Inst_RDSR	= 0x05,	//Read Status Register
	Inst_WRSR	= 0x01,	//Write Status Register
	Inst_RDFR	= 0x48,	//Read Function Register
	Inst_WRFR	= 0x42,	//Write Function Register
	Inst_QPIEN	= 0x35,	//Enter QPI mode
	Inst_QPIDI	= 0xF5,	//Exit QPI mode
	Inst_PERSUS	= 0x75,	//Suspend during program/erase	(75h/B0h)
	Inst_PERRSM	= 0x7A,	//Resume program/erase	(7Ah/30h)
	Inst_DP		= 0xB9,	//Deep Power Down
	Inst_RDPD	= 0xAB,	//Release Power Down
	//Inst_RDID	= 0xAB,	//Read ID	���R�}���h�Ƃ��Đ������Ȃ��iRDPD�Ɠ���0xAB�j �� RDJDID���g�p����
	Inst_RDJDID	= 0x9F,	//Read JEDEC ID Command
	Inst_RDJDID_QPI	= 0xAF,	//Read JEDEC ID Command
	Inst_RDMDID	= 0x90,	//Read Manufacturer & Device ID
	Inst_RDUID	= 0x4B,	//Read Unique ID
	Inst_RDSFDP	= 0x5A,	//SFDP Read
	Inst_NOP	= 0x00,	//No Operation
	Inst_RSTEN	= 0x66,	//Software Reset Enable
	Inst_RST	= 0x99,	//Software Reset
	Inst_IRER	= 0x64,	//Erase Information Row
	Inst_IRP	= 0x62,	//Program Information Row
	Inst_IRRD	= 0x68,	//Read Information Row
	Inst_STWBR	= 0xC0	//Set Wrapped Burst Read
}
EInstruction;

//Function Register Bit Definition
typedef	enum
{
	FuncRegBv_Reserved0	= (1 << 0),
	FuncRegBv_Reserved1	= (1 << 1),
	FuncRegBv_PSUS		= (1 << 2),	//Program suspend bit
	FuncRegBv_ESUS		= (1 << 3),	//Erase suspend bit
	FuncRegBv_IRLock0	= (1 << 4),	//Lock the Information Row 0
	FuncRegBv_IRLock1	= (1 << 5),	//Lock the Information Row 1
	FuncRegBv_IRLock2	= (1 << 6),	//Lock the Information Row 2
	FuncRegBv_IRLock3	= (1 << 7)	//Lock the Information Row 3
}
EFuncRegBv;

//Status Register Bit
typedef	enum
{
	StsRegBv_WIP	= (1 << 0),	//Write In Progress Bit
	StsRegBv_WEL	= (1 << 1),	//Write Enable Latch
	StsRegBv_BP0	= (1 << 2),	//Block Protection Bit
	StsRegBv_BP1	= (1 << 3),	//Block Protection Bit
	StsRegBv_BP2	= (1 << 4),	//Block Protection Bit
	StsRegBv_BP3	= (1 << 5),	//Block Protection Bit
	StsRegBv_QE		= (1 << 6),	//Quad Enable bit
	StsRegBv_SRWD	= (1 << 7)	//Status Register Write Disable
}
EStsRegBv;

//�ϐ��^�萔
static	const	uint8_t	DUMMY_BYTE	= 0x00;

//�������[���
static	const	uint16_t	PAGE_BYTESIZE		= 256U;
static	const	uint16_t	SECTOR_BYTESIZE		= 4U * 1024;
static	const	uint16_t	BLOCK32K_BYTESIZE	= 32U * 1024;
static	const	uint32_t	BLOCK64K_BYTESIZE	= 64UL * 1024;
static	uint32_t	fmemByteSize;

//�֐�
static	void	SendCommand(EInstruction cmd, uint32_t addr);
static	void	SendCommandOnly(EInstruction cmd);
static	void	SendData(uint8_t data);
static	uint8_t	ReceiveData(void);
static	void	WriteEnable(void);

//�R�}���h�𑗐M����
static	void	SendCommand(EInstruction cmd, uint32_t addr)
{
	SPI_MasterTransmit(cmd);
	SPI_MasterTransmit((addr >> 16) & 0xFF);
	SPI_MasterTransmit((addr >> 8) & 0xFF);
	SPI_MasterTransmit(addr & 0xFF);
}

//�R�}���h�𑗐M����
static	void	SendCommandOnly(EInstruction cmd)
{
	SPI_MasterTransmit(cmd);
}

//�f�[�^�𑗐M����
static	void	SendData(uint8_t data)
{
	SPI_MasterTransmit(data);
}

//��M�f�[�^���擾����
static	uint8_t	ReceiveData(void)
{
	SPI_MasterTransmit(DUMMY_BYTE);
	uint8_t data = SPI_MasterReceive();
	return data;
}

//���Z�b�g
void	FMEM_Reset(void)
{
	SPI_SlaveSelect(ESpi_Mem);
	SPI_MasterTransmit(Inst_RSTEN);
	SPI_SlaveDeselect(ESpi_Mem);

	SPI_SlaveSelect(ESpi_Mem);
	SPI_MasterTransmit(Inst_RST);
	SPI_SlaveDeselect(ESpi_Mem);
}

//������
uint8_t	FMEM_Initialize(void)
{
	uint16_t manuDevId = FMEM_ManufacturerId();
	uint8_t mid = (manuDevId >> 8) & 0xFF;
	uint8_t did = manuDevId & 0xFF;

	switch (did)
	{
	case	0x12:	fmemByteSize = 4UL * 1024 * 1024 / 8;	break;	//IS25LP040E, IS25WP040E
	case	0x11:	fmemByteSize = 2UL * 1024 * 1024 / 8;	break;	//IS25LP020E, IS25WP020E
	case	0x10:	fmemByteSize = 1UL * 1024 * 1024 / 8;	break;	//IS25LP010E, IS25WP010E
	case	0x05:	fmemByteSize =		512UL * 1024 / 8;	break;	//IS25LP512E, IS25WP512E
	case	0x02:	fmemByteSize =		256UL * 1024 / 8;	break;	//IS25LP025E, IS25WP025E
	default:	fmemByteSize = 0;	break;
	}

	return (mid == MANUFACTURER_ID);
}

//������ID,�f�o�C�XID
//�߂�l�F	ManufacturerID, DeviceID
uint16_t	FMEM_ManufacturerId(void)
{
	SPI_SlaveSelect(ESpi_Mem);

	SendCommand(Inst_RDMDID, 0x000000UL);
	uint8_t mid = ReceiveData();
	uint8_t did = ReceiveData();

	SPI_SlaveDeselect(ESpi_Mem);
	return (((uint16_t)mid) << 8) | did;
}

//�y�[�W�T�C�Y(byte)�A�y�[�W�������擾����
uint16_t	FMEM_PageSize(void) { return PAGE_BYTESIZE; }
uint16_t	FMEM_TotalPages(void) { return fmemByteSize / PAGE_BYTESIZE; }

//�Z�N�^�[�T�C�Y(byte)�A�Z�N�^�[�������擾����
uint16_t	FMEM_SectorSize(void) { return SECTOR_BYTESIZE; }
uint8_t		FMEM_TotalSectors(void) { return fmemByteSize / SECTOR_BYTESIZE; }

//�u���b�N�������擾����
uint8_t	FMEM_Total32kBlocks(void) { return fmemByteSize / BLOCK32K_BYTESIZE; }
uint8_t	FMEM_Total64kBlocks(void) { return fmemByteSize / BLOCK64K_BYTESIZE; }

//�������[�e�ʂ��擾����(byte)
uint32_t	FMEM_MemorySize(void) { return fmemByteSize; }

//�y�[�W-�A�h���X�ϊ�	//�y�[�W�̐擪�A�h���X
uint32_t	FMEM_PageToAddr(uint16_t page) { return (uint32_t)page * PAGE_BYTESIZE; }

//�A�h���X-�y�[�W�ϊ�	//�A�h���X���܂܂��y�[�W�̃C���f�b�N�X
uint16_t	FMEM_AddrToPage(uint32_t addr) { return addr / PAGE_BYTESIZE; }

//�Z�N�^�[-�A�h���X�ϊ�	//�Z�N�^�[�̐擪�A�h���X
uint32_t	FMEM_SectorToAddr(uint8_t sector) { return (uint32_t)sector * SECTOR_BYTESIZE; }

//�A�h���X-�Z�N�^�[�ϊ�	//�A�h���X���܂܂��Z�N�^�[�̃C���f�b�N�X
uint8_t	FMEM_AddrToSector(uint32_t addr) { return addr / SECTOR_BYTESIZE; }

//�u���b�N32K-�A�h���X�ϊ�	//�u���b�N32K�̐擪�A�h���X
uint32_t	FMEM_Block32kToAddr(uint8_t block) { return (uint32_t)block * BLOCK32K_BYTESIZE; }

//�A�h���X-�u���b�N32K�ϊ�	//�A�h���X���܂܂��u���b�N32K�̃C���f�b�N�X
uint8_t	FMEM_AddrToBlock32k(uint32_t addr) { return addr / BLOCK32K_BYTESIZE; }

//�u���b�N64K-�A�h���X�ϊ�	//�u���b�N64K�̐擪�A�h���X
uint32_t	FMEM_Block64kToAddr(uint8_t block) { return (uint32_t)block * BLOCK64K_BYTESIZE; }

//�A�h���X-�u���b�N64K�ϊ�	//�A�h���X���܂܂��u���b�N64K�̃C���f�b�N�X
uint8_t	FMEM_AddrToBlock64k(uint32_t addr) { return addr / BLOCK64K_BYTESIZE; }

//�f�[�^��ǂݍ���
uint8_t	FMEM_ReadByte(uint32_t addr)
{
	SPI_SlaveSelect(ESpi_Mem);

	SendCommand(Inst_NORD, addr);
	uint8_t data = ReceiveData();

	SPI_SlaveDeselect(ESpi_Mem);
	return data;
}

//�f�[�^��ǂݍ��ށi�A���j
/*
�Eaddr����f�[�^��ǂݎn�߂�B
�E�y�[�W�A�Z�N�^�[�A�u���b�N�̋��E�Ɋ֌W�Ȃ��A�`�b�v�̖����܂œǂݑ�����B
�E�`�b�v�̖����܂ŗ���ƁA�`�b�v�̐擪���瑱����ǂށB
	�ȍ~�A�`�b�v�������[�v����length���̃f�[�^��ǂݑ�����B
*/
void	FMEM_ReadBytes(uint32_t addr, uint8_t* buf, uint16_t length)
{
	SPI_SlaveSelect(ESpi_Mem);

	SendCommand(Inst_NORD, addr);
	for (uint16_t i = 0; i < length; i++) { buf[i] = ReceiveData(); }

	SPI_SlaveDeselect(ESpi_Mem);
}

//�񓯊��Ŏ��s�����������������������̊�����҂�
void	FMEM_Await(void)
{
	uint8_t stsReg;
	do
	{
		SPI_SlaveSelect(ESpi_Mem);
		SendCommandOnly(Inst_RDSR);
		stsReg = ReceiveData();
		SPI_SlaveDeselect(ESpi_Mem);
	}
	while (stsReg & StsRegBv_WIP);
}
//
uint8_t	FMEM_Test_WIP(void)
{
	WriteEnable();

	SPI_SlaveSelect(ESpi_Mem);
	SendCommandOnly(Inst_RDSR);
	uint8_t stsReg = ReceiveData();
	SPI_SlaveDeselect(ESpi_Mem);
	return stsReg;
}

//�������݁^��������������(WREN)
static	void	WriteEnable(void)
{
	SPI_SlaveSelect(ESpi_Mem);
	SendCommandOnly(Inst_WREN);
	SPI_SlaveDeselect(ESpi_Mem);
}

//�f�[�^����������i�Z�N�^�[�P�ʁj�F�񓯊�
void	FMEM_EraceSectorAsync(uint8_t sector)
{
	WriteEnable();
	SPI_SlaveSelect(ESpi_Mem);
	SendCommand(Inst_SER, FMEM_SectorToAddr(sector));
	SPI_SlaveDeselect(ESpi_Mem);	//�����ŏ������J�n�����
}

//�f�[�^����������i�Z�N�^�[�P�ʁj
void	FMEM_EraceSector(uint8_t sector)
{
	FMEM_EraceSectorAsync(sector);
	FMEM_Await();
}

//�f�[�^����������i32KB�u���b�N�P�ʁj�F�񓯊�
void	FMEM_EraceBlock32kAsync(uint8_t block)
{
	WriteEnable();
	SPI_SlaveSelect(ESpi_Mem);
	SendCommand(Inst_BER32, FMEM_Block32kToAddr(block));
	SPI_SlaveDeselect(ESpi_Mem);	//�����ŏ������J�n�����
}

//�f�[�^����������i32KB�u���b�N�P�ʁj
void	FMEM_EraceBlock32k(uint8_t block)
{
	FMEM_EraceBlock32kAsync(block);
	FMEM_Await();
}

//�f�[�^����������i64KB�u���b�N�P�ʁj�F�񓯊�
void	FMEM_EraceBlock64kAsync(uint8_t block)
{
	WriteEnable();
	SPI_SlaveSelect(ESpi_Mem);
	SendCommand(Inst_BER64, FMEM_Block64kToAddr(block));
	SPI_SlaveDeselect(ESpi_Mem);	//�����ŏ������J�n�����
}

//�f�[�^����������i64KB�u���b�N�P�ʁj
void	FMEM_EraceBlock64k(uint8_t block)
{
	FMEM_EraceBlock64kAsync(block);
	FMEM_Await();
}

//�f�[�^����������i�`�b�v�S�́j�F�񓯊�
void	FMEM_EraceChipAsync(void)
{
	WriteEnable();
	SPI_SlaveSelect(ESpi_Mem);
	SendCommandOnly(Inst_CER);
	SPI_SlaveDeselect(ESpi_Mem);	//�����ŏ������J�n�����
}

//�f�[�^����������i�`�b�v�S�́j
void	FMEM_EraceChip(void)
{
	FMEM_EraceChipAsync();
	FMEM_Await();
}

//�f�[�^����������
/*
�Eaddr����f�[�^�������n�߂�B
�Eaddr���܂ރy�[�W�̖����܂ŗ���ƁA�y�[�W�̐擪���瑱���������B���y�[�W�T�C�Y��256byte
	�ȍ~�A�y�[�W�������[�v���Ȃ���length���̃f�[�^������������B
�E�y�[�W���Ńf�[�^���������܂Ȃ����������̊����f�[�^�͉����e�����󂯂Ȃ��B
*/
//�f�[�^���������ށi1�y�[�W���j�F�񓯊�
void	FMEM_WritePageAsync(uint32_t addr, uint8_t* datas, uint16_t length)
{
	WriteEnable();
	SPI_SlaveSelect(ESpi_Mem);
	SendCommand(Inst_PP, addr);
	for (uint16_t i = 0; i < length; i++) { SendData(datas[i]); }
	SPI_SlaveDeselect(ESpi_Mem);	//�����ŏ����݂��J�n�����
}

//�f�[�^���������ށi1�y�[�W���j
//�Elength���y�[�W�T�C�Y��蒷���ꍇ�A���̃y�[�W���Ń��[�v���ď㏑������B
void	FMEM_WritePage(uint32_t addr, uint8_t* datas, uint16_t length)
{
	FMEM_WritePageAsync(addr, datas, length);
	FMEM_Await();
}

//�f�[�^���������ށi�y�[�W���܂����j
void	FMEM_WritePages(uint32_t addr, uint8_t* datas, uint16_t length)
{
	uint16_t idx = 0;
	while (0 < length)
	{
		uint16_t writeLength = (PAGE_BYTESIZE < length) ? PAGE_BYTESIZE : length;
		FMEM_WritePageAsync(addr, &datas[idx], writeLength);
		FMEM_Await();
		length -= writeLength;
		idx += writeLength;
		addr += PAGE_BYTESIZE;
	}
}
