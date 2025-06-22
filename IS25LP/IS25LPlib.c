//	IS25LP040E - SPI/QPI SERIAL FLASH MEMORY	基本ライブラリ
//	『昼夜逆転』工作室	https://github.com/jsdiy
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
	//Inst_RDID	= 0xAB,	//Read ID	※コマンドとして成立しない（RDPDと同じ0xAB） → RDJDIDを使用する
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

//変数／定数
static	const	uint8_t	DUMMY_BYTE	= 0x00;

//メモリー情報
static	const	uint16_t	PAGE_BYTESIZE		= 256U;
static	const	uint16_t	SECTOR_BYTESIZE		= 4U * 1024;
static	const	uint16_t	BLOCK32K_BYTESIZE	= 32U * 1024;
static	const	uint32_t	BLOCK64K_BYTESIZE	= 64UL * 1024;
static	uint32_t	fmemByteSize;

//関数
static	void	SendCommand(EInstruction cmd, uint32_t addr);
static	void	SendCommandOnly(EInstruction cmd);
static	void	SendData(uint8_t data);
static	uint8_t	ReceiveData(void);
static	void	WriteEnable(void);

//コマンドを送信する
static	void	SendCommand(EInstruction cmd, uint32_t addr)
{
	SPI_MasterTransmit(cmd);
	SPI_MasterTransmit((addr >> 16) & 0xFF);
	SPI_MasterTransmit((addr >> 8) & 0xFF);
	SPI_MasterTransmit(addr & 0xFF);
}

//コマンドを送信する
static	void	SendCommandOnly(EInstruction cmd)
{
	SPI_MasterTransmit(cmd);
}

//データを送信する
static	void	SendData(uint8_t data)
{
	SPI_MasterTransmit(data);
}

//受信データを取得する
static	uint8_t	ReceiveData(void)
{
	SPI_MasterTransmit(DUMMY_BYTE);
	uint8_t data = SPI_MasterReceive();
	return data;
}

//リセット
void	FMEM_Reset(void)
{
	SPI_SlaveSelect(ESpi_Mem);
	SPI_MasterTransmit(Inst_RSTEN);
	SPI_SlaveDeselect(ESpi_Mem);

	SPI_SlaveSelect(ESpi_Mem);
	SPI_MasterTransmit(Inst_RST);
	SPI_SlaveDeselect(ESpi_Mem);
}

//初期化
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

//製造者ID,デバイスID
//戻り値：	ManufacturerID, DeviceID
uint16_t	FMEM_ManufacturerId(void)
{
	SPI_SlaveSelect(ESpi_Mem);

	SendCommand(Inst_RDMDID, 0x000000UL);
	uint8_t mid = ReceiveData();
	uint8_t did = ReceiveData();

	SPI_SlaveDeselect(ESpi_Mem);
	return (((uint16_t)mid) << 8) | did;
}

//ページサイズ(byte)、ページ総数を取得する
uint16_t	FMEM_PageSize(void) { return PAGE_BYTESIZE; }
uint16_t	FMEM_TotalPages(void) { return fmemByteSize / PAGE_BYTESIZE; }

//セクターサイズ(byte)、セクター総数を取得する
uint16_t	FMEM_SectorSize(void) { return SECTOR_BYTESIZE; }
uint8_t		FMEM_TotalSectors(void) { return fmemByteSize / SECTOR_BYTESIZE; }

//ブロック総数を取得する
uint8_t	FMEM_Total32kBlocks(void) { return fmemByteSize / BLOCK32K_BYTESIZE; }
uint8_t	FMEM_Total64kBlocks(void) { return fmemByteSize / BLOCK64K_BYTESIZE; }

//メモリー容量を取得する(byte)
uint32_t	FMEM_MemorySize(void) { return fmemByteSize; }

//ページ-アドレス変換	//ページの先頭アドレス
uint32_t	FMEM_PageToAddr(uint16_t page) { return (uint32_t)page * PAGE_BYTESIZE; }

//アドレス-ページ変換	//アドレスが含まれるページのインデックス
uint16_t	FMEM_AddrToPage(uint32_t addr) { return addr / PAGE_BYTESIZE; }

//セクター-アドレス変換	//セクターの先頭アドレス
uint32_t	FMEM_SectorToAddr(uint8_t sector) { return (uint32_t)sector * SECTOR_BYTESIZE; }

//アドレス-セクター変換	//アドレスが含まれるセクターのインデックス
uint8_t	FMEM_AddrToSector(uint32_t addr) { return addr / SECTOR_BYTESIZE; }

//ブロック32K-アドレス変換	//ブロック32Kの先頭アドレス
uint32_t	FMEM_Block32kToAddr(uint8_t block) { return (uint32_t)block * BLOCK32K_BYTESIZE; }

//アドレス-ブロック32K変換	//アドレスが含まれるブロック32Kのインデックス
uint8_t	FMEM_AddrToBlock32k(uint32_t addr) { return addr / BLOCK32K_BYTESIZE; }

//ブロック64K-アドレス変換	//ブロック64Kの先頭アドレス
uint32_t	FMEM_Block64kToAddr(uint8_t block) { return (uint32_t)block * BLOCK64K_BYTESIZE; }

//アドレス-ブロック64K変換	//アドレスが含まれるブロック64Kのインデックス
uint8_t	FMEM_AddrToBlock64k(uint32_t addr) { return addr / BLOCK64K_BYTESIZE; }

//データを読み込む
uint8_t	FMEM_ReadByte(uint32_t addr)
{
	SPI_SlaveSelect(ESpi_Mem);

	SendCommand(Inst_NORD, addr);
	uint8_t data = ReceiveData();

	SPI_SlaveDeselect(ESpi_Mem);
	return data;
}

//データを読み込む（連続）
/*
・addrからデータを読み始める。
・ページ、セクター、ブロックの境界に関係なく、チップの末尾まで読み続ける。
・チップの末尾まで来ると、チップの先頭から続きを読む。
	以降、チップ内をループしてlength分のデータを読み続ける。
*/
void	FMEM_ReadBytes(uint32_t addr, uint8_t* buf, uint16_t length)
{
	SPI_SlaveSelect(ESpi_Mem);

	SendCommand(Inst_NORD, addr);
	for (uint16_t i = 0; i < length; i++) { buf[i] = ReceiveData(); }

	SPI_SlaveDeselect(ESpi_Mem);
}

//非同期で実行したメモリ書き換え処理の完了を待つ
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

//書き込み／消去を準備する(WREN)
static	void	WriteEnable(void)
{
	SPI_SlaveSelect(ESpi_Mem);
	SendCommandOnly(Inst_WREN);
	SPI_SlaveDeselect(ESpi_Mem);
}

//データを消去する（セクター単位）：非同期
void	FMEM_EraceSectorAsync(uint8_t sector)
{
	WriteEnable();
	SPI_SlaveSelect(ESpi_Mem);
	SendCommand(Inst_SER, FMEM_SectorToAddr(sector));
	SPI_SlaveDeselect(ESpi_Mem);	//ここで消去が開始される
}

//データを消去する（セクター単位）
void	FMEM_EraceSector(uint8_t sector)
{
	FMEM_EraceSectorAsync(sector);
	FMEM_Await();
}

//データを消去する（32KBブロック単位）：非同期
void	FMEM_EraceBlock32kAsync(uint8_t block)
{
	WriteEnable();
	SPI_SlaveSelect(ESpi_Mem);
	SendCommand(Inst_BER32, FMEM_Block32kToAddr(block));
	SPI_SlaveDeselect(ESpi_Mem);	//ここで消去が開始される
}

//データを消去する（32KBブロック単位）
void	FMEM_EraceBlock32k(uint8_t block)
{
	FMEM_EraceBlock32kAsync(block);
	FMEM_Await();
}

//データを消去する（64KBブロック単位）：非同期
void	FMEM_EraceBlock64kAsync(uint8_t block)
{
	WriteEnable();
	SPI_SlaveSelect(ESpi_Mem);
	SendCommand(Inst_BER64, FMEM_Block64kToAddr(block));
	SPI_SlaveDeselect(ESpi_Mem);	//ここで消去が開始される
}

//データを消去する（64KBブロック単位）
void	FMEM_EraceBlock64k(uint8_t block)
{
	FMEM_EraceBlock64kAsync(block);
	FMEM_Await();
}

//データを消去する（チップ全体）：非同期
void	FMEM_EraceChipAsync(void)
{
	WriteEnable();
	SPI_SlaveSelect(ESpi_Mem);
	SendCommandOnly(Inst_CER);
	SPI_SlaveDeselect(ESpi_Mem);	//ここで消去が開始される
}

//データを消去する（チップ全体）
void	FMEM_EraceChip(void)
{
	FMEM_EraceChipAsync();
	FMEM_Await();
}

//データを書き込む
/*
・addrからデータを書き始める。
・addrを含むページの末尾まで来ると、ページの先頭から続きを書く。※ページサイズは256byte
	以降、ページ内をループしながらlength分のデータを書き続ける。
・ページ内でデータを書き込まなかった部分の既存データは何も影響を受けない。
*/
//データを書き込む（1ページ内）：非同期
void	FMEM_WritePageAsync(uint32_t addr, uint8_t* datas, uint16_t length)
{
	WriteEnable();
	SPI_SlaveSelect(ESpi_Mem);
	SendCommand(Inst_PP, addr);
	for (uint16_t i = 0; i < length; i++) { SendData(datas[i]); }
	SPI_SlaveDeselect(ESpi_Mem);	//ここで書込みが開始される
}

//データを書き込む（1ページ内）
//・lengthがページサイズより長い場合、そのページ内でループして上書きする。
void	FMEM_WritePage(uint32_t addr, uint8_t* datas, uint16_t length)
{
	FMEM_WritePageAsync(addr, datas, length);
	FMEM_Await();
}

//データを書き込む（ページをまたぐ）
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
