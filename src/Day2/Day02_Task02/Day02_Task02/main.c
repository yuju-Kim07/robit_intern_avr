#define F_CPU 16000000UL // CPU 클럭 16MHz 설정

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

#define LCD_ADDR 0x27 // I2C LCD 모듈 주소 정의

// 1. I2C 통신 기본 함수들 (TWI 모듈 설정)
void TWI_init(void) {
	TWSR = 0x00; // TWI 비트레이트 분주비 기본 1 설정
	TWBR = 72;   // TWI 전송 속도 100kHz 설정 (SCL 주파수 계산 값)
	TWCR = (1 << TWEN); // TWI(I2C) 통신 기능 활성화
}

void TWI_start(void) {
	// I2C 통신 시작(START) 조건 발생 및 전송 제어권 확보
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT))); // TWI 하드웨어 처리 완료 대기
}

void TWI_write(unsigned char data) {
	TWDR = data; // 전송할 데이터 레지스터에 값 저장
	TWCR = (1 << TWINT) | (1 << TWEN); // 데이터 전송 시작
	while (!(TWCR & (1 << TWINT))); // 전송 완료 플래그(TWINT) 대기
}

void TWI_stop(void) {
	// I2C 통신 종료(STOP) 신호 전송 및 버스 해제
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

// 2. LCD 제어 함수들
void LCD_I2C_write(unsigned char data) {
	TWI_start(); // I2C 통신 시작
	TWI_write(LCD_ADDR << 1); // LCD 슬레이브 주소 전송 (Write 모드)
	TWI_write(data); // 제어 데이터 전송
	TWI_stop(); // I2C 통신 종료
}

void LCD_pulse(unsigned char data) {
	// Enable 핀 펄스(HIGH -> LOW) 생성으로 LCD에 데이터 래치 전달
	LCD_I2C_write(data | 0x04); // EN 핀을 HIGH 상태로 설정 (0x04 비트 조합)
	_delay_us(1);               // 최소 유지 시간 대기
	LCD_I2C_write(data & ~0x04); // EN 핀을 LOW 상태로 복원
	_delay_us(50);              // 명령어 처리 대기 시간
}

void LCD_send4bit(unsigned char data) {
	LCD_I2C_write(data); // 4비트 데이터 전송
	LCD_pulse(data);     // 펄스 신호 인가
}

void LCD_command(unsigned char command) {
	// 8비트 명령어를 상위 4비트와 하위 4비트로 나누어 순차 전송
	unsigned char high = command & 0xF0; // 상위 4비트 추출
	unsigned char low = (command << 4) & 0xF0; // 하위 4비트 추출
	LCD_send4bit(high | 0x08); // 백라이트 유지(0x08)하며 상위 4비트 전송 (RS=0: 명령어)
	LCD_send4bit(low | 0x08);  // 백라이트 유지(0x08)하며 하위 4비트 전송
}

void LCD_data(unsigned char data) {
	// 8비트 문자 데이터를 상위 4비트와 하위 4비트로 나누어 순차 전송
	unsigned char high = data & 0xF0; // 상위 4비트 추출
	unsigned char low = (data << 4) & 0xF0; // 하위 4비트 추출
	LCD_send4bit(high | 0x09); // 데이터 전송 모드(RS=1, 백라이트=0x08 -> 합쳐서 0x09)로 상위 4비트 전송
	LCD_send4bit(low | 0x09);  // 데이터 전송 모드로 하위 4비트 전송
}

void LCD_init(void) {
	_delay_ms(50); // 전원 투입 후 LCD 안정화 대기 시간
	
	// LCD 초기화 시퀀스 명령어 강제 전송 (4비트 모드 진입 과정)
	LCD_send4bit(0x30 | 0x08);
	_delay_ms(5);
	LCD_send4bit(0x30 | 0x08);
	_delay_us(200);
	LCD_send4bit(0x30 | 0x08);
	_delay_us(200);
	LCD_send4bit(0x20 | 0x08); // 4비트 통신 모드로 전환 설정
	_delay_us(200);

	LCD_command(0x28); // 기능 설정: 2줄 표시(2-line), 5x8 폰트 Matrix 설정
	LCD_command(0x0C); // 디스플레이 제어: 화면 켜기(Display ON), 커서 숨김(Cursor OFF)
	LCD_command(0x06); // 모드 설정: 문자 입력 시 커서 우측 자동 이동 설정
	LCD_command(0x01); // 화면 클리어: 화면 전체 지우기 및 DDRAM 주소 0번 초기화
	_delay_ms(2);
}

void LCD_string(char *text) {
	// 문자열 포인터를 받아 NULL 문자가 나올 때까지 문자를 하나씩 출력
	while (*text) {
		LCD_data(*text);
		text++;
	}
}

void LCD_position(unsigned char row, unsigned char column) {
	// 커서 위치 지정 (row: 0=첫째 줄, 1=둘째 줄 / column: 0~15 열)
	if (row == 0) LCD_command(0x80 + column); // 첫 번째 줄 DDRAM 시작 주소(0x80) 연산
	else LCD_command(0xC0 + column);          // 두 번째 줄 DDRAM 시작 주소(0xC0) 연산
}

// 3. 스위치 입력 감지 함수 (채터링 방지 및 Active-Low 하드웨어 대응)
unsigned char get_button_press(volatile uint8_t *pin, uint8_t bit) {
	if (!((*pin) & (1 << bit))) { // 버튼 눌림 감지 (Active-Low로 인해 0일 때 참)
		_delay_ms(25);            // 기계적 접점 튀는 현상(채터링) 제거를 위한 지연 대기
		if (!((*pin) & (1 << bit))) {
			return 1;             // 노이즈 필터링 통과 후 유효한 눌림 신호 반환
		}
	}
	return 0;
}

// 4. 메인 실행부
int main(void) {
	int val_a = 1;      // 첫 번째 피연산자 A 초기값 1 설정
	int val_b = 1;      // 두 번째 피연산자 B 초기값 1 설정
	int result = 2;     // 연산 결과 C 초기값 2 설정
	
	char op_list[4] = {'+', '-', '*', '/'}; // 사칙연산자 순환 배열 정의
	int op_index = 0;   // 연산자 선택 인덱스 ('+' 기호부터 시작)
	
	char lcd_buffer[17]; // LCD 출력용 문자열 임시 버퍼 공간 (16글자 + NULL)

	// 스위치 1, 2번 포트 설정: PORTC (PC0, PC1) 입력 방향 지정 및 내부 풀업 저항 활성화
	DDRC &= ~((1 << PC0) | (1 << PC1));
	PORTC |= ((1 << PC0) | (1 << PC1));

	// 스위치 3, 4번 포트 설정: PORTD (PD2, PD3) 입력 방향 지정 및 내부 풀업 저항 활성화
	DDRD &= ~((1 << PD2) | (1 << PD3));
	PORTD |= ((1 << PD2) | (1 << PD3));

	// 하드웨어 통신 및 모듈 초기화 수행
	TWI_init();
	LCD_init();

	// 첫 번째 줄 맨 앞 자리에 개인 이니셜 출력
	LCD_position(0, 0);
	LCD_string("KYJ");

	while (1) {
		// 1번 스위치 (PC0 연동): 첫 번째 숫자(A) 1씩 증가 제어
		if (get_button_press(&PINC, PC0)) {
			val_a++;
			if (val_a > 9) val_a = 0; // 숫자가 9를 넘어가면 0으로 순환 초기화
			while (!(PINC & (1 << PC0))); // 버튼을 손에서 뗄 때까지 무한 대기 (중복 인식 방지)
		}

		// 2번 스위치 (PC1 연동): 사칙연산자 순서대로 변경 제어 ('+' -> '-' -> '*' -> '/')
		if (get_button_press(&PINC, PC1)) {
			op_index = (op_index + 1) % 4; // 인덱스를 0~3 사이로 순환 증가
			while (!(PINC & (1 << PC1))); // 버튼 뗄 때까지 대기
		}

		// 3번 스위치 (PD2 연동): 두 번째 숫자(B) 1씩 증가 제어
		if (get_button_press(&PIND, PD2)) {
			val_b++;
			if (val_b > 9) val_b = 0; // 숫자가 9를 넘어가면 0으로 순환 초기화
			while (!(PIND & (1 << PD2))); // 버튼 뗄 때까지 대기
		}

		// 4번 스위치 (PD3 연동): 연산 수행 및 결과값 도출 제어
		if (get_button_press(&PIND, PD3)) {
			switch (op_list[op_index]) {
				case '+': result = val_a + val_b; break; // 덧셈 연산 수행
				case '-': result = val_a - val_b; break; // 뺄셈 연산 수행
				case '*': result = val_a * val_b; break; // 곱셈 연산 수행
				case '/':
				if (val_b != 0) result = val_a / val_b; // 0으로 나누기 예외 상황 방지 처리
				else result = 0;
				break;
			}
			while (!(PIND & (1 << PD3))); // 버튼 뗄 때까지 대기
		}

		// LCD 두 번째 줄에 'A 연산자 B = 결과값' 형태로 계산식 실시간 포맷팅 및 출력
		LCD_position(1, 0); // 커서 위치를 두 번째 줄 첫 번째 칸으로 이동
		sprintf(lcd_buffer, "%d %c %d = %-6d", val_a, op_list[op_index], val_b, result); // 문자열 조합
		LCD_string(lcd_buffer); // LCD 화면에 문자열 최종 출력

		_delay_ms(50); // 시스템 구동 주기 및 안정성을 위한 짧은 지연 시간 대기
	}

	return 0;
}