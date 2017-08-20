#ifndef SETTINGS_H_
#define SETTINGS_H_

#define BAUD 9600
#define MYUBRR F_CPU/16UL/BAUD-1

#define begin_shift PORTD &= ~(1<<latchPin);
#define end_shift PORTD |= (1<<latchPin);

#define PRESCALER 1024 // Предделитель

#define clockPin	PD2		// Тактовый вход
#define latchPin	PD3		// Защелка
#define dataPin		PD4		// Данные
#define button1  1<<PD5		// Кнопка режима настройки часового пояса
#define button2  1<<PD6		// Кнопка режима отображения секунд
#define buttonsPins (button1 | button2)
#define tiltSensor1 (PIND>>PD7)&1
#define tiltSensor2 (PINB>>PB0)&1

#define minus			(1<<G)
#define dpoint			(1<<DP)

#define zero			(1<<A)|(1<<B)|(1<<C)|(1<<D)|(1<<E)|(1<<F)
#define one				(1<<B)|(1<<C)
#define two				(1<<A)|(1<<B)|(1<<D)|(1<<E)|(1<<G)
#define three			(1<<A)|(1<<B)|(1<<C)|(1<<D)|(1<<G)
#define four			(1<<B)|(1<<C)|(1<<F)|(1<<G)
#define five			(1<<A)|(1<<C)|(1<<D)|(1<<F)|(1<<G)
#define six				(1<<A)|(1<<C)|(1<<D)|(1<<E)|(1<<F)|(1<<G)
#define seven			(1<<A)|(1<<B)|(1<<C)
#define eight			(1<<A)|(1<<B)|(1<<C)|(1<<D)|(1<<E)|(1<<F)|(1<<G)
#define nine			(1<<A)|(1<<B)|(1<<C)|(1<<D)|(1<<F)|(1<<G)

#define zero_flipped	zero
#define one_flipped		(1<<E)|(1<<F)
#define two_flipped		two
#define	three_flipped	(1<<A)|(1<<D)|(1<<E)|(1<<F)|(1<<G)
#define four_flipped	(1<<C)|(1<<E)|(1<<F)|(1<<G)
#define five_flipped	five
#define six_flipped		nine
#define seven_flipped	(1<<D)|(1<<E)|(1<<F)
#define eight_flipped	eight
#define nine_flipped	six

#define symbol_H		(1<<B)|(1<<C)|(1<<E)|(1<<F)|(1<<G)
#define symbol_E		(1<<A)|(1<<D)|(1<<E)|(1<<F)|(1<<G)
#define symbol_L		(1<<D)|(1<<E)|(1<<F)
#define symbol_O		zero

#define symbol_H_flipped	symbol_H
#define symbol_E_flipped	three
#define symbol_L_flipped	seven
#define symbol_O_flipped	zero

#define displayDigit1		1<<0
#define displayDigit2		1<<1
#define displayDigit3		1<<2
#define displayDigit4		1<<3

#endif 