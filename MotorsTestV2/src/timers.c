/*
 * timers.c
 *
 *  Created on: Oct 16, 2021
 *      Author: austi
 */
#include "msp.h"
#include <timers.h>
#include <motors.h>
#include <gpio.h>

//=================
// Globals
//=================
    extern volatile char Motors_Ready;
    extern volatile uint16_t TA2_CCR1_Count = 0;
    extern volatile uint16_t TA2_CCR2_Count = 0;



//==========================
// Timer Initialization
//==========================

void Init_Timers(void){
    Init_TimerA2(TIMERA2_HALF);
    Init_TimerA0(MOTOR_PWM_PERIOD, LEFT_OFF, RIGHT_OFF);  // Set the initial period and duty cycle of the motors
}


// PWM for Motors

void Init_TimerA0(uint16_t period, uint16_t duty_left, uint16_t duty_right)
{
    TIMER_A0->CCTL[0] = 0x0080; // CCI0 toggle
    TIMER_A0->CCR[0] = period; // Period is (8 / SMCLK)
    TIMER_A0->EX0 = 0x0000; // divide by 1
    TIMER_A0->CCTL[3] = 0x0040; // CCR3 toggle/reset
    TIMER_A0->CCR[3] = duty_right; // CCR3 duty cycle is duty_right/period
    TIMER_A0->CCTL[4] = 0x0040; // CCR4 toggle/reset
    TIMER_A0->CCR[4] = duty_left; // CCR4 duty cycle is duty_left/period
    TIMER_A0->CTL = 0x02F0; // SMCLK=12MHz, divide by 8, up-down mode
}


// LED Blink , Timed Motor Movement, Switch Debounce

void Init_TimerA2(uint16_t period){
    TIMER_A2->CTL = 0x0100;      // Set clock source to ACLK (32.768 KHz), input clock divider / 1

    TIMER_A2->CCR[0] = period;   // Set capture compare register value
    TIMER_A2->CCTL[0] |= CCIE;   // Capture mode, compare mode, enable CC interrupt
    TIMER_A2->CCTL[0] &= ~CCIFG; // Clear interrupt flag

    //TIMER_A2->CCTL[1] |= CCIE;   // Capture mode, compare mode, enable CC interrupt
    TIMER_A2->CCTL[1] &= ~CCIFG; // Clear interrupt flag

    //TIMER_A2->CCTL[2] |= CCIE;   // Capture mode, compare mode, enable CC interrupt
    TIMER_A2->CCTL[2] &= ~CCIFG; // Clear interrupt flag


    TIMER_A2->EX0 = 0x0000;      // Input clock divider, divide by 1
   //Configure Interrupts
    NVIC_SetPriority(TA2_0_IRQn, 2); //sets interrupt priority for CCTL[0]
       //enable interrupt
    NVIC_EnableIRQ(TA2_0_IRQn);

    NVIC_SetPriority(TA2_N_IRQn, 2); //sets interrupt priority for CCTL[1-6]
       //enable interrupt
    NVIC_EnableIRQ(TA2_N_IRQn);

    TIMER_A2->CTL |= 0x0014;     // reset and start Timer A in up mode

}




// Timer A2 Interrupt

void TA2_0_IRQHandler(void) {
    TIMER_A2->CCTL[0] &= ~CCIFG; // Clear flag
    P2OUT ^= (RGB_RED | RGB_BLUE | RGB_GREEN);

}

void TA2_N_IRQHandler(void){
    switch(TIMER_A2->IV){

    // Capture Compare 1 For Timed Motor Movement
        case 0x02:
            TIMER_A2->CCTL[2] &= ~CCIFG;
            Motors_Ready = 1;
            MotorsSimple(LEFT_OFF, FORWARD, RIGHT_OFF, FORWARD);
            TIMER_A2->CCTL[1] &= ~CCIE;   // Disable Interrupt when counter expires
        break;

    // Capture Compare 2 for switch debounce
        case 0x04:
            TIMER_A2->CCTL[2] &= ~CCIFG;
            P1IFG &= ~SW1;
            P4IE |= SW1;
            P1OUT &= ~RED_LED;
            TIMER_A2->CCTL[2] &= ~CCIE;   // Disable Interrupt when counter expires
        break;

        default: break;
    }

}

