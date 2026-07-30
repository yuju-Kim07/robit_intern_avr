#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

volatile uint8_t mode = 0;


ISR(INT2_vect) {
	mode = 3;// 회로도 PD2 = INT2 (스위치 3 눌렀다 뗄 때)
}


ISR(INT3_vect) {
	mode = 4;// 회로도 PD3 = INT3 (스위치 4 눌렀다 뗄 때)
}

int main(void) {
	
	DDRA = 0xFF;  // PA0 ~ PA7 출력 설정 (LED)
	DDRC = 0x00;  // PC0, PC1 입력 설정 (스위치 1, 2)
	DDRD &= ~((1 << PD2) | (1 << PD3)); // PD2, PD3 입력 설정 (스위치 3, 4)
	
	EICRA |= (1 << ISC21) | (1 << ISC20); // INT2 (PD2) 버튼을 누를때, 버튼이 위로 올라가면서 진행
	EICRB |= (1 << ISC31) | (1 << ISC30); // INT3 (PD3) 버튼을 누를때, 버튼이 위로 올라가면서 진행

	EIMSK |= (1 << INT2) | (1 << INT3);   // 외부 인터럽트 허용
	sei();                               // 전역 인터럽트 활성화

	uint8_t blink_state = 0; //led 켜져있음

	while (1) {
		
		if (mode == 3) {//스위치 3을 눌렀다면
			for (int i = 7; i >= 0; i--) {//8개의 led를 내림차수으로
				PORTA = ~(1 << i); // 0V일 때 켜짐
				_delay_ms(150);    // 이동 속도 0.15s
			}
			PORTA = ~0x00; // 전체 끄기
			mode = 0;      // 완료 후 기본 모드로 복귀
		}

		else if (mode == 4) {
			for (int i = 0; i < 8; i++) {//8개의 led를 오름차순으로
				PORTA = ~(1 << i); // 0V일 때 켜짐
				_delay_ms(150);    // 이동 속도 0.15s
			}
			PORTA = ~0x00; // 전체 끄기
			mode = 0;      // 완료 후 기본 모드로 복귀
		}
		
		else {
			uint8_t sw1 = !(PINC & (1 << PINC0)); // PC0
			uint8_t sw2 = !(PINC & (1 << PINC1)); // PC1

			if (sw1 && sw2) { //만약 스위치 1, 2를 동시에 눌렀을 때
				PORTA = ~0xFF; // 전체 켜기
			}
			else if (sw1) {
				PORTA = ~0xF0; // PA4~PA7 켜기
			}
			else if (sw2) {
				PORTA = ~0x0F; //PA0~PA3켜기
			}
			else {
				// 아무 스위치도 안 누를 때 0.5초 간격으로 깜빡이기
				if (blink_state == 0) PORTA = ~0xFF; // 전체 켜기
				else PORTA = ~0x00;                  // 전체 끄기

				
				for (int t = 0; t < 10; t++) {// 0.5초 대기 동안 인터럽트를 감지하도록
					_delay_ms(50);
					if (mode != 0) break;//스위치3,4가 눌린다면 즉시 동작실행하기
				}
				blink_state = !blink_state;//led 꺼져있음
			}
		}
	}

	return 0;
}