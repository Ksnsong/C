#include "NU32.h"          // constants, funcs for startup and UART
#include <stdio.h>
#include <stdint.h>

#define VOLTS_PER_COUNT (3.3/1024)
#define CORE_TICK_TIME 25    // nanoseconds between core ticks
#define SAMPLE_TIME 10       // 10 core timer ticks = 250 ns
#define DELAY_TICKS 20000000 // delay 1/2 sec, 20 M core ticks, between messages

#define PERIOD 1024        // this is PR2 + 1
#define MAXVOLTAGE 3.3     // corresponds to max high voltage output of PIC32

#define DELAYTIME 5000000 // 5 million
void delay(void);
void greenLight(void);
void redLight(void);
unsigned int adc_sample_convert(int pin) { // sample & convert the value on the given 
                                           // adc pin the pin should be configured as an 
                                           // analog input in AD1PCFG
    unsigned int elapsed = 0, finish_time = 0;
    AD1CHSbits.CH0SA = pin;                // connect chosen pin to MUXA for sampling
    AD1CON1bits.SAMP = 1;                  // start sampling
    elapsed = _CP0_GET_COUNT();
    finish_time = elapsed + SAMPLE_TIME;
    while (_CP0_GET_COUNT() < finish_time) { 
      ;                                   // sample for more than 250 ns
    }
    AD1CON1bits.SAMP = 0;                 // stop sampling and start converting
    while (!AD1CON1bits.DONE) {
      ;                                   // wait for the conversion process to finish
    }
    return ADC1BUF0;                      // read the buffer with the result
}

volatile unsigned int oldB = 0, oldF = 0, newB = 0, newF = 0; // save port values

void outPort(unsigned char data){ 
	unsigned char bits; 
	for (bits=0x80; bits!=0; bits >>= 1) { 
		if ((bits & data) == bits){           // send Data 
			PORTBbits.RB3 = 1; 
			} 
		else 
			PORTBbits.RB3 = 0; 
	PORTBbits.RB0 = 1;                              // Clock the data
	PORTBbits.RB0 = 0; 
   } 
   PORTBbits.RB1 = 1;    // Latch the data 
   PORTBbits.RB1 = 0; 
   
} 

void delay(void) {
  int i;
  for (i = 0; i < DELAYTIME; i++) {
      ; //do nothing
  }
}

void __ISR(_CHANGE_NOTICE_VECTOR, IPL3SOFT) CNISR(void) { // INT step 1
  newB = PORTB;           // since pins on port B and F are being monitored 
  oldB = newB;            // save the current values for future use
  NU32_LED1 = !NU32_LED1; // toggle LED1
  IFS1bits.CNIF = 0;      // clear the interrupt flag
}
   
int main(void) {
	unsigned char a;
	PORTBbits.RB1=0;
	PORTBbits.RB0=0;
	PORTBbits.RB3=0;
	int gate = 1;
	NU32_Startup(); // cache on, min flash wait, interrupts on, LED/button init, UART init
	AD1PCFG = 0x00FF;       // set B8-B15 as analog in, 0-7 as digital pins
	TRISB = 0xFF00;         // set 0-3as digital outputs,  B4-B7 as digital inputs
	TRISBbits.TRISB0 = 0;
	TRISBbits.TRISB1 = 0;
	TRISBbits.TRISB3 = 0;
	
	unsigned int sample14 = 0;
	AD1PCFGbits.PCFG14 = 0;                 // AN14 is an adc pin
	AD1CON3bits.ADCS = 2;                   // ADC clock period is Tad = 2*(ADCS+1)*Tpb =
                                          //                           2*3*12.5ns = 75ns
	AD1CON1bits.ADON = 1;                   // turn on A/D converter

	while(1) {                      // carriage return and newline
	sample14 = adc_sample_convert(8);
	if (sample14>10 && gate ==1){
		greenLight();
		gate = 2;
	}
	else if (sample14>10 && gate ==2){
		redLight();
		gate = 1;
	}		
		
	}
	return 0;
}

void greenLight(void){
	outPort(8);
	delay();
	outPort(4);
	delay();
	outPort(2);
	delay();
	outPort(1);
	delay();
	outPort(0);
}

void redLight(void){
	outPort(16);
	delay();
	outPort(32);
	delay();
	outPort(64);
	delay();
	outPort(128);
	delay();
	outPort(0);
}