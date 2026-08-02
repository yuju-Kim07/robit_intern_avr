#ifndef F_CPU
#define F_CPU 16000000UL
#endif
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>

void UART0_Init(void) //ATmega128과 pc와 시리얼 통신이 잘 되도록 하는 함수
{
	UBRR0H = 0;
	UBRR0L = 103; //속도를 9600으로 설정시키기 위한 식에서의 숫자
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); //8비트로 지정& 비동기 방식 사용
	UCSR0B = (1 << TXEN0) | (1 << RXEN0);   //송신, 수신 기능 활성화
}

void Putch(unsigned char data) //한 글자를 송신하는 함수
{
	while (!(UCSR0A & (1 << UDRE0))); //송신버퍼가 비어있을 때(1)까지 대기
	UDR0 = data;                      //UDR0로 data 송신
}

void Put(char *str) //송신된 글자를 문자열만큼 출력하는 함수
{
	while (*str != '\0') //문자열이 NULL이 아닐때만 반복
	{
		Putch(*str); //한 글자를 송신하는 함수에 배열의 원소를 하나씩 집어 넣음
		str++;       //배열 원소 주소 증가
	}
}

unsigned char Getch(void) //한 글자를 수신하는 함수
{
	while (!(UCSR0A & (1 << RXC0))); //수신할 데이터가 있을 때(1)까지 대기
	return UDR0;                     //수신한 글자를 반환
}

// 서보는 '펄스 신호를 얼마나 길게 주느냐'로 각도가 정해짐
#define SERVO_PERIOD  39999u  // 서보신호는 주기가 20ms (50Hz).  20ms=20000us인데, 0.5us마다 타이머가 숫자를 하나 셈-->40000번 세짐--->0~39999까지-->39999u
#define SERVO_PULSE_MIN  500u// 0도일 때 펄스폭(500us)-->서보가 0도 돌아감
#define SERVO_PULSE_MAX 2500u// 180도일 때 펄스폭(2500us)-->서보가 180도 돌아감
#define SERVO_COMBACK_ANGLE  90 // 초기화 시 복귀할 원점 각도

void Servo_Init(void)//서보모터 PWM출력 준비하는 함수
{
	
	DDRB |= (1 << PB7);// PB7을 출력으로 설정
	
	TCCR1A = (1 << COM1C1) | (1 << WGM11);//OC1C핀이 되면 핀이 LOW가 되고, 주기가 끝나면 다시 HIGH가 됨-->HIGH로 유지되는 기간==펄스의 길이
	
	TCCR1B = (1 << WGM13) | (1 << WGM12) | (1 << CS11);//0.5US가 지나면 타이머에 숫자가 하나 올라감

	ICR1 = SERVO_PERIOD;
}

uint16_t Servo_AngleToTicks(uint8_t angle)//각도를 타이머 값으로 바꿔주는 함수
{
	uint32_t pulse_us = SERVO_PULSE_MIN
	    + ((uint32_t)angle * (SERVO_PULSE_MAX - SERVO_PULSE_MIN)) / 180UL;//각도를 펄스 길이로 환산

	return (uint16_t)(pulse_us * 2UL);//US를 타이머가 숫자를 세는 횟수로 치환
}


void Servo_SetAngle(uint8_t angle)// 서보를 원하는 각도로 움직이는 함수
{
	OCR1C = Servo_AngleToTicks(angle);
}

void GetLine(char *buf, uint8_t maxlen)// 시리얼로 들어오는 글자를 한 줄(엔터 누르기 전까지) 모아서 문자열로 만들어주는 함수(예:사용자가 "9" "0" 을 치고 엔터를 누르면 "90" 이 저장)

{
	uint8_t i = 0;
	char c;
	while (1)
	{
		c = Getch(); // 글자 하나가 올 때까지 기다렸다가 받음

		if (c == '\r' || c == '\n') // 엔터 입력되면 끝
		{
			if (i == 0) continue; // 빈 줄(엔터만)은 무시하고 계속 대기
			break;
		}

		if (i < maxlen - 1) // 버퍼가 넘치지 않게 글자 수 제한
		{
			buf[i++] = c; // 받은 글자를 buf 뒤에 하나씩 쌓음
		}
	}
	buf[i] = '\0'; // 문자열 끝을 표시(문자열은 항상 마지막에 NULL이 필요함)
}

int main(void)
{
	char buf[16]; // 입력받은 각도 문자열을 저장할 공간

	UART0_Init();  // 시리얼 통신 준비
	Servo_Init();  // 서보 PWM 신호 준비

	Servo_SetAngle(SERVO_COMBACK_ANGLE); // 전원 투입/리셋 시 원점(90도)으로 복귀
	_delay_ms(500);                   // 서보가 원점까지 실제로 움직일 시간을 벌어줌

	while (1)
	{
		GetLine(buf, sizeof(buf));   // PC에서 한 줄(각도 값) 입력받기
		int angle = atoi(buf);       // 받은 문자열("90" 등)을 진짜 숫자로 변환

		if (angle < 0 || angle > 180) // 0~180 범위를 벗어나면
		{
			Put("WRONG ANGLE\n");     // 경고만 보여주고 모터는 그대로 둠
		}
		else
		{
			Servo_SetAngle((uint8_t)angle); // 정상 범위면 실제로 그 각도로 이동
		}
	}
	return 0;
}