#define F_CPU 16000000UL // CPU 클럭 16MHz 설정

#include <avr/io.h>
#include <util/delay.h>

#define LCD_ADDR 0x27 // I2C LCD 모듈 주소

// 1. I2C 통신 기본 함수들 (TWI 모듈 설정)
void TWI_init(void) {
	TWSR = 0x00; // 분주비 기본 1 설정
	TWBR = 72;   // 전송 속도 100kHz 설정
	TWCR = (1 << TWEN); // TWI(I2C) 통신 기능 활성화
}

void TWI_start(void) {
	// I2C 통신 시작(START) 조건 발생
	TWCR = (1 << TWINT) | (1 << TWSTA) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT))); // 처리 완료 대기
}

void TWI_write(unsigned char data) {
	TWDR = data; // 전송할 데이터 레지스터 저장
	// 데이터 전송 시작
	TWCR = (1 << TWINT) | (1 << TWEN);
	while (!(TWCR & (1 << TWINT))); // 전송 완료 플래그 대기
}

void TWI_stop(void) {
	// I2C 통신 종료(STOP) 신호 전송
	TWCR = (1 << TWINT) | (1 << TWSTO) | (1 << TWEN);
}

// 2. LCD 제어 함수들 (I2C를 통한 LCD 데이터 전송)
void LCD_I2C_write(unsigned char data) {
	TWI_start();                // 통신 시작
	TWI_write(LCD_ADDR << 1);   // LCD 주소 쓰기 모드 전송
	TWI_write(data);            // 실제 데이터 전송
	TWI_stop();                 // 통신 종료
}

void LCD_pulse(unsigned char data) {
	// 데이터 정상 인식을 위한 Enable 핀 펄스(HIGH -> LOW) 생성
	LCD_I2C_write(data | 0x04); // EN = High (0x04 비트 켜기)
	_delay_us(1);               // 짧은 유지 시간
	LCD_I2C_write(data & ~0x04); // EN = Low (0x04 비트 끄기)
	_delay_us(50);              // LCD 처리 대기 시간
}

void LCD_send4bit(unsigned char data) {
	// 4비트 분할 데이터 전송 및 펄스 발생
	LCD_I2C_write(data);
	LCD_pulse(data);
}

void LCD_command(unsigned char command) {
	// 1바이트 명령어를 상/하위 4비트로 분할 전송
	unsigned char high = command & 0xF0;
	unsigned char low = (command << 4) & 0xF0;
	// 백라이트(0x08) 비트 결합 전송
	LCD_send4bit(high | 0x08);
	LCD_send4bit(low | 0x08);
}

void LCD_data(unsigned char data) {
	// 출력용 문자 데이터 상/하위 4비트 분할
	unsigned char high = data & 0xF0;
	unsigned char low = (data << 4) & 0xF0;
	// 데이터 모드 RS 핀(0x01) 활성화 결합 (0x08 + 0x01 = 0x09)
	LCD_send4bit(high | 0x09);
	LCD_send4bit(low | 0x09);
}

void LCD_init(void) {
	_delay_ms(50); // 전원 안정화 대기
	
	// 4비트 모드 진입 초기화 시퀀스
	LCD_send4bit(0x30 | 0x08);
	_delay_ms(5);
	LCD_send4bit(0x30 | 0x08);
	_delay_us(200);
	LCD_send4bit(0x30 | 0x08);
	_delay_us(200);
	LCD_send4bit(0x20 | 0x08); // 4비트 통신 전환
	_delay_us(200);

	// 디스플레이 기본 설정 명령어 전달
	LCD_command(0x28); // 2줄 표시, 5x8 폰트 설정
	LCD_command(0x0C); // 디스플레이 켜기, 커서 끄기
	LCD_command(0x06); // 커서 우측 이동 설정
	LCD_command(0x01); // 화면 전체 클리어
	_delay_ms(2);      // 클리어 명령어 처리 대기
}

void LCD_string(char *text) {
	// 문자열 종료 문자('\0') 도달까지 순차적 출력
	while (*text) {
		LCD_data(*text);
		text++;
	}
}

void LCD_position(unsigned char row, unsigned char column) {
	// 행/열 좌표 기반 커서 이동 위치 지정
	if (row == 0) LCD_command(0x80 + column); // 첫 번째 줄 주소
	else LCD_command(0xC0 + column);          // 두 번째 줄 주소
}

void LCD_number(unsigned int number) {
	// 4자리 숫자 자리수별 분할 및 문자 변환 출력
	LCD_data((number / 1000) % 10 + '0'); // 천의 자리
	LCD_data((number / 100) % 10 + '0');  // 백의 자리
	LCD_data((number / 10) % 10 + '0');   // 십의 자리
	LCD_data(number % 10 + '0');          // 일의 자리
}

// 3. ADC(가변저항) 설정 및 읽기 함수들
void ADC_init(void) {
	// 기준 전압 AVCC(5V), 아날로그 핀 ADC0(PF0) 지정
	ADMUX = (1 << REFS0);
	// ADC 활성화(ADEN) 및 분주비 128 설정
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

unsigned int ADC_read(void) {
	ADCSRA |= (1 << ADSC);             // 아날로그-디지털 변환 시작
	while (ADCSRA & (1 << ADSC));     // 변환 완료 대기
	return ADC;                       // 10비트 변환값 반환
}

// 4. 메인 실행부
int main(void) {
	unsigned int adc_val;  // 가변저항 원본 입력값
	unsigned int voltage;  // 전압 환산값 저장
	unsigned char led_pos; // LED 점등 위치 저장

	DDRA = 0xFF;  // PORTA 출력 방향 설정 (LED용)
	PORTA = 0xFF; // 전체 소등 초기화 (Active-Low)

	// 하드웨어 모듈 초기화 수행
	TWI_init();
	LCD_init();
	ADC_init();

	// LCD 첫 번째 줄 이니셜 출력
	LCD_position(0, 0);
	LCD_string("KYJ");

	while (1) {
		adc_val = ADC_read(); // 가변저항 입력값 독출

		// ADC 값 기반 LED 점등 위치 계산
		led_pos = adc_val / 128;
		if (led_pos > 7) led_pos = 7; // 범위 제한 예외 처리
		PORTA = ~(1 << led_pos);      // 해당 LED 점등 (Active-Low 반전)

		// 1023 기준값 5.0V 전압 환산 연산
		voltage = (unsigned long)adc_val * 50 / 1023;

		// LCD 두 번째 줄 커서 이동
		LCD_position(1, 0);
		
		LCD_number(adc_val); // ADC 원본 값 출력
		LCD_data('       ');       // 여백 공백 출력
		
		// 전압 정수부, 소수점, 소수 첫째 자리, 단위 출력
		LCD_data((voltage / 10) + '0'); // 전압 정수부
		LCD_data('.');                   // 소수점
		LCD_data((voltage % 10) + '0'); // 전압 소수 첫째 자리
		LCD_data('V');                   // 단위 'V'
		
		LCD_string("     "); // 이전 데이터 잔상 제거용 공백 처리

		_delay_ms(100); // 0.1초 주기 갱신 딜레이
	}

	return 0;
}