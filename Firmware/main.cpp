#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <string.h>
#include <stdlib.h>
#include "settings.h"


enum clockMode { clockTime, clockSetTimezone, clockSeconds };
enum displayMode { displayNormal, displayFlipped };
enum segments { A, B, C, D, E, F, G, DP };

uint8_t displayDigits[][4] = {
								{ displayDigit1, displayDigit2, displayDigit3, displayDigit4 },
								{ displayDigit4, displayDigit3, displayDigit2, displayDigit1 }
							};

uint8_t symbols[][10] = {
							{ zero, one, two, three, four, five, six, seven, eight, nine },
							{ zero_flipped, one_flipped, two_flipped, three_flipped, four_flipped, five_flipped, six_flipped, seven_flipped, eight_flipped, nine_flipped }
						};

uint8_t hello[][10][4] = {
							{
								{ minus, minus, minus, minus },
								{ symbol_H, minus, minus, minus },
								{ symbol_E, symbol_H, minus, minus },
								{ symbol_L, symbol_E, symbol_H, minus },
								{ symbol_L, symbol_L, symbol_E, symbol_H },
								{ symbol_O, symbol_L, symbol_L, symbol_E },
								{ minus, symbol_O, symbol_L, symbol_L },
								{ minus, minus, symbol_O, symbol_L },
								{ minus, minus, minus, symbol_O },
								{ minus, minus, minus, minus }
							},
							{
								{ minus, minus, minus, minus },
								{ symbol_H_flipped, minus, minus, minus },
								{ symbol_E_flipped, symbol_H_flipped, minus, minus },
								{ symbol_L_flipped, symbol_E_flipped, symbol_H_flipped, minus },
								{ symbol_L_flipped, symbol_L_flipped, symbol_E_flipped, symbol_H_flipped },
								{ symbol_O_flipped, symbol_L_flipped, symbol_L_flipped, symbol_E_flipped },
								{ minus, symbol_O_flipped, symbol_L_flipped, symbol_L_flipped },
								{ minus, minus, symbol_O_flipped, symbol_L_flipped },
								{ minus, minus, minus, symbol_O_flipped },
								{ minus, minus, minus, minus }
							}
						};

uint8_t buffTime[8] = {0xff, 0xff , 0xff, 0xff};
uint8_t buffTimezone[4] = {0xff, 0xff , 0xff, 0xff};

uint8_t currentDisplayMode = displayNormal;
uint8_t currentClockMode = clockTime;
uint8_t EEMEM _timezone = 5;
uint8_t timezone;
uint8_t GPSTimeHour;
uint8_t GPSTimeMin;
uint8_t GPSTimeSec;

char bufferUSART[82];
volatile bool NMEAMessageReceived = false;
volatile bool GPSSynchronizeTime = false;
volatile bool startSyncTime = true;

bool point = false;
bool splash = true;

void shift(uint8_t data);
void splashscreen();
void drawtime();
void drawdigit(uint8_t digit, uint8_t num);
inline void button1Pressed();
inline void button2Pressed();
inline void StartDrawingTimer();
inline void StartClockingTimer();
inline void LoadTimezone();
inline void SaveTimezone();
inline void USART_Init(uint8_t ubrr);
inline void checkTiltSensors();
unsigned char USART_Receive();
bool parseNMEAMessage();
bool CheckGPS();
inline void syncTime();

int main(void)
{	
	uint8_t oldStatePinD = 0;
	uint8_t statePinD = 0;
	uint8_t	delayState = 0;

	// Конфигурируем пины в состояние Output
	DDRD = (1<<clockPin) | (1<<latchPin) | (1<<dataPin);

	// Ощичаем первый сдвиговый регистр
	begin_shift
	shift(0);
	end_shift

	// Инициализация USART 
	USART_Init(MYUBRR);

	// Загрузка часового пояса из EEPROM
	LoadTimezone();

	// Разрешаем общие прерывания
	sei();

	// Запускаем таймер отрисовки
	StartDrawingTimer();

	// Первая синхронизация GPS
	while (!CheckGPS());

	// Запускаем таймер отсчета времени
	StartClockingTimer();

    while (1) 
    {
		CheckGPS();
		
		statePinD = PIND&buttonsPins;

		if (statePinD == oldStatePinD)
		{
			if (delayState < 5)
			delayState++;
			else if (delayState == 5)
			{
				switch(statePinD)
				{
					case button1: button1Pressed(); break;
					case button2: button2Pressed(); break;
				}

				delayState++;
			}
		}
		else
		{
			oldStatePinD = statePinD;
			delayState = 0;
		}
			
		_delay_ms(10);
    }
}

ISR(TIMER2_OVF_vect)
{
	uint8_t static csec = 0;
	uint8_t static sec = 0;
	uint8_t static min = 0;
	uint8_t static hour = 0;
	uint8_t _hour;

	if (GPSSynchronizeTime)
	{
		GPSSynchronizeTime = false;
		hour = GPSTimeHour;
		min = GPSTimeMin;
		sec = GPSTimeSec;
	}

	point ^= true;
	csec += 25;
	if (csec>=100)
	{
		csec = 0;
		sec++;
		if(sec >= 60)
		{
			sec = 0;
			min++;
			if (min >= 60)
			{
				syncTime();
				min = 0;
				hour++;
				if (hour>=24)
				hour = 0;
			}
		}
	}
		
	_hour = hour + timezone;

	if (_hour >= 24) _hour -= 24;

	buffTime[0] = min % 10;
	buffTime[1] = min / 10;

	buffTime[2] = _hour % 10;
	buffTime[3] = _hour / 10;

	buffTime[4] = csec % 10;
	buffTime[5] = csec / 10;

	buffTime[6] = sec % 10;
	buffTime[7] = sec / 10;
}

ISR(TIMER1_COMPA_vect)
{
	checkTiltSensors();

	if (splash)
		splashscreen();
	else
		drawtime();
}

void splashscreen()
{
	static uint8_t currentdigit = 0;
	static uint8_t frame = 0;
	static uint8_t count = 0;

	begin_shift
	shift(~(hello[currentDisplayMode][frame][currentdigit]));
	shift(displayDigits[currentDisplayMode][currentdigit]);
	end_shift

	count++;
	if (count == 40)
	{
		count = 0;
		if(frame == 9)
			splash = false;
		else
			frame++;
	}

	currentdigit++;
	currentdigit &= 3;
}

void drawtime()
{
	static uint8_t currentdigit = 0;

	switch(currentClockMode)
	{
		case clockSeconds: drawdigit(currentdigit, buffTime[currentdigit+4]); break;
		case clockSetTimezone: drawdigit(currentdigit, buffTimezone[currentdigit]); break;
		default: drawdigit(currentdigit, buffTime[currentdigit]); break;
	}

	currentdigit++;
	currentdigit &= 3;
}

void drawdigit(uint8_t digit, uint8_t num)
{
	if (digit>4) return;

	begin_shift
	if (num>9)
		shift(~minus);
	else
		shift(~(symbols[currentDisplayMode][num] | ( currentClockMode != clockSetTimezone && displayDigits[currentDisplayMode][digit] == displayDigit3 && point ? dpoint : 0 )));
	shift(displayDigits[currentDisplayMode][digit]);
	end_shift
}

void shift(uint8_t data)
{
	for (int i = 0; i < 8; i++){
		PORTD &= ~((1<<clockPin)|(1<<dataPin));
		PORTD |= (((data&0x80)>>7)<<dataPin);
		PORTD |= (1<<clockPin);
		data <<= 1;
	}
}

inline void button1Pressed()
{
	currentClockMode = ((currentClockMode == clockTime) ? clockSetTimezone : clockTime);

	if (currentClockMode == clockSetTimezone)
		syncTime();
}

inline void button2Pressed()
{
	switch (currentClockMode)
	{
		case clockTime: currentClockMode = clockSeconds; break;
		case clockSeconds: currentClockMode = clockTime; break;
		case clockSetTimezone: 
			timezone = (timezone == 12) ? 2 : timezone+1;
			SaveTimezone();
			break;
	}
}

inline void StartDrawingTimer()
{
	// Прерывание таймера при совпадении
	TIMSK1 = (1<<OCIE1A);
	// Установка предделителя 1024
	TCCR1B = (1<<CS10)|(1<<CS12)|(1<<WGM12);
	// Установка значения регистра сравнения
	OCR1A = F_CPU/PRESCALER/200;
}

inline void StartClockingTimer()
{
	// Асинхронный режим таймеарц
	ASSR = (1<<AS2);
	// Установка предделителя частоты 32
	TCCR2B |= (1<<CS21) | (1<<CS20);
	// Прерывание таймера по переполнению счетчика
	TIMSK2 |= (1<<TOIE2);
}

inline void LoadTimezone()
{
	timezone = eeprom_read_byte(&_timezone);
	buffTimezone[0] = timezone % 10;
	buffTimezone[1] = timezone / 10;
}

inline void SaveTimezone()
{
	eeprom_write_byte(&_timezone, timezone);
	buffTimezone[0] = timezone % 10;
	buffTimezone[1] = timezone / 10;
}

inline void USART_Init(uint8_t ubrr)
{
	// Установка скорости 
	UBRR0H = ubrr >> 8;
	UBRR0L = ubrr & 0xFF;
	// Разрешение приема, передачи данных и
	// прерывания по завершению приема данных
	UCSR0B = (1<<RXEN0)|(1<<RXCIE0);
	// Установка режима работы USART: асинхронный, размер данных 8 бит,
	//  1 стоп бит, бит четности выключен
	UCSR0C = (3<<UCSZ00);
} inline bool parseNMEAMessage(){	uint8_t length = 0;	uint8_t checkSum = 0;	uint8_t _checkSum = 0;	char* pTime = NULL;	if (bufferUSART[0] == '$')	{		length = (uint8_t)strlen(bufferUSART);				if (length > 10)		{			if (strstr(bufferUSART, "$GPGGA") != NULL ||				strstr(bufferUSART, "$GPRMC") != NULL)			{				checkSum = strtoul(&bufferUSART[length-2], NULL, 16);							if (checkSum != 0)				{					// Время					pTime = strstr(bufferUSART, ",");										if (pTime != NULL)					{						pTime++;						for (uint8_t i = 0; i<6; i++)						{							if (pTime[i] < '0' || pTime[i] > '9')								return false;						}						GPSTimeHour = (pTime[0]-'0')*10+(pTime[1]-'0');						GPSTimeMin = (pTime[2]-'0')*10+(pTime[3]-'0');						GPSTimeSec = (pTime[4]-'0')*10+(pTime[5]-'0');						// Подсчет контрольной суммы между $ и *						for (int i = 1; i < length-3; i++) {
							_checkSum ^= bufferUSART[i];
						}						if (_checkSum == checkSum)							return true;					}				}			}		}	}	return false;}inline void syncTime(){	NMEAMessageReceived = false;	startSyncTime = true;}inline void checkTiltSensors(){	if (!tiltSensor1 && tiltSensor2)
	{
		currentDisplayMode = displayNormal;
	}
	else if (tiltSensor1 && !tiltSensor2)
	{
		currentDisplayMode = displayFlipped;
	}}bool CheckGPS(){	if (!startSyncTime) return false;	if (!NMEAMessageReceived) return false;
	if (parseNMEAMessage())
	{
		startSyncTime = false;
		GPSSynchronizeTime = true;
		return true;
	}
	else
	{
		NMEAMessageReceived = false;
		return false;
	}}ISR(USART_RX_vect){	static uint8_t bufferCount = 0;	char data = UDR0;		if (!NMEAMessageReceived)	{		if (bufferCount >= sizeof(bufferUSART))			bufferCount = 0;				if (data == '$')		{			bufferCount = 0;			bufferUSART[bufferCount] = data;			bufferCount++;		}		else if (data == '\r')		{			bufferUSART[bufferCount] = 0;			bufferCount = 0;			NMEAMessageReceived = true;		}		else		{			bufferUSART[bufferCount] = data;			bufferCount++;		}	}}