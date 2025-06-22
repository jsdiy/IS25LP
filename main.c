//	IS25LP040E - SPI/QPI SERIAL FLASH MEMORY	��{���C�u����
//	�w����t�]�x�H�쎺	https://github.com/jsdiy
//	2025/05	@jsdiy

/*
�y�|�[�g�Ή��z	ATmega328P/IS25LP040E
bit		7	6	5	4	 3	  2		1	0
portB	-	-	SCK	MISO MOSI -		-	-
				:	:	 :	  :
IS25LP040E		SCK	SO	 SI	  CS#			��WP#,HOLD#��VCC�֐ڑ�����B

�yATmega328P�z
|543210---54321|
|CCCCCCGRVBBBBB|	R=Vref
>              |
|CDDDDDVGBBDDDB|
|601234--675670|

�y�r���h���z
Microchip Studio 7 (Version: 7.0.2594 - )
*/

#include <avr/io.h>
#include <util/delay.h>
#include "IS25LPlib.h"
#include "spilib.h"

//�o�b�t�@
#define BUFSIZE	2000
static	uint8_t	buf[BUFSIZE];

int	main(void)
{
	//������
	SPI_MasterInit();
	FMEM_Initialize();

	//�ʐM�e�X�g
	//�EIS25LP040E�̏ꍇ��0x9D12���Ԃ��Ă���B
	uint16_t mid = FMEM_ManufacturerId();
	//LCD_PutHexNumber(mid);

	//���ꂩ�珑���������Ƃ���̈������
	//�E�K�v�ŏ������w�肷��΂悢���A��Ƃ���64KB�u���b�N�Ŏw�肵�Ă���B
	FMEM_EraceBlock64k(0);

	//��������
	uint32_t addr = 0x000000UL;
	FMEM_WritePages(addr, buf, BUFSIZE);

	//�ǂݏo��
	FMEM_ReadBytes(addr, buf, BUFSIZE);
	//LCD_DrawImage(x, y, w, h, buf);

	return 0;
}
