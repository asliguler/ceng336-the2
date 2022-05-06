#include <xc.h>
#include <stdint.h>

uint8_t game_started = 0, game_level = 1;

void init_ports() {
    TRISA = 0x00;
    TRISB = 0x00;
    TRISC = 0x01;
    TRISD = 0x00;
    TRISE = 0x00;
    TRISF = 0x00;
    TRISG = 0x1f;
    TRISH = 0x00;
    TRISJ = 0x00;
}

void init_irq() {
    INTCONbits.TMR0IE = 1;
    INTCONbits.GIE = 1;
}

void config_tmr0() {
    T0CONbits.T08BIT = 1; // 8-bit mode
    T0CONbits.T0PS0 = 1; // 1:256 pre-scaler
    T0CONbits.T0PS1 = 1; // 1:256 pre-scaler
    T0CONbits.T0PS2 = 1; // 1:256 pre-scaler 
}

void init_tmr1() {
    T1CON = 0x00;
    T1CONbits.T1CKPS0 = 1; // 1:8 pre-scaler
    T1CONbits.T1CKPS1 = 1; // 1:8 pre-scaler
    T1CONbits.TMR1ON = 1; // turn on timer
    // read value of TIMER1 from TMR1H:TMR1L registers
}

uint8_t generate_random() {
    uint8_t random_note = (TMR1L & 0x07);
    random_note %= 5;
    switch(game_level) {
        case 1:
            random_note = ((TMR1L >> 1) & 0x07);
        case 2:
            random_note = ((TMR1L >> 3) & 0x07);
        case 3:
            random_note = ((TMR1L >> 5) & 0x07);
    }
    random_note %= 5;
    return random_note;
}

typedef enum {TMR_IDLE, TMR_RUN, TMR_DONE} tmr_state_t;
tmr_state_t tmr0_state = TMR_IDLE, tmr1_state = TMR_IDLE;
uint8_t tmr0_startreq = 0, tmr1_startreq = 0;
uint8_t tmr0_ticks_left, tmr1_ticks_left;   

void initial_7segment_display() {
    // !!TODO: THIS SHOULD BE INSIDE A LOOP TO SWITCH AND FORWARD
    //         BETWEEN LEFTMOST AND RIGHTMOST DISPLAYS
    
    // set health to 9
    PORTH = 0x01; // to set leftmost display
    PORTJ = 0x6f; // to set 9
    // TODO: wait a while
    // set level to 1
    PORTH = 0x08; // to set rightmost display
    PORTJ = 0x06; // to set 1
}


void input_task() {
    if (PORTCbits.RC0) {
        game_started = 1;
        TRISC = 0x00;
        game_level = 1;
    }
}

void game_over() {
    //TODO: appropriate texts displayed on 7-segment display
    TRISC = 0x01;
    game_started = 0;
}
