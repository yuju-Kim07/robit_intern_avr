#define F_CPU 16000000UL

#include <avr/io.h>
#include <util/delay.h>

void updateLeds(uint8_t pattern)// LED 출력 처리 (반전 출력)
{
	PORTA = ~pattern;
}

uint8_t readSwitches(void)// 스위치 입력 상태 확인 후 누른 버튼 번호 반환
{
	if (!(PINC & (1 << PC0))) return 1; // SW1 눌림 감지
	if (!(PINC & (1 << PC1))) return 2; // SW2 눌림 감지
	if (!(PIND & (1 << PD2))) return 3; // SW3 눌림 감지
	if (!(PIND & (1 << PD3))) return 4; // SW4 눌림 감지
	return 0;                           // 눌린 버튼 없음
}

int main(void)
{
	uint8_t binaryCounter = 0; // 이진 카운팅용 변수
	uint8_t activeSw = 0;      // 눌린 스위치 번호 저장
	int idx = 0;              // 반복문 제어 변수

	
	DDRA = 0xFF;// LED 연결 포트(PORTA) 전체 출력 설정
	PORTA = 0xFF; // 초기 LED 상태--> 전체 꺼짐

	// PC0과 PC1 핀을 입력 모드로 설정할 때 (비트를 0으로)
	DDRC &= ~((1 << PC0) | (1 << PC1));
	// 해당 입력 핀들의 내부 풀업 저항을 활성화할 때 (비트를 1로)
	PORTC |= (1 << PC0) | (1 << PC1);

	// PD2와 PD3 핀을 입력 모드로 설정할 때 (비트를 0으로)
	DDRD &= ~((1 << PD2) | (1 << PD3));
	// 해당 입력 핀들의 내부 풀업 저항을 활성화할 때 (비트를 1로)
	PORTD |= (1 << PD2) | (1 << PD3);
	
	while (1)
	{
		// 스위치 상태 주기적 확인
		activeSw = readSwitches();

		switch (activeSw)
		{
			case 1: // SW1 (PC0) 동작: LED 3개 묶음 왼쪽으로
			{
				uint8_t shiftData = 0x07; // 0b00000111 (LED 3개)
				for (idx = 0; idx < 2; idx++)
				{
					updateLeds(shiftData);
					_delay_ms(300);
					shiftData <<= 1;     // 왼쪽으로 이동
				}
				while (!(PINC & (1 << PC0))); // 버튼 뗄 때까지 대기
				break;
			}

			case 2: // SW2 (PC1) 동작: LED 3개 묶음 오른쪽으로
			{
				uint8_t shiftData = 0xE0; // 0b11100000 (LED 3개)
				for (idx = 0; idx < 2; idx++)
				{
					updateLeds(shiftData);
					_delay_ms(300);
					shiftData >>= 1;     // 오른쪽으로 이동
				}
				while (!(PINC & (1 << PC1))); // 버튼 뗄 때까지 대기
				break;
			}

			case 3: // SW3 (PD2) 동작: LED 좌우 왕복 깜박임
			{
				// 오른쪽에서 왼쪽 방향으로 깜박임 (PA7 -> PA0)
				for (idx = 7; idx >= 0; idx--)
				{
					updateLeds(1 << idx);
					_delay_ms(100);
				}
				// 다시 오른쪽 방향으로 돌아가며 깜박임 (PA1 -> PA6)
				for (idx = 1; idx <= 6; idx++)
				{
					updateLeds(1 << idx);
					_delay_ms(100);
				}
				while (!(PIND & (1 << PD2))); // 버튼 뗄 때까지 대기
				break;
			}

			case 4: // SW4 (PD3) 동작: 카운터 초기화 및 LED OFF
			{
				binaryCounter = 0; // 이진 카운터 값 리셋
				PORTA = 0xFF;      // LED 전체 꺼짐
				while (!(PIND & (1 << PD3))); // 버튼 뗄 때까지 대기
				break;
			}

			default: // 버튼이 안 눌렸을 때 기본 동작--> 2진수 1씩 증가
			{
				updateLeds(binaryCounter);
				binaryCounter++;
				_delay_ms(100);
				break;
			}
		}
	}

	return 0;
}