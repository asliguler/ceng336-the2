#include <xc.h>
#include <stdint.h>

void tmr0_isr();
void __interrupt(high_priority) highPriorityISR(void) {
    if (INTCONbits.TMR0IF) tmr0_isr();
    else if (PIR1bits.TMR1IF) tmr1_isr();
}
void __interrupt(low_priority) lowPriorityISR(void) {}

uint8_t game_started = 0, game_level = 1;

uint8_t health = 9;

uint16_t random_n_value;

typedef enum {LVL_BEG, LVL_NOT_BEG} level_state = LVL_BEG;


typedef enum {TMR_IDLE, TMR_RUN, TMR_DONE} tmr_state_t;
tmr_state_t tmr0_state = TMR_IDLE, tmr1_state = TMR_IDLE;
uint8_t tmr0_startreq = 0, tmr1_startreq = 0;
uint8_t tmr0_ticks_left, tmr1_ticks_left; 
uint8_t tmr0_count = 0;

#define TIMER0_PRELOAD_LEVEL1 74
#define TIMER0_PRELOAD_LEVEL2 4
#define TIMER0_PRELOAD_LEVEL3 190


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
    PIE1.TMR1IE = 1;
    INTCONbits.GIE = 1;
}

void config_tmr0() {
    T0CONbits.T08BIT = 1; // 8-bit mode
    T0CONbits.T0PS0 = 1; // 1:256 pre-scaler
    T0CONbits.T0PS1 = 1; // 1:256 pre-scaler
    T0CONbits.T0PS2 = 1; // 1:256 pre-scaler 
}

void tmr0_preload() {
    switch(game_level) {
        case 1:
            TMR0L = TIMER0_PRELOAD_LEVEL1 & 0xff;
        case 2:
            TMR0L = TIMER0_PRELOAD_LEVEL2 & 0xff;
        case 3:
            TMR0L = TIMER0_PRELOAD_LEVEL3 & 0xff;
    }
}

void tmr0_isr(){
    INTCONbits.TMR0IF = 0;
    switch (game_level) {
        case 1:
            if tmr0_count < 76 {

                tmr0_count++;
                TMR0L = 0x00;
            }
            else {
                tmr0_count = 0;
                tmr0_state = TMR_DONE;

                tmr0_preload();
            }
        case 2:
            if tmr0_count < 61 {
                tmr0_count++;
                TMR0L = 0x00;
            }
            else {
                tmr0_count = 0;
                tmr0_state = TMR_DONE;
                tmr0_preload();
            }
        case 3:
            if tmr0_count < 45 {
                tmr0_count++;
                TMR0L = 0x00;
            }
            else {
                tmr0_count = 0;
                tmr0_state = TMR_DONE;
                tmr0_preload();
            }
    }
}

void tmr1_isr() {
    PIR1bits.TMR1IF = 0;
    tmr1_state = TMR_DONE;
}

void init_tmr1() {
    if (game_started) {
        T1CON = 0x00;
    }
    // read value of TIMER1 from TMR1H:TMR1L registers
}

uint16_t generate_random() {
    uint16_t random_note ;
    switch(game_level) {
        case 1:
            random_n_value = (random_n_value >> 1) 
            random_note = (random_n_value & 0x0007);
        case 2:
            random_n_value = (random_n_value >> 3) 
            random_note = (random_n_value & 0x0007);
        case 3:
            random_n_value = (random_n_value >> 7) 
            random_note = (random_n_value & 0x0007);
    }
    random_note %= 5;
    return random_note;
}


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

void timer_task() {
    switch (tmr0_state) {
        case TMR_IDLE:
            if (tmr0_startreq) {
                // eger oyun icerisinden timer baslatma istegi
                // gelmisse timeri levela gore preload edip baslatalim
                tmr0_startreq = 0;
                tmr0_preload();
                T0CONbits.TMR0ON = 1;
                tmr0_state = TMR_RUN;
            }
            break;
        case TMR_RUN:
            break;
        case TMR_DONE:
            break;
    }
    switch (tmr1_state) {
        case TMR_IDLE:
            if (tmr1_startreq) {
                tmr1_startreq = 0;
                T1CONbits.TMR1ON = 1;
                tmr1_state TMR_RUN;
            }
        break;
        case 
    }
}

void input_task() {
    if (PORTCbits.RC0) {
        game_started = 1;
        TRISC = 0x00;
        game_level = 1;
        random_n_value = (TMR1H << 8) | TMR1L;
    }
}

void create_note_task(){
    uint16_t new_note = generate_random();

}


void forward_task(){

}

void check_press(){

}

void game_task() {
    if(game_started){
    tmr0_startreq = 1;
    uint16_t random_note =  create_note_task();

    switch(random_note){
        case 0 :
        case 1 :
        case 2 :
        case 3 :
        case 4 :
    }

    }
}

void game_over() {
    //TODO: appropriate texts displayed on 7-segment display
    TRISC = 0x01;
    game_started = 0;
    health = 9;
}

void main(void) {
    init_ports();
    config_tmr0();
    init_irq();
    
    while(1) {
        input_task();
        init_tmr1();
        timer_task();
        game_task();
    }
}
