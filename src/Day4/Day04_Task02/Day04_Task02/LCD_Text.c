/*==============================================================================
 *   CLCD 구현부 (I2C / PCF8574 방식)
 *   LCD의 RS, R/W, E, DB4~DB7, 백라이트 신호를
 *   PCF8574라는 칩 하나를 거쳐서 I2C(PD0=SCL, PD1=SDA) 한 줄로 보냄
 *============================================================================*/
#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "LCD_Text.h"

/* ---------------- TWI(I2C) 기본 통신 ---------------- */
static void TWI_Init(void)                  // I2C 통신 준비
{
	TWSR = 0x00;                             // 분주비 1
	TWBR = 72;                               // 통신 속도 약 100kHz (16MHz 기준)
	TWCR = (1 << TWEN);                      // TWI 기능 켜기
}

static void TWI_Start(void)                  // I2C 통신 시작 신호
{
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));          // 완료될 때까지 대기
}

static void TWI_Stop(void)                   // I2C 통신 끝 신호
{
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

static void TWI_Write(uint8_t data)          // I2C로 1바이트 전송
{
	TWDR = data;
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT)));          // 완료될 때까지 대기
}

/* ---------------- PCF8574 -> LCD 신호 조합 ----------------
 * 비트 배치 : DB7 DB6 DB5 DB4 BL E RW RS   (bit7 ~ bit0)
 * BL = 백라이트(항상 켜두려고 1로 고정), RW는 항상 쓰기(0)만 사용
 * ------------------------------------------------------------ */
static void LCD_WriteByte(uint8_t data)      // PCF8574로 8비트 그대로 전송
{
	TWI_Start();
	TWI_Write(LCD_I2C_ADDR << 1);            // LCD 주소 + 쓰기
	TWI_Write(data);
	TWI_Stop();
}

static void LCD_Pulse(uint8_t data)          // E(Enable) 핀을 1->0 으로 눌러줘야 LCD가 읽어감
{
	LCD_WriteByte(data | 0x04);              // E = 1
	_delay_us(1);
	LCD_WriteByte(data & ~0x04);             // E = 0  (이 순간 LCD가 데이터를 읽음)
	_delay_us(50);
}

static void LCD_Send4(uint8_t nibble, uint8_t rs) // 4비트(상위 니블 자리)만 전송
{
	uint8_t data = (nibble & 0xF0) | 0x08 | rs; // 0x08 = 백라이트 켜기, rs=0(명령)/1(데이터)
	LCD_Pulse(data);
}

/* ---------------- 헤더에 선언된 함수들 ---------------- */
void lcdCommand(U8 byte)                     // LCD에 명령 보내기
{
	LCD_Send4(byte & 0xF0, 0);                // 상위 4비트 먼저
	LCD_Send4((byte << 4) & 0xF0, 0);         // 하위 4비트 나중
	if (byte == ALLCLR) _delay_ms(2);         // 화면 지우기는 시간이 더 걸림
	else _delay_us(50);
}

void lcdData(U8 byte)                        // LCD에 글자 하나 보내기
{
	LCD_Send4(byte & 0xF0, 1);
	LCD_Send4((byte << 4) & 0xF0, 1);
	_delay_us(50);
}

void lcdDisplayPosition(U8 line, U8 col)     // line(1~2), col(1~16)로 커서 이동
{
	U8 addr = (line == 1) ? LINE1 : LINE2;
	lcdCommand(addr + (col - 1));
}

void lcdInit(void)                           // LCD 초기화 (I2C 준비까지 포함)
{
	TWI_Init();                              // I2C 통신 먼저 켜기
	_delay_ms(50);                           // LCD 전원 안정될 때까지 대기

	LCD_Send4(0x30, 0);                      // LCD 깨우기 (3번 반복)
	_delay_ms(5);
	LCD_Send4(0x30, 0);
	_delay_us(150);
	LCD_Send4(0x30, 0);
	_delay_us(150);
	LCD_Send4(0x20, 0);                      // 4bit 모드로 전환

	lcdCommand(FUNCSET);                     // 4bit, 2줄, 5x8 폰트
	lcdCommand(DISPON);                      // 화면 켜기
	lcdCommand(ENTMODE);                     // 오른쪽으로 이동하며 쓰기
	lcdCommand(ALLCLR);                      // 화면 지우기
}

void lcdClear(void)                          // 화면 전체 지우기
{
	lcdCommand(ALLCLR);
}

void lcdString(U8 line, U8 col, char *str)   // line, col 위치부터 문자열 출력
{
	lcdDisplayPosition(line, col);
	while (*str != '\0') {
		lcdData(*str);
		str++;
	}
}

void lcdNumber(U8 line, U8 col, int num)     // line, col 위치부터 숫자 출력
{
	char buf[8];
	sprintf(buf, "%d", num);
	lcdString(line, col, buf);
}