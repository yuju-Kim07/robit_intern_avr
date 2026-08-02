#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>


#define ADC_NEAR   3     // 가장 가까이 댔을 때 ADC 값
#define ADC_FAR    532   // 유효 최대 거리에서 ADC 값

#define DISTANCE_AT_NEAR_CM  0   // ADC_NEAR일 때 실제 거리(cm)
#define DISTANCE_AT_FAR_CM   20   // ADC_FAR일 때 실제 거리(cm)

#define SENSOR_ERROR_MIN   2     // 예외 처리의 경계선
#define SENSOR_ERROR_MAX   533  // 예외 처리의 경계선

#define MEASURE_PERIOD 1000//측정 주기 = 1

#define ADC_SAMPLE_COUNT 5//평균을 구하기 위해 5개의 값 쓰기


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

void ADC_init(void)
{
	ADMUX = (1 << REFS0); // AVCC 기준전압. (0V~5V까지-->0~1023)
	ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	//ADCSRA: ADC제어, 상태 레지스터.
	//ADEN=1: ADC회로를 켜기
	//ADPS2~0=1: 너무 빠르지 않게 분주비를 128로 설정
}

uint16_t ADC_read(uint8_t channel) //16비트에 저장하여서, 8비트가 다 저장될 수 있도록.  ADC를 읽는 함수
{
	ADMUX = (ADMUX & 0xE0) | (channel & 0x1F);//레지스터의 하위 5비트(0001 1111)이 채널의 번호.  (1110 0000)과 &로 연결시켜서 0000 0000로 만들고 새 채널 입력받기
	ADCSRA |= (1 << ADSC);//변환 시작
	while (ADCSRA & (1 << ADSC));//ADSC가 0(변환 끝)이 될때까지 반복
	return ADC;//변환된 값을 ADC에 저장
}


int16_t ADC_to_distance_cm(uint16_t adc_val)//ADC를 CM로 환산하기
{
	if (adc_val < ADC_NEAR) adc_val = ADC_NEAR;
	if (adc_val > ADC_FAR)  adc_val = ADC_FAR;

	int32_t between_adc  = ADC_FAR - ADC_NEAR;//ADC의 범위
	int32_t between_dist = DISTANCE_AT_FAR_CM - DISTANCE_AT_NEAR_CM;//실제로 측정되는 거리 범위

	int32_t distance = DISTANCE_AT_NEAR_CM
	+ (int32_t)(adc_val - ADC_NEAR) * between_dist / between_adc;//ADC값을 실제로 측정되는 거리 값으로 치환하기. 치환하게되는 과정에서 값이 너무 커지므로 임시로 32비트로 설정

	return (int8_t)distance;//실제 거리 자체는 값이 크지 않기(약 20cm) 때문에 8비트로 돌아옴
}

void ADC_read_multi(uint8_t channel, uint16_t *out_raw, uint16_t *out_avg)//노이즈를 줄이려면 평균값을 산출해야 함

{
	uint16_t first;//첫번째 값을 필터링 안된 값으로 두기
	uint32_t sum = 0;//평균을 구하기 위한 합 변수

	for (uint8_t i = 0; i < ADC_SAMPLE_COUNT; i++)//5번 반복
	{
		uint16_t sample = ADC_read(channel);//5개의 값
		if (i == 0) first = sample;   // 5번 중 맨 처음 샘플을 필터링 안된 값으로 따로 저장
		sum += sample;
	}

	*out_raw = first;// 필터링 안 된 값 = 첫 샘플
	*out_avg = (uint16_t)(sum / ADC_SAMPLE_COUNT); // filtered = 5개 평균
}

int main(void)
{
	char buf[64];//문자열을 시리얼로 보내기 위해

	UART0_Init();//pc와 시리얼 통신의 원활한 수,송신을 위한 함수 호출
	ADC_init();//ADC 시작하기 위한 함수
	
	while (1)
	{
		uint16_t adc_raw, adc_raw_filtering;
		ADC_read_multi(1, &adc_raw, &adc_raw_filtering);

		if (adc_raw_filtering <= SENSOR_ERROR_MIN || adc_raw_filtering >= SENSOR_ERROR_MAX)
		{
			Put("DISTANCE_OUT\n"); // 비정상 센서 데이터 예외처리: 거리 계산 대신 에러 메시지 출력
		}
		else
		{
			int8_t distance_cm_avg = ADC_to_distance_cm(adc_raw_filtering);

			sprintf(buf, "RAW: %4d| FILTERED: %4d| DISTANCE: %4dcm\n", adc_raw, adc_raw_filtering, distance_cm_avg);
			Put(buf);
		}

		_delay_ms(MEASURE_PERIOD);
	}

	return 0;
}