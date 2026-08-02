#define F_CPU 16000000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include "LCD_Text.h"


typedef enum {//단계에 대한 구조체
	YEAR = 0,  // 1단계: 연도 맞추는 중
	MONTH,      // 2단계: 월 맞추는 중
	DAY,        // 3단계: 일 맞추는 중
	HOUR,       // 4단계: 시 맞추는 중
	MIN,        // 5단계: 분 맞추는 중
	SEC,        // 6단계: 초 맞추는 중
	MSEC,       // 7단계: 밀리초(1/100초) 맞추는 중
	SHOW_ALL,   // 8단계: 다 맞췄으니 화면에 정지 화면으로 보여주는 중 (SW2 대기)
	RUN_CLOCK   // 9단계: 진짜로 시간이 흘러가는 중
} SetState;

SetState current_state = YEAR;   // 처음에 켜지면 연도 맞추기부터 시작

//날짜/시간 저장하는 변수들
int year   = 0;   // 00~99년
int month  = 1;   // 01~12월
int day    = 1;   // 01~그 달의 마지막 날
int hour   = 0;   // 00~23시
int minute = 0;   // 00~59분
int second = 0;   // 00~59초
int msec   = 0;   // 00~99 (1/100초 자리)

// 스위치 이전 상태 저장용 (버튼이 방금 눌렸나 확인하려고)
uint8_t sw1_prev = 1;   // 풀업 저항이라 평소엔 안 누르면 1(HIGH)
uint8_t sw2_prev = 1;

//가변저항 관련 함수
void ADC_Init(void)
{
	ADMUX = (1 << REFS0);
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // ADC 전원 켜고, 128인 분주
}

uint16_t Read_ADC(void) // 가변저항 돌린 값을 0~1023 사이 숫자로 한 번 읽어오는 함수
{
	ADCSRA |= (1 << ADSC); // 변환 시작
	while (ADCSRA & (1 << ADSC)); // 다 읽을 때까지 기다리기
	return ADC;
}

// 가변저항 값이 미세하게 떨려서(노이즈 때문에) 숫자가 혼자 작동하는 것 막기
uint16_t Read_ADC_Avg(void)
{
	uint32_t sum = 0;
	for (uint8_t i = 0; i < 8; i++) {//8번 반복
		sum += Read_ADC();
	}
	return (uint16_t)(sum / 8);//평균으로 구하기
}

//TIMER0 : 1ms(밀리초)마다 인터럽트 걸기
volatile uint8_t ms_tick_count = 0;          // 1ms 지날 때마다 1씩 올라가는 누적 변수

void Timer0_Init(void)
{
	TCCR0 = (1 << WGM01) | (1 << CS01) | (1 << CS00); // 64분주 기어 넣기
	OCR0 = 249;                              // 0부터 249까지(총 250번) 세면 비교 일치
	TIMSK |= (1 << OCIE0);                   // 비교 일치 인터럽트 허용 (TIMSK 레지스터 건드리기)
}

ISR(TIMER0_COMP_vect) // 타이머 0이 1ms마다 알아서 자동으로 불러주는 함수
{
	ms_tick_count++; // 1ms 지났을 때 증가
}

// 스위치 설정 (SW1=PC0, SW2=PC1)
void Switch_Init(void)
{
	DDRC &= ~((1 << PC0) | (1 << PC1));// 스위치 핀들은 입력 모드로 설정
	PORTC |= (1 << PC0) | (1 << PC1);  // 내부 풀업 저항 켜서 평소에 1 유지하게 만들기
}

void Show_DateTime(void); //LCD에 값 띄우는 함수 호출

// 달마다 마지막 날이 며칠인지 계산해주는 함수 (윤년이면 2월을 29일로)
int Get_Max_Day(int y, int m)
{
	int days_table[] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	if (m == 2 && (y % 4 == 0)) return 29;   // 4년마다 돌아오는 윤년이면 2월은 29일
	if (m >= 1 && m <= 12) return days_table[m];
	return 31;
}

//SW1, SW2 버튼이 막 눌리는 순간을 캐치해서 다음 단계로 넘겨주는 함수
void Check_Switches(void)
{
	//SW1 체크 (다음 설정 단계로 넘어갈 때 씀)
	uint8_t sw1_now = (PINC & (1 << PC0)) ? 1 : 0;
	if (sw1_prev == 1 && sw1_now == 0) {  // 평소엔 안 눌렸다가(1) 지금 막 눌렸으면(0)
		_delay_ms(20);// 손가락 떨리는거 20ms 기다려주기
		if (!(PINC & (1 << PC0))) { // 잠깐 기다렸는데도 진짜 눌려있다면
			if (current_state < SHOW_ALL) { // 설정하는 동안에는 누를 때마다 다음 단계로 전진
				current_state++;
				
				// 다음 단계로 넘어가자마자 숫자가 01부터 시작하게 세팅
				switch (current_state) {
					case MONTH: month  = 1; break;
					case DAY:   day    = 1; break;
					case HOUR:  hour   = 1; break;
					case MIN:   minute = 1; break;
					case SEC:   second = 1; break;
					case MSEC:  msec   = 1; break;
					case SHOW_ALL:                          // 설정이 드디어 끝났을 때
					lcdClear();                         // 화면에 남아있던 것 지우고
					Show_DateTime();                    // 지금까지 맞춘 날짜/시간 전체 보여주기
					break;
					default: break;
				}
			}
		}
	}
	sw1_prev = sw1_now;

	// SW2 체크 (시계를 진짜로 움직이게 시작할 때 씀)
	uint8_t sw2_now = (PINC & (1 << PC1)) ? 1 : 0;
	if (sw2_prev == 1 && sw2_now == 0) {          // SW2가 방금 눌렸으면
		if (!(PINC & (1 << PC1))) {
			if (current_state == SHOW_ALL) {      // 전체 보기 화면 상태일 때만 시계 출발 허용
				current_state = RUN_CLOCK;
			}
		}
	}
	sw2_prev = sw2_now;
}

// 시간을 1/100초씩 올려주면서 시계 자리올림함수
void Add_Hundredth_Second(void)
{
	msec++;
	if (msec >= 100) {         // 밀리초가 100이 되면 (즉, 1초가 되면)
		msec = 0;
		second++;
		if (second >= 60) {    // 초가 60이 되면
			second = 0;
			minute++;
			if (minute >= 60) { // 분이 60이 되면
				minute = 0;
				hour++;
				if (hour >= 24) { // 시간이 24가 되면 (하루가 지나면)
					hour = 0;
					day++;
					int max_d = Get_Max_Day(year, month);
					if (day > max_d) { // 그 달의 마지막 날을 넘어가면 다음 달로
						day = 1;
						month++;
						if (month > 12) { // 12월이 넘어가면 새해로
							month = 1;
							year++;
							if (year > 99) year = 0; // 2자리 연도라 99년 다음은 다시 00년으로 리셋
						}
					}
				}
			}
		}
	}
}

//LCD에 값 출력하는 함수
void Show_DateTime(void)
{
	char line1[17];
	char line2[17];
	sprintf(line1, "%02d%02d%02d          ", year, month, day);          // 윗줄: 연월일 (예: 190722)
	sprintf(line2, "%02d:%02d:%02d.%02d     ", hour, minute, second, msec); // 아랫줄: 시분초.밀리초
	lcdString(1, 1, line1);//첫번째 줄 첫자리에
	lcdString(2, 1, line2);//두번째 줄 첫자리에
}

int main(void)
{
	lcdInit(); // LCD 화면 켜기
	ADC_Init(); // 가변저항 준비 완료
	Switch_Init();// 스위치 준비 완료
	Timer0_Init(); // 1ms마다 일하는 타이머 0 준비 완료
	sei();// 전체 인터럽트 허용 스위치

	while (1) {
		Check_Switches();  // 버튼이 눌렸는지 계속 감시하기

		if (current_state < SHOW_ALL) {
			uint16_t adc_val = Read_ADC_Avg();  // 노이즈 뺀 깔끔한 가변저항 평균값 가져오기
			char buf[17];

			switch (current_state) {
				case YEAR:
				year = (adc_val * 100) / 1024;          // 0~1023을 0~99 범위로 바꿔서 연도에 대입
				sprintf(buf, "YEAR  %02d", year);
				break;
				case MONTH:
				month = (adc_val * 12) / 1024 + 1;      // 1~12 범위로 바꿔서 월에 대입
				sprintf(buf, "MONTH %02d", month);
				break;
				case DAY: {
					int max_d = Get_Max_Day(year, month);   // 그 달의 마지막 날짜 확인
					day = (adc_val * max_d) / 1024 + 1;     // 1~마지막날 범위로 바꿔서 일에 대입
					sprintf(buf, "DAY   %02d", day);
					break;
				}
				case HOUR:
				hour = (adc_val * 24) / 1024;           // 0~23 범위로 바꿔서 시에 대입
				sprintf(buf, "HOUR  %02d", hour);
				break;
				case MIN:
				minute = (adc_val * 60) / 1024;         // 0~59 범위로 바꿔서 분에 대입
				sprintf(buf, "MIN   %02d", minute);
				break;
				case SEC:
				second = (adc_val * 60) / 1024;         // 0~59 범위로 바꿔서 초에 대입
				sprintf(buf, "SEC   %02d", second);
				break;
				case MSEC:
				msec = (adc_val * 100) / 1024;          // 0~99 범위로 바꿔서 밀리초에 대입
				sprintf(buf, "MSEC  %02d", msec);
				break;
				default:
				break;
			}
			lcdString(1, 1, buf); // 현재 맞추고 있는 값을 LCD 첫 번째 줄에 실시간으로 보여주기
		}
		else if (current_state == SHOW_ALL) {
			//값 다 맞추고 SW2 누르기 기다림
			
		}
		else {
			//시간이 흐르는 진짜 시계 작동 중
			uint8_t updated = 0;               // 이번 바퀴에 화면을 새로 고쳐야 하는지 체크
			
			while (ms_tick_count >= 20) {      // 타이머가 쌓아둔 1ms 통장에서 10ms(=1/100초)만큼 쏙 빼오기
				ms_tick_count -= 20;           // 통장에서 10만큼 차감
				Add_Hundredth_Second();        // 시간을 1/100초만큼 앞으로 전진시키기
				updated = 1;                   // 시간이 바뀌었으니 화면 갱신 필요하다고 신호 보내기
			}
			
			if (updated) {                     // 시간이 변했을 때만
				Show_DateTime();               // LCD 화면에 최신 날짜와 시간을 갱신하기
			}
		}
	}
	return 0;
}