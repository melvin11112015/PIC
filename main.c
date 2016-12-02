/******************************************************************************/
/* Files to Include                                                           */
/******************************************************************************/

#if defined(__XC)
#include <xc.h>         /* XC8 General Include File */
#elif defined(HI_TECH_C)
#include <htc.h>        /* HiTech General Include File */
#endif

#include <stdint.h>        /* For uint8_t definition */
#include <stdbool.h>

#define _XTAL_FREQ 32000000 //32Mhz clock source
#define R_PIN LATAbits.LATA5
#define G_PIN LATAbits.LATA0
#define B_PIN LATAbits.LATA1
#define SIGNAL_OUTPUT LATAbits.LATA0
#define COMPENSATION 5

/******************************************************************************/
/* User Global Variable Declaration                                           */
/******************************************************************************/

volatile unsigned int t1ov_cnt, temp;
unsigned int test, period_x, measuredPulse, flag, dutyx10, duty;

/******************************************************************************/
/* Main Program                                                               */

/******************************************************************************/

//***debug code
//debug function

/*
void Data2PWM(unsigned int data) {
    // Generate PWM signals to show data
    //duty+ : 700us --> Digit 1
    //duty+ : 200us --> Digit 0
    __delay_ms(5);
    for (unsigned int i = 0; i < 16; i++) {
        if (((data >> i) & 1) == 0) {
            SIGNAL_OUTPUT = 1;
            __delay_us(200);
            SIGNAL_OUTPUT = 0;
            __delay_us(200);
        } else {
            SIGNAL_OUTPUT = 1;
            __delay_us(700);
            SIGNAL_OUTPUT = 0;
            __delay_us(200);
        }
    }
    __delay_ms(5);

}*/
//***debug code

void InitApp(void) {
    /* Configure the oscillator for the device */
    OSCCONbits.IRCF = 0b1110; //32MHZ
    OSCCONbits.SCS = 0b00;

    /* Initialize I/O and Peripherals for application */
    PORTA = 0;
    TRISA = 0B010000;
    APFCONbits.T1GSEL = 0; //T1G On RA4
    ANSELA = 0; //Important!Enable digtal IO special function!Default pins read as 1

    //color white
    R_PIN = 1;
    G_PIN = 1;
    B_PIN = 1;

    /* Initialize global variable */
    TMR1 = 0;

    t1ov_cnt = 0;
    flag = 0;
    temp = 0;
    dutyx10 = 0;
    test = 0;
    measuredPulse = 100;

    /* Enable interrupts */
    PIR1bits.TMR1IF = 0; //clear flag
    PIR1bits.TMR1GIF = 0;

    T1CONbits.TMR1CS = 0b01; //Fosc as  TMR1 CLK

    T1GCONbits.TMR1GE = 1;
    T1GCONbits.T1GSS = 0; // source - PIN T1G
    T1GCONbits.T1GTM = 1; //???
    T1GCONbits.T1GSPM = 1; //??
    T1GCONbits.T1GPOL = 1;

    INTCONbits.GIE = 1; //Enable interrupt
    INTCONbits.PEIE = 1;
    PIE1bits.TMR1GIE = 1;
    PIE1bits.TMR1IE = 1;
}

void main(void) {

    InitApp();

    T1CONbits.TMR1ON = 1; //enable TMR1
    T1GCONbits.T1GGO = 0; //

    SIGNAL_OUTPUT = 0;

    while (1) {

        //debug-----
        // Data2PWM(test|T1GCONbits.T1GGO_nDONE);
        //Data2PWM(TMR1);
        //debug-----

        if (T1GCONbits.T1GVAL == 0) {
            if (flag == 1) {
                period_x = temp;
                T1GCONbits.T1GTM = 0; //???
                T1GCONbits.T1GSPM = 1; //??
                PIR1bits.TMR1GIF = 0;
                TMR1 = 0;
            } else if (flag >= 2) {
                dutyx10 = 0;
                measuredPulse = temp;
                T1GCONbits.T1GTM = 1; //???
                T1GCONbits.T1GSPM = 1; //??
                PIR1bits.TMR1GIF = 0;
                TMR1 = 0;
                flag = 0;
            }
            T1GCONbits.T1GGO = 1;
            flag++;
        }

        if (measuredPulse < period_x) {
            dutyx10 = ((float) measuredPulse / (float) period_x)*1000;
        } else if (PORTAbits.RA4 == 1) {
            dutyx10 = 1000;
        } else dutyx10 = 0;
        duty = (dutyx10 + COMPENSATION) / 10;

        //***debug code
        //if (duty == 100)R_PIN = 1;else R_PIN = 0; //debug
        //Data2PWM(duty);//debug
        //***debug code

        if (duty <= 11) {
            //light off
            R_PIN = 0;
            G_PIN = 0;
            B_PIN = 0;
        } else if ((duty >= 12) && (duty <= 24)) {
            //color cyan
            R_PIN = 0;
            G_PIN = 1;
            B_PIN = 1;
        } else if ((duty >= 25) && (duty <= 37)) {
            //color magenta
            R_PIN = 1;
            G_PIN = 0;
            B_PIN = 1;
        } else if ((duty >= 38) && (duty <= 100)) {
            //color red
            R_PIN = 1;
            G_PIN = 0;
            B_PIN = 0;
        } else {
            //error condition
            //color white
            R_PIN = 1;
            G_PIN = 1;
            B_PIN = 1;
        }

    }
}

/******************************************************************************/
/* Interrupt Routines                                                         */

/******************************************************************************/

void interrupt isr(void) {
    if (PIR1bits.TMR1IF == 1) {

        if (T1GCONbits.T1GGO_nDONE == 1)
            t1ov_cnt++;
        else
            t1ov_cnt = 0;
        PIR1bits.TMR1IF = 0; //clear flag
    } else if (PIR1bits.TMR1GIF == 1) {

        temp = t1ov_cnt;
        temp <<= 16;
        temp += TMR1;
        t1ov_cnt = 0;
        PIR1bits.TMR1GIF = 0; //clear flag
    }
}