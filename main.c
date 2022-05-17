// Simple datalogger example, continuously ADC sending RS232 data (9600baud) at 100 samples/sec
// Written by Tim Gilmour, based on Texas Instruments demo code

#include "msp430g2553.h"

#define REDLED BIT0
#define GREENLED BIT6

volatile unsigned char digits[5];
volatile unsigned char digitIndex = 0;

void main(void)
{
  volatile unsigned char newADCReading = 0;

  WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT
  digits[3] = 0x0A;  // LF
  digits[4] = 0x0D;  // CR
  
  // set up UART
  DCOCTL = 0;                               // Select lowest DCOx and MODx settings
  BCSCTL1 = CALBC1_1MHZ;                    // Set DCO calibration
  DCOCTL = CALDCO_1MHZ;
  P1SEL = BIT2;                     	    // P1.2=TXD
  P1SEL2 = BIT2;                    	    // P1.2=TXD
  UCA0CTL1 |= UCSSEL_2;                     // SMCLK
  UCA0BR0 = 104;                            // 1MHz 9600
  UCA0BR1 = 0;                              // 1MHz 9600
  UCA0MCTL = UCBRS0;                        // Modulation UCBRSx = 1
  UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
  
  // set up ADC
  ADC10CTL0 = ADC10SHT_2 + ADC10ON + ADC10IE; // ADC10ON, interrupt enabled
  ADC10CTL1 = INCH_1;                       // input A1
  ADC10AE0 |= 0x02;                         // PA.1 ADC option select

  // set up GPIO's for LED
  P1OUT = 0;                                // initialize P1 to zeros
  P1DIR |= GREENLED | REDLED;               // Set P1.0 and P1.6 to output direction

  // set up timer
  TACCR0 = 0x0200;                          // used for red led-flash delay
  TACCTL0 |= CCIE;                          // Compare-mode interrupt.
  __enable_interrupt();

  while(1)  // main loop
  {
	  // get ADC sample  (assumes that the event loop below is MUCH slower than this ADC conversion)
	  ADC10CTL0 |= ENC + ADC10SC;                // Sampling and conversion start
	  LPM0;           			     // begin LPM0 here (ADC10_ISR will exit LPM0 and resume below after conversion has finished)
	  newADCReading = (ADC10MEM >> 2);           // currently only using 8 bits?

	  //  convert newADCReading into the ASCII decimal number, then TX to computer via RS232
	  digits[0] = ((newADCReading/100) % 10) + '0';
	  digits[1] = ((newADCReading/10) % 10) + '0';
	  digits[2] = (newADCReading % 10) + '0';
	  
	  UCA0TXBUF = digits[digitIndex++];         // begin sending the first digit, and then increment the digitIndex
	  IE2 |= UCA0TXIE;                          // Enable USCI_A0 RX interrupt  (will be disabled in ISR after finishing)

	  __delay_cycles(9000);
  }
}

#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
{
  UCA0TXBUF = digits[digitIndex++];         // begin sending the next digit, and then increment the index
  if (digitIndex >= 5)
  {
	  IE2 &= ~UCA0TXIE;                 // turn off TX interrupts
	  digitIndex = 0;                   // reset to beginning
  }
}

#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
	LPM0_EXIT;
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void ta0_isr(void)
{
  TACTL = 0;				   // stop timer
  LPM0_EXIT;
}
