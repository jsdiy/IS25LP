//	IS25LP040E - SPI/QPI SERIAL FLASH MEMORY	基本ライブラリ
//	『昼夜逆転』工作室	https://github.com/jsdiy
//	2025/05	@jsdiy

/*
【ポート対応】	ATmega328P/IS25LP040E
bit		7	6	5	4	 3	  2		1	0
portB	-	-	SCK	MISO MOSI -		-	-
				:	:	 :	  :
IS25LP040E		SCK	SO	 SI	  CS#			※WP#,HOLD#はVCCへ接続する。

【ATmega328P】
|543210---54321|
|CCCCCCGRVBBBBB|	R=Vref
>              |
|CDDDDDVGBBDDDB|
|601234--675670|

【ビルド環境】
Microchip Studio 7 (Version: 7.0.2594 - )
*/

#include <avr/io.h>
#include <util/delay.h>
#include "IS25LPlib.h"
#include "spilib.h"

//バッファ
#define BUFSIZE	2000
static	uint8_t	buf[BUFSIZE];

int	main(void)
{
	//初期化
	SPI_MasterInit();
	FMEM_Initialize();

	//通信テスト
	//・IS25LP040Eの場合は0x9D12が返ってくる。
	uint16_t mid = FMEM_ManufacturerId();
	//LCD_PutHexNumber(mid);

	//これから書き込もうとする領域を消去
	//・必要最小限を指定すればよいが、例として64KBブロックで指定している。
	FMEM_EraceBlock64k(0);

	//書き込み
	uint32_t addr = 0x000000UL;
	FMEM_WritePages(addr, buf, BUFSIZE);

	//読み出し
	FMEM_ReadBytes(addr, buf, BUFSIZE);
	//LCD_DrawImage(x, y, w, h, buf);

	return 0;
}
