#define F_CPU 16000000UL
#include <avr/io.h>
#include <util/delay.h>

void UART_Init(void)//PD3을 출력으로 설정하고, 평소엔 1(대기 상태)로 유지
{
	DDRD  |= (1 << PD3);   // PD3을 출력으로 설정
	PORTD |= (1 << PD3);   // UART는 평소에 HIGH 상태를 유지해야
}

//비트 1개를 보내는 함수. bit가 1이면 핀을 HIGH로, 0이면 LOW로 만들고
void UART_Send1(uint8_t bit)
{
	if (bit) {//비트가 1이라면
		PORTD |= (1 << PD3);   // 1 -> 핀을 HIGH로
	} else {
		PORTD &= ~(1 << PD3);  // 0 -> 핀을 LOW로
	}
	_delay_us(104);      // 이 상태로 1비트 시간만큼 유지( 1비트 길이(104us)만큼 그 상태를 유지해야지 1비트가 들어왔다고 여기기 때문에)

}


void UART_Send_All(uint8_t data)//문자열 글자를 전송하기 위한 함수
{
	UART_Send1(0);                 // 스타트 비트 : 항상 0 (여기서부터 데이터가 시작한다는 신호)(대기가 1이여서)

	for (uint8_t i = 0; i < 8; i++) {
		UART_Send1(data & 0x01);   // 제일 아래 비트(LSB)부터 순서대로 전송
		data = data >> 1;            // 다음 비트를 아래로 내림
	}

	UART_Send1(1);                 // 스톱 비트 : 항상 1 (여기서 한 바이트가 끝났다는 신호)
}

//문자열을 한 글자씩 UART_Send_All로 전송
void UART_SendString(const char *str)
{
	while (*str != '\0') {//NULL이 아닐때 반복
		UART_Send_All((uint8_t)(*str));
		str++;//배열의 다음 원소값 보기
	}
}

int main(void)
{
	UART_Init();

	while (1) {//무한반복
		UART_SendString("HelloWorld! "); 
		_delay_ms(1000);// 1초 대기
	}
}