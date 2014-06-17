/****************************************************************************
 *
 * ir_sender.ino // Jacques Supcik // 17-June-2014
 *
 ****************************************************************************
 *
 * Interrupt driven program to control Infra-red LED for light barrier
 *
 ****************************************************************************
 *
 * Copyright 2014 Jacques Supcik
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ****************************************************************************/

// N.B. The DigialWrite() function of the Arduino is too slow for our
// purpose. If we want to ascillate at 38KHz using an interrupt function,
// we have to address the PORTs directly. THIS WILL MAKE THE FOLLOWING CODE
// "NOT PORTABLE".

// The IR LEDs the output D2 to D7 (which correspond to the PINs PD2 to PD7)
// The pins D8 to D13 (which correspond to the PINs PB0 to PB5) are for
// feedback and debugging.

#include <avr/sleep.h>

// The IR_MODULATION is given in KhZ
#define IR_MODULATION_KHZ 38

#define BURST_LEN 20
#define SPACE_LEN 20

ISR (TIMER1_COMPA_vect) {
    // count counts the number of interrupts and generate the modulation
    static uint8_t count = 0;

    // For the feedback, we want a blinking LED with a short duty cycle. It
    // should look like a heartbeat. So we aim at 60-80 bpm.
    // For 60 bpm, the period will need 2 * 38000 ticks. This is a large
    // number and would require a UINT32. But Operations on 32 bits are
    // expensive on the Arduino, so we decided to use 3 UINT8 variables:
    static uint8_t heart_beat0 = 0;
    static uint8_t heart_beat1 = 0;
    static uint8_t heart_beat2 = 0;
  
    count++;
    if (count < BURST_LEN * 2) {
        PORTD ^= (_BV(PORTD2) | _BV(PORTD3) | _BV(PORTD4) |
                  _BV(PORTD5) | _BV(PORTD6) | _BV(PORTD7));
    } else if (count < (BURST_LEN + SPACE_LEN) * 2) {
        PORTD &= ~(_BV(PORTD2) | _BV(PORTD3) | _BV(PORTD4) |
                   _BV(PORTD5) | _BV(PORTD6) | _BV(PORTD7));
    } else {
        count = 0;
    }
  
    // heart_beat0 counts from 0 to IR_MODULATION_KHZ at 2 * IR_MODULATION_KHZ
    // heart_beat1 counts from 0 to 20 at 2KHz
    // heart_beat2 counts from 0 to 100 at 100Hz (so in 1 second)
    if (heart_beat0 < (IR_MODULATION_KHZ)) {
        heart_beat0++;
    } else {
        heart_beat0 = 0;
        if (heart_beat1 < 20) {
            heart_beat1++;
        } else {
            heart_beat1 = 0;
            if (heart_beat2 < 100) {
                heart_beat2++;
            } else {
                heart_beat2 = 0;
            }
        }
    }

    if (heart_beat2 < 95) { // 5% duty cycle
        PORTB &= ~(_BV(PORTB0) | _BV(PORTB1) | _BV(PORTB2) |
                   _BV(PORTB3) | _BV(PORTB4) | _BV(PORTB5));
    } else {
        PORTB |= (_BV(PORTB0) | _BV(PORTB1) | _BV(PORTB2) |
                  _BV(PORTB3) | _BV(PORTB4) | _BV(PORTB5));
    }
}

void setup() {
    DDRD |= _BV(DDD2) | _BV(DDD3) | _BV(DDD4) | 
            _BV(DDD5) | _BV(DDD6) | _BV(DDD7);
    DDRB |= _BV(DDB0) | _BV(DDB1) | _BV(DDB2) | 
            _BV(DDB3) | _BV(DDB4) | _BV(DDB5);

    // Switch off LEDs
    PORTB &= ~(_BV(PORTD2) | _BV(PORTD3) | _BV(PORTD4) |
               _BV(PORTD5) | _BV(PORTD6) | _BV(PORTD7));
    
    TCCR1A  = 0;

    // set waveform mode 4 (CTC) + no prescaler
    TCCR1B  = _BV(WGM12) | _BV(CS10);  


    // According the the documentation, the frequency of the timer in CTC
    // mode is given by the following formula:
    // 
    //              f_clk_I/O
    //  OCR_nx = ---------------- - 1
    //            f_OCnx * 2 * N
    //
    // N is the pre-scaler, and with a 16bit timer and rather slow
    // CPU (<20MHz), we can use a pre-scaler of 1
    //
    // set OCR1A limit for IR_MODULATION_KHZ
    OCR1A   = ((F_CPU / (1L * IR_MODULATION_KHZ * 1000 * 2)) - 1);

    // enable interrupt on compare match A
    TIMSK1  = _BV(OCIE1A); 

    // DISABLE INTERRUPTS ON TIMER 0 (Improve precison)
    TIMSK0  = 0; 
    sei();

    // Set the SLEEP mode to IDLE (so the interrupts still works)
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();
}

void loop() {
    // Put the CPU in IDLE mode to save energy
    sleep_cpu();
}
