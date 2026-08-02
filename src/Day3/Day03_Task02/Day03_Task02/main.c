#define F_CPU 16000000
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

//문자열을 pc로 전송하는 함수
//pc에서 입력한 숫자에 따라 led가 바뀌는 함수(0~9 이외의 숫자 입력시 예외)

void UART0_Init(void) //ATmega128과 pc와 시리얼 통신이 잘 되도록 하는 함수
{
	UBRR0H=0;
	UBRR0L=103;//속도를 9600으로 설정시키기 위한 식에서의 숫자
	
	UCSR0C=(1<<UCSZ01)|(1<<UCSZ00);//8비트로 지정& 비동기 방식 사용
	UCSR0B=(1<<TXEN0)|(1<<RXEN0);//송신, 수신 기능 활성화
}

void Putch(unsigned char data)//한 글자를 송신하는 함수
{
	while(!(UCSR0A&(1<<UDRE0)));//송신버퍼가 비어있을 때(1)까지 대기
	UDR0=data;//UDR0로 data 송신
}

void Put(char *str)//송신된 글자를 문자열만큼 출력하는 함수
{
	while(*str!='\0')//문자열이 NULL이 아닐때만 반복
	{
		Putch(*str);//한 글자를 송신하는 함수에 배열의 원소를 하나씩 집어 넣음
		str++;//배열 원소 주소 증가
	}
}

unsigned char Getch(void)//한 글자를 수신하는 함수
{
	while(!(UCSR0A&(1<<RXC0)));//수신할 데이터가 있을 때(1)까지 대기
	return UDR0;//수신한 글자를 반환
}

int main(void)
{
	UART0_Init();//원활한 통신을 위한 함수 호출
	
	DDRA=0xFF;//PORTA를 출력으로 사용(led)
	DDRC=0x00;//PORTC를 입력으로 사용(sw1)
	PORTC |= (1 << PC0);//플로팅 예방위해 풀업
	
	PORTA=0xFF;//led를 꺼두는 것으로 초기화
	
	while(1)//무한 반복
	{
		uint8_t sw1=!(PINC&(1<<PC0));//스위치를 0으로 되야 눌림으로 설정됨(B/C active low여서)
		
		if(sw1)//스위치1 누른다면
		{
			PORTA=0xFF;//led 끄는 것으로 초기화
			Put("RESET\n");//문자열로 pc에 보낼 수 있는 함수에 RESET 송신
			while(!(PINC&(1<<PC0)));//송신이 끝날 때까지 대기
			_delay_ms(300);//0.3초 대기
			continue;//while 조건문의 맨 앞으로 가기
		}
		
		if(UCSR0A&(1<<RXC0))//만약 수신이 준비되었다면
		{
			char num=Getch();//문자 변수 num에 수신받은 값 대입
			
			if(num=='0')//만약 num이 0이라면
			{
				PORTA=~(1<<PA0);// 0번 led 켜기
				Put("0 LED on\n");//pc에 "0 LED on" 송신
			}
			else if(num=='1')//만약 num이 1이라면
			{
				PORTA=~(1<<PA1);// 1번 led 켜기
				Put("1 LED on\n");//pc에 "1 LED on" 송신
			}
			else if(num=='2')//만약 num이 2이라면
			{
				PORTA=~(1<<PA2);// 2번 led 켜기
				Put("2 LED on\n");//pc에 "2 LED on" 송신
			}
			else if(num=='3')//만약 num이 3이라면
			{
				PORTA=~(1<<PA3);// 3번 led 켜기
				Put("3 LED on\n");//pc에 "3 LED on" 송신
			}
			else if(num=='4')//만약 num이 4이라면
			{
				PORTA=~(1<<PA4);// 4번 led 켜기
				Put("4 LED on\n");//pc에 "4 LED on" 송신
			}
			else if(num=='5')//만약 num이 5이라면
			{
				PORTA=~(1<<PA5);// 5번 led 켜기
				Put("5 LED on\n");//pc에 "5 LED on" 송신
			}
			else if(num=='6')//만약 num이 6이라면
			{
				PORTA=~(1<<PA6);// 6번 led 켜기
				Put("6 LED on\n");//pc에 "6 LED on" 송신
			}
			else if(num=='7')//만약 num이 7이라면
			{
				PORTA=~(1<<PA7);// 7번 led 켜기
				Put("7 LED on\n");//pc에 "7 LED on" 송신
			}
			else if(num=='8')//만약 num이 8이라면
			{
				for(int i=7; i>=0; i--)//오른쪽 끝에서 왼쪽 끝까지
				{
					PORTA=~(1<<i);//led가 순서대로 켜지기
					_delay_ms(300);
				}
				Put("LEFT\n");//pc에 "LEFT" 송신
			}
			else if(num=='9')//만약 num이 9이라면
			{
				for(int i=0; i<8; i++)//왼쪽 끝에서 오른쪽 끝까지
				{
					PORTA=~(1<<i);//led가 순서대로 켜지기
					_delay_ms(300);
				}
				Put("RIGHT\n");//pc에 "RIGHT" 송신
			}
			else
			{
				Put("잘못된 숫자를 입력했습니다.\n");
			}
		}
	}
	return 0;
}