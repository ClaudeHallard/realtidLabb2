#include "tinythreads.h"
#include <stdbool.h>
#include <avr/io.h>
#include <avr/interrupt.h>


typedef struct
{
	uint8_t first_pattern;
	uint8_t second_pattern;
	uint8_t third_pattern;
	uint8_t fourth_pattern;
}char_pattern_t;

	char_pattern_t DIGIT_BIT_PATTERNS[10];
	mutex initMutex = {0,0};

	
void LCD_Init(void)
	{
	
	//LCD enable, Low Power Waveform, Interupt Disabled, Blanking Disabled
	LCDCRA |= (1<<LCDEN) | (1<<LCDAB);
	LCDCRA &= ~(1<<LCDIE) & ~(1<<LCDBL);
		
	// Async clock source, 1/3 BIAS, 1/4 Duty, 25 Segments
	LCDCRB |= (1<<LCDCS) | (1<<LCDMUX1) | (1<<LCDMUX0) | (1<<LCDPM2) | (1<<LCDPM1) | (1<<LCDPM0);
	LCDCRB &= ~(1<<LCD2B);
		
	// Drive time 300 micro seconds, Voltage 3,35
	LCDCCR |= (1<<LCDCC3) | (1<<LCDCC2) | (1<<LCDCC1) | (1<<LCDCC0);
	LCDCCR &= ~(1<<LCDDC2) & ~(1<<LCDDC1) & ~(1<<LCDDC0);
		
	// D = 8, PreScaler 16
	LCDFRR |= (1<<LCDCD2) | (1<<LCDCD1) | (1<<LCDCD0);
	LCDFRR &= ~(1<<LCDPS2) & ~(1<<LCDPS1) & ~(1<<LCDPS0);
	}

// Write char to display (PART 1)
void writeChar(char ch, int pos)
{
	
	//Global pointer for address of LCDDR0, used as a base to offset from.
	volatile uint8_t* p_LCDDR0 = &LCDDR0;
	
	//If ch or pos is out of range just return.
	if(ch < '0' || ch > '9' || pos < 0 || pos > 5){
		return;
	}
	
	//Ascii '0' = 48, '1' = 49... (-48 to convert from char to integer).
	ch -= '0';
	
	//If pos is odd number we use upper Nibble, else lower Nibble.
	uint8_t uneven = pos%2;
	uint8_t shift = (uneven == 1) ? 4 : 0;
	
	//Pos = 0,1,2 for input 0-5.
	pos = pos/2;
	
	//Write to the LCD segments, use OR to not erase the wrong section
	//Bit mask the bits we don't want to change.
	*(p_LCDDR0+0 + pos) = (*(p_LCDDR0+0 + pos) & 0x9f9>>shift)		|	 DIGIT_BIT_PATTERNS[(uint8_t)ch].first_pattern << shift;
	*(p_LCDDR0+5 + pos) = (*(p_LCDDR0+5 + pos) & 0xf0>>shift)		|	 DIGIT_BIT_PATTERNS[(uint8_t)ch].second_pattern << shift;
	*(p_LCDDR0+10 + pos) = (*(p_LCDDR0+10 + pos) & 0xf0>>shift)		|	 DIGIT_BIT_PATTERNS[(uint8_t)ch].third_pattern << shift;
	*(p_LCDDR0+15 + pos) = (*(p_LCDDR0+15 + pos) & 0xf0>>shift)		|	 DIGIT_BIT_PATTERNS[(uint8_t)ch].fourth_pattern << shift;
}


//To use the different patterns just use DIGIT_ARR[x] (where x is the number 0-9)
void char_init(char_pattern_t DIGIT_ARR[])
{
	//DIGIT 0
	DIGIT_ARR[0] = (char_pattern_t){0x1, 0x5, 0x5, 0x1};
	//DIGIT 1
	DIGIT_ARR[1] = (char_pattern_t){0x0, 0x1, 0x1, 0x0};
	//DIGIT 2
	DIGIT_ARR[2] = (char_pattern_t){0x1, 0x1, 0xE, 0x1};
	//DIGIT 3
	DIGIT_ARR[3] = (char_pattern_t){0x1, 0x1, 0xB, 0x1};
	//DIGIT 4
	DIGIT_ARR[4] = (char_pattern_t){0x0, 0x5, 0xB, 0x0};
	//DIGIT 5
	DIGIT_ARR[5] = (char_pattern_t){0x1, 0x4, 0xB, 0x1};
	//DIGIT 6
	DIGIT_ARR[6] = (char_pattern_t){0x1, 0x4, 0xF, 0x1};
	//DIGIT 7
	DIGIT_ARR[7] = (char_pattern_t){0x1, 0x1, 0x1, 0x0};
	//DIGIT 8
	DIGIT_ARR[8] = (char_pattern_t){0x1, 0x5, 0xF, 0x1};
	//DIGIT 9
	DIGIT_ARR[9] = (char_pattern_t){0x1, 0x5, 0xB, 0x0};
}

	int pp;

void printAt(long num, int pos) {
	lock(&initMutex);
	pp = pos;
    writeChar( (num % 100) / 10 + '0', pp);
    pp++;
    writeChar( num % 10 + '0', pp);
	unlock(&initMutex);
}

uint8_t is_prime(long i)
{
	long index = 2;
	while(index < i)
	{
		if(i % index == 0)
		{
			return (uint8_t)0x00;
		}
		index++;
	}
	return (uint8_t)0x01;
}


void computePrimes(int pos) {
    long n;
    for(n = 1; ; n++) {
        if (is_prime(n)) {
            printAt(n, pos);
			//yield();
        }
    }
}

	uint8_t state = 0;
	
	ISR(PCINT0_vect) {
 			yield();

// 		if (((PINB & 0x80) == 0x0) && (state == 0x0))
// 		{
// 			yield();
// 			state = 0x1;
// 			
// 		}
// 		else if ((PINB & 0x80) && state == 0x1)
// 		{
// 			state = 0x0;
// 		}
	}

	ISR(TIMER1_COMPA_vect){
		//yield();
	}


int main() {
	LCD_Init();
	char_init(DIGIT_BIT_PATTERNS);
		
    spawn(computePrimes, 0);
    computePrimes(3);
}
