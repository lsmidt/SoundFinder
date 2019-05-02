#include <msp430.h> 

/**
 * © Louis Smidt 2019
 */

#define ADC_INPUTS 4

unsigned int samples[ADC_INPUTS];

unsigned int length = 56;       // store length of
unsigned char *frameptr;
unsigned char frames[] = {0x00, 0x00, 0x00, 0x00, //32 bits of 0's for start frames

            0xF1, 0x10, 0x00, 0x00,  // LED1
            0xF1, 0x10, 0x00, 0x00,
            0xF1, 0x10, 0x00, 0x00,
            0xF1, 0x10, 0x00, 0x00,
            0xF1, 0x10, 0x00, 0x00,  // LED5
            0xF1, 0x10, 0x00, 0x00,
            0xF1, 0x10, 0x00, 0x00,
            0xF1, 0x10, 0x00, 0x00,
            0xF1, 0x10, 0x00, 0x00,
            0xF1, 0x10, 0x00, 0x00,  // LED 10
            0xF1, 0x10, 0x00, 0x00,
            0xF1, 0x10, 0x00, 0x00,

                0xFF, 0xFF, 0xFF, 0xFF}; // End frame, 32 bits of 1's


void setup(void) {
        // WDT + Clock Setup
        WDTCTL = WDT_ADLY_250;                  // Watchdog timer 1s period
        BCSCTL3 |= LFXT1S_2;                    // ACLK = VLO (init ACLK to use VLO)

        BCSCTL1 = CALBC1_8MHZ;                  // 8 MHz DCO
        DCOCTL = CALDCO_8MHZ;

        IE1 |= WDTIE;

        BCSCTL2 &= ~(SELS);     // clear SELS bit to set SMCLK to DCO
        BCSCTL2 |= DIVS_3 + DIVM_0 + SELM_0;
            // SELS -> source SMCLK and MCLK from DCO ; Divide SMCLK by 8, MCLK by 1

        // Pins
        P1DIR |= BIT5 + BIT7;
        P1SEL |= BIT5 + BIT7;
        P1SEL2 |= BIT5 + BIT7;
            //P1.5, 7 to Output -> DATA & CLK for SPI


        // USCI Setup
        UCB0CTL1 |= UCSWRST;
        UCB0CTL0 |= UCCKPH + UCMSB + UCMST + UCSYNC;
        UCB0CTL1 |= UCSSEL_2;           // use SMCLK for SPI
        UCB0CTL1 &= ~UCSWRST;          // clear sw reset, enables the USCI for operation

        IE2 |= UCB0TXIE;               // SPI TX Interrupt


        // ADC10 Setup
        ADC10AE0 |= BIT3 + BIT2 + BIT1 + BIT0;
            //Enable A3 - A0 as analog inputs

        // ADC10DTC0 sets One Block Mode by default, nothing written to it
        ADC10DTC1 = ADC_INPUTS;
            //set number of blocks for DTC to transfer
        ADC10SA = (unsigned int) samples;

        ADC10CTL1 = INCH_3 + ADC10DIV_1 + CONSEQ_1 + SHS_0+ ADC10SSEL_0;
            //CONSEQ_1 -> Sequence of Channels ; INCH_3 -> Beginning at A3 - A0; SHS_1 -> Start conversion when TA0 outputs high
        ADC10CTL0 = SREF_1 + ADC10SHT_2 + ADC10ON + REFON + ADC10IE + MSC + ENC + ADC10SC;
            // SREF_0 -> Ref'd to VDD - GND ; SHT_3 -> ; MSC -> Auto Repeat Channel Sequence; ADC10SHT_1 -> Sample settling time of 64 ADC10CLKs

}

int diff2(int a, int b){
    /*
     * Return the absolute value of the difference between two integers
     */
    int c = a - b;
    if (c < 0){
        return -1 * c;
    }
    return c;
}

void allOff(){
    /*
     * turn all the LEDs off
     */
    unsigned int count;
    for (count = 4; count <= 48; count+=4){
        frames[count] = 0xE0;           //write brightness to 0
    }
}


void onLED(unsigned int a, unsigned int b){
    /*
     * Specify range a - b of which LEDs are ON. Others are OFF.
     * Range of a is 4-44 b is 8 - 48
     */

    allOff();

    unsigned int count;
    for (count = 4; count <= 48; count+=4){
        if (count >= a && count <= b){
            frames[count] = 0xF1;       //write brightness value to 0xF1.
        }
    }

    UCB0TXBUF = *frameptr;       //write out updated
}


void readMics(void){
    /*
     *  samples[0] <- A3, TOP-LEFT
     *  samples[1] <- A2, TOP OR TOP-RIGHT
     *  samples[2] <- A1, BOTTOM-RIGHT
     *  samples[3] <- A0, BOTTOM-LEFT
     *
     *  Find loudest sound, find difference between loudest sound and
     *
     */

//    __bic_SR_register (GIE);

    unsigned int TL, TR, BL, BR;
    unsigned int thresh;

    TL = samples[0];
    TR = samples[1];
    BR = samples[2];
    BL = samples[3];

    thresh = 15;    //threshold to activate lights

    unsigned int axis1, axis2, st = 4, nd = 4;
    char updated = 0;

    // diagonal or anti-diagonal amplitude difference is checked first
    axis1 = (unsigned int) diff2(TL, BR);
    axis2 = (unsigned int) diff2(BL, TR);

    //main logic

    if (axis1 > thresh && axis1 > axis2){ //the two big cases
        updated = 1;

        if (TL > BR) {
            st = 16;
            nd = 24;

        }else if (BR > TL){
            st = 40;
            nd = 48;
        }

    }

    if (axis2 > thresh && axis2 > axis1) {
        updated = 1;

        if (BL > TR){
            st = 28;
            nd = 36;

        }else if (TR > BL){
            st = 4;
            nd = 12;
        }

    }

    if (updated){
        onLED(st, nd);
    }else{
        allOff();
    }

    __delay_cycles(2000);       //helps slow the response time for the LEDs


}


void main(void)
{

    setup();
    __bis_SR_register(GIE);

   while(1){
       allOff();

       ADC10CTL0 &= ~ENC;       //reset ENC to restart conversion

       ADC10DTC1 = 4;
       ADC10SA = (unsigned int) samples;        //trigger DTC

       ADC10CTL0 |= ENC;              // activate ADC sequence
       ADC10CTL0 |= ADC10SC;

       __bis_SR_register(LPM0_bits + GIE);  //turn off CPU until ADC conversion is completed, avoids race condition between CPU and DTC

       readMics();
   }

}


// USCI TX ISR
#pragma vector=USCIAB0TX_VECTOR
__interrupt void USCIB0TX_ISR(void){

    if (length > 0){     //still sending array / increment and repeat transmit
        length--;
        frameptr++;
        UCB0TXBUF = *frameptr;
    }
    else{               // finished sending / clear ISR / reset length and data pointer
    IFG2 &= ~UCB0TXIFG;
    length = 56;
    frameptr = frames;
    }
}



// ADC10 interrupt service routine
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR (void)
{
    __bic_SR_register_on_exit(LPM0_bits); // exit LMP0
}

// Watchdog Timer interrupt service routine
 #if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
 #pragma vector=WDT_VECTOR
 __interrupt void watchdog_timer(void)
 #elif defined(__GNUC__)
 void __attribute__ ((interrupt(WDT_VECTOR))) watchdog_timer (void)
 #else
 #error Compiler not supported!
 #endif
 {

     __bic_SR_register_on_exit(LPM0_bits);  //exit LPM every 250ms
 }











