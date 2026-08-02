/*==============================================================================
 *   CLCD (I2C 방식, PCF8574 사용)
 *   MPU_Type : Atmega128 (16MHz)
 *   SCL = PD0, SDA = PD1  (ATmega128의 하드웨어 I2C 핀)
 *============================================================================*/
#ifndef __Text_Lcd_H
#define __Text_Lcd_H

#include <avr/io.h>

/* PCF8574(I2C->병렬 변환칩) 주소
 * 안 나오면 0x20~0x27, 0x38~0x3F 중에서 바꿔가며 시도해볼 것 (i2c_scanner.c 로 확인 가능) */
#define LCD_I2C_ADDR   0x27

/* LCD(HD44780 계열) 명령어 */
#define FUNCSET   0x28   // Function set : 4bit 모드, 2줄, 5x8 폰트
#define ENTMODE   0x06   // Entry mode set : 글자 쓸 때마다 오른쪽으로 이동
#define ALLCLR    0x01   // 화면 전체 지우기
#define DISPON    0x0C   // 화면 켜기, 커서는 안 보이게
#define LINE1     0x80   // 1번째 줄 시작 주소
#define LINE2     0xC0   // 2번째 줄 시작 주소

typedef unsigned char U8;
typedef unsigned int  U16;

/* ===== 사용자가 직접 호출하는 함수 ===== */
void lcdInit(void);                          // LCD 초기화 (제일 먼저 한 번 호출)
void lcdClear(void);                         // 화면 전체 지우기
void lcdString(U8 line, U8 col, char *str);  // line(1~2), col(1~16) 위치에 문자열 출력
void lcdNumber(U8 line, U8 col, int num);    // line, col 위치에 숫자 출력

/* ===== 내부적으로만 쓰이는 함수 ===== */
void lcdCommand(U8 byte);
void lcdData(U8 byte);
void lcdDisplayPosition(U8 line, U8 col);

#endif