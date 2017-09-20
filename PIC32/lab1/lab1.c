#include "NU32.h"          // constants, funcs for startup and UART

#define VOLTS_PER_COUNT (3.3/1024)
#define CORE_TICK_TIME 25    // nanoseconds between core ticks
#define SAMPLE_TIME 10       // 10 core timer ticks = 250 ns
#define DELAY_TICKS 20000000 // delay 1/2 sec, 20 M core ticks, between messages

#define PERIOD 1024        // this is PR2 + 1
#define MAXVOLTAGE 3.3     // corresponds to max high voltage output of PIC32

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

void __ISR(_CHANGE_NOTICE_VECTOR, IPL3SOFT) CNISR(void) { // INT step 1
  newB = PORTB;           // since pins on port B and F are being monitored 
  newF = PORTF;           // by CN, must read both to allow continued functioning
                          // ... do something here with oldB, oldF, newB, newF ...
  oldB = newB;            // save the current values for future use
  oldF = newF;
  LATBINV = 0xF0;         // toggle buffered RB4, RB5 and open-drain RB6, RB7
  NU32_LED1 = !NU32_LED1; // toggle LED1
  IFS1bits.CNIF = 0;      // clear the interrupt flag
}

int main(void) {
  unsigned int sample_D1 = 0, sample_D2 = 0, sample_D3 = 0, elapsed = 0;
  char msg[100] = {};
	
  NU32_Startup(); // cache on, min flash wait, interrupts on, LED/button init, UART init
  AD1PCFG = 0x00FF;       // set B8-B15 as analog in, 0-7 as digital pins
  TRISB = 0xFF0F;         // set B4-B7 as digital outputs, 0-3 as digital inputs
  ODCBSET = 0x00C0;       // set ODCB bits 6 and 7, so RB6, RB7 are open drain outputs
  CNPUEbits.CNPUE2 = 1;   // CN2/RB0 input has internal pull-up 
  CNPUEbits.CNPUE3 = 1;   // CN3/RB1 input has internal pull-up
  CNPUEbits.CNPUE17 = 1;  // CN17/RF4 input has internal pull-up
                          // due to errata internal pull-ups may not result in a logic 1
                          
  oldB = PORTB;           // bits 0-3 are relevant input
  oldF = PORTF;           // pins of port F are inputs, by default
  LATB = 0x0050;          // RB4 is buffered high, RB5 is buffered low,
                          // RB6 is floating open drain (could be pulled to 3.3 V by
                          // external pull-up resistor), RB7 is low

  __builtin_disable_interrupts(); // step 1: disable interrupts
  CNCONbits.ON = 1;               // step 2: configure peripheral: turn on CN
  CNENbits.CNEN2 = 1;             // Use CN2/RB0 as a change notification
  CNENbits.CNEN17 = 1;            // Use CN17/RF4 as a change notification
  CNENbits.CNEN18 = 1;            // Use CN18/RF5 as a change notification
  
  IPC6bits.CNIP = 3;              // step 3: set interrupt priority
  IPC6bits.CNIS = 2;              // step 4: set interrupt subpriority
  IFS1bits.CNIF = 0;              // step 5: clear the interrupt flag
  IEC1bits.CNIE = 1;              // step 6: enable the CN interrupt
  __builtin_enable_interrupts();  // step 7: CPU enabled for mvec interrupts


//ADC
  AD1PCFGbits.PCFG8 = 0;                 // AN14 is an adc pin
  AD1PCFGbits.PCFG9 = 0;                 // AN15 is an adc pin
  AD1PCFGbits.PCFG10 = 0;
  AD1CON3bits.ADCS = 2;                   // ADC clock period is Tad = 2*(ADCS+1)*Tpb =
                                          //                           2*3*12.5ns = 75ns
  AD1CON1bits.ADON = 1;                   // turn on A/D converter

//PWM  
  PR2 = PERIOD - 1;       // Timer2 is OC3's base, PR2 defines PWM frequency, 78.125 kHz
  TMR2 = 0;               // initialize value of Timer2
  T2CON = 1<<15; // turn Timer2 on, all defaults fine
  
  //D1 pin
  OC2CONbits.OCTSEL = 0;  // use Timer2 for OC2
  OC2CONbits.OCM = 0b110; // PWM mode with fault pin disabled
  OC2CONbits.ON = 1;      // Turn OC2 on
  //D2 pin
  OC3CONbits.OCTSEL = 0;  // use Timer2 for OC3
  OC3CONbits.OCM = 0b110; // PWM mode with fault pin disabled
  OC3CONbits.ON = 1;      // Turn OC3 on
  //D3 pin
  OC4CONbits.OCTSEL = 0;  // use Timer2 for OC4
  OC4CONbits.OCM = 0b110; // PWM mode with fault pin disabled
  OC4CONbits.ON = 1;      // Turn OC4 on
  
  while(1) {                      // carriage return and newline
      sample_D2 = adc_sample_convert(8);
	  sample_D1 = adc_sample_convert(9);
	  sample_D3 = adc_sample_convert(10);
      OC3RS = sample_D2; //Red
	  OC2RS = sample_D1; //Blue
	  OC4RS = sample_D3; //Green

	  if (PORTBbits.RB1 == 0){
		OC3RS = 0; //Red
		OC2RS = 1023; //Blue
		OC4RS = 0;                           // infinite loop
	  }
  }
  return 0;
}
