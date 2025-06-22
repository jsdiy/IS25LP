//	IS25LP040E - SPI/QPI SERIAL FLASH MEMORY	基本ライブラリ
//	『昼夜逆転』工作室	https://github.com/jsdiy
//	2025/05	@jsdiy

#ifndef IS25LPLIB_H_
#define IS25LPLIB_H_

//ピンアサイン
//・CE#(SS#)はSPIモジュールの方で定義されているものとする。
//・WP#,HOLD#を使用する場合はここに定義し、機能を実装する。

//関数
void	FMEM_Reset(void);
uint8_t	FMEM_Initialize(void);
uint16_t	FMEM_ManufacturerId(void);
uint16_t	FMEM_PageSize(void);		//256Byte
uint16_t	FMEM_TotalPages(void);		//2048ページ	※IS25LP/WP040E(DiveceID:0x12)を想定
uint16_t	FMEM_SectorSize(void);		//4KB
uint8_t		FMEM_TotalSectors(void);	//128セクター		※同上
uint8_t		FMEM_Total32kBlocks(void);	//16ブロック(32KB単位)	※同上
uint8_t		FMEM_Total64kBlocks(void);	//8ブロック(64KB単位)		※同上
uint32_t	FMEM_MemorySize(void);		//4Mbit=512KB	※同上
uint32_t	FMEM_PageToAddr(uint16_t page);
uint16_t	FMEM_AddrToPage(uint32_t addr);
uint32_t	FMEM_SectorToAddr(uint8_t sector);
uint8_t		FMEM_AddrToSector(uint32_t addr);
uint32_t	FMEM_Block32kToAddr(uint8_t block);
uint8_t		FMEM_AddrToBlock32k(uint32_t addr);
uint32_t	FMEM_Block64kToAddr(uint8_t block);
uint8_t		FMEM_AddrToBlock64k(uint32_t addr);
uint8_t	FMEM_ReadByte(uint32_t addr);
void	FMEM_ReadBytes(uint32_t addr, uint8_t* buf, uint16_t length);
void	FMEM_Await(void);
void	FMEM_EraceSectorAsync(uint8_t sector);
void	FMEM_EraceSector(uint8_t sector);
void	FMEM_EraceBlock32kAsync(uint8_t block);
void	FMEM_EraceBlock32k(uint8_t block);
void	FMEM_EraceBlock64kAsync(uint8_t block);
void	FMEM_EraceBlock64k(uint8_t block);
void	FMEM_EraceChipAsync(void);
void	FMEM_EraceChip(void);
void	FMEM_WritePageAsync(uint32_t addr, uint8_t* datas, uint16_t length);
void	FMEM_WritePage(uint32_t addr, uint8_t* datas, uint16_t length);
void	FMEM_WritePages(uint32_t addr, uint8_t* datas, uint16_t length);

#endif	// IS25LPLIB_H_
