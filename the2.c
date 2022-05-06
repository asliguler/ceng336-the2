#include <xc.h>
#include <stdint.h>

void tmr0_isr();
void __interrupt(high_priority) highPriorityISR(void) {
    if (INTCONbits.TMR0IF) tmr0_isr();
    else if (PIR1bits.TMR1IF) tmr1_isr(); // TODO: can be also low priorty TBD
}
void __interrupt(low_priority) lowPriorityISR(void) {}

uint8_t game_started = 0, game_level = 1;

uint8_t health = 9;

bool display_side = 0; 

uint8_t rc0_state = 0;

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
    // global & timer0 & timer1 interruptlarını enable et
    INTCONbits.TMR0IE = 1;
    PIE1.TMR1IE = 1;
    INTCONbits.GIE = 1;
}

void config_tmr0() {
    T0CONbits.T08BIT = 1; // 8-bit mode
    T0CONbits.T0PS0 = 1; // 1:256 pre-scaler
    T0CONbits.T0PS1 = 1; // 1:256 pre-scaler
    T0CONbits.T0PS2 = 1; // 1:256 pre-scaler 
    // INFO: Timer0'nun degerlerini TMR0L registerindan oku
}

void tmr0_preload() {
    // timer0'nun baslangic degerleri
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
    // interrupt flagini disable edelim ki 
    // tekrar interrupt gelmis gibi algilamasin
    INTCONbits.TMR0IF = 0;
    switch (game_level) {
        case 1:
            // 500 msec tamamlamak icin 76 kere overflow olmali
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
            // 400 msec tamamlamak icin 61 kere overflow olmali
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
            // 300 msec tamamlamak icin 45 kere overflow olmali
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
    // interrupt flagini disable edelim ki 
    // tekrar interrupt gelmis gibi algilamasin
    PIR1bits.TMR1IF = 0;
    // overflow oldugu icin timer tamamlandi
    tmr1_state = TMR_DONE;
}

void init_tmr1() {
    T1CON = 0x00;
    // INFO: read value of TIMER1 from TMR1H:TMR1L registers
}


uint16_t generate_random() {
    uint16_t random_note =  (TMR1H << 8) | TMR1L;
    switch(game_level) {
        case 1:
            random_note = ((random_note >> 1) & 0x0007);
        case 2:
            random_note = ((random_note >> 3) & 0x0007);
        case 3:
            random_note = ((random_note >> 5) & 0x0007);
    }
    random_note %= 5;
    return random_note;
}


void seven_segment_display() {
    if (game_started){
        if (tmr1_state == TMR_DONE) { 
            display_side != display_side
            tmr1_state = TMR_IDLE;
            tmr1_startreq = 1;
        }
        if (display_side) {
            PORTH = 0x01; // to set leftmost display
            // set health
            switch (health) {
                case 9:
                    PORTJ = 0x6f; // to set 9
                    break;
                case 8:
                    PORTJ = 0x7f; 
                    break;
                case 7:
                    PORTJ = 0x07;
                    break;
                case 6:
                    PORTJ = 0x7d;
                    break;
                case 5:
                    PORTJ = 0x6d;
                    break;
                case 4:
                    PORTJ = 0x66;
                    break;
                case 3:
                    PORTJ = 0x4f;
                    break;
                case 2: 
                    PORTJ = 0x5b;
                    break;
                case 1:
                    PORTJ = 0x06;
                    break;
                case 0: 
                    PORTJ = 0x3f;
                    break;
            }
            
            
        } else {
            PORTH = 0x08; // to set rightmost display
            switch (game_level) {
                case 3:
                    PORTJ = 0x4f;
                    break;
                case 2: 
                    PORTJ = 0x5b;
                    break;
                case 1:
                    PORTJ = 0x06;
                    break;
            }
        }
    }
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
                tmr1_state = TMR_RUN;
            }
    }
}

void input_task() {
    if (PORTCbits.RC0) rc0_state = 1; // RC0'ye basildi
    else if (rc0_state == 1){ // RC0 release edildi
        rc0_state = 0;
        game_started = 1; 
        TRISC = 0x00; // RC0'yu bundan sonra notlari gostermek icin output olarak kullanacagiz
        game_level = 1;
        health = 9;
    }
}

void game_task() {

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
    init_tmr1();
    init_irq();
    while(1) {
        seven_segment_display();
        input_task();
        timer0_task();
        game_task();
    }
}
