#include <xc.h>
#include <stdint.h>
#pragma config OSC = HSPLL

void game_over();
void tmr0_isr();
void tmr1_isr();

void __interrupt() highPriorityISR(void) {
    if (INTCONbits.TMR0IF) tmr0_isr();
    else if (PIR1bits.TMR1IF) tmr1_isr();
    return;// TODO: can be also low priorty TBD
}


uint8_t game_started = 0, game_level = 1;

uint8_t health = 9;

uint8_t display_side = 0; // Used for 7 display segment that is alternating with the timer1. 

//Timer states used for both timers.
//Timer waits for a request while TMR_IDLE. 
//After the request state becomes TMR_RUN and stays as for a period according to level (500ms for level1, 400ms for level2). 
//After the period, it becomes TMR_DONE. 
typedef enum {TMR_IDLE, TMR_RUN, TMR_DONE} tmr_state_t;  
tmr_state_t tmr0_state = TMR_IDLE, tmr1_state = TMR_IDLE;

uint8_t tmr0_startreq = 0, tmr1_startreq = 0; //Requests for timers. 
uint8_t tmr0_ticks_left, tmr1_ticks_left; 
uint8_t tmr0_count = 0;

uint16_t random_n_value = 0; //N value that is used for the random note generation algorithm. 

//Level states used for the levels. Gives information about which level state is a level in (should we light notes or blank notes, did the level finish etc.). 
// LVL_BEG is the beginning of a level, where we light first note.
// LVL_CONT is the states where we light all the other notes. (we will have 4 for level 1, 9 for level 2, 14 for level 3).
// LVL_END is the end state of a level.
typedef enum { LVL_BEG, LVL_CONT, LVL_BLANK, LVL_END} level_state_t;
level_state_t level_state = LVL_BEG; 


// TODO: Commment 
typedef enum {GAME_OVER_END, GAME_OVER_LOSE, GAME_OVER_DEFAULT} game_over_state_t;
game_over_state_t game_over_state = GAME_OVER_DEFAULT; 

//Press states for five buttons that user will use :  RG0, RG1, RG2, RG3, RG4. 
typedef enum {NOT_PRSS, PRSS } press_state_t; 
press_state_t press_state0=NOT_PRSS, press_state1=NOT_PRSS, press_state2=NOT_PRSS , press_state3=NOT_PRSS , press_state4=NOT_PRSS , press_state5=NOT_PRSS;
press_state_t press_state_rc0 = NOT_PRSS;


uint8_t level_max_note = 5;  //Number of notes in a level. 
uint8_t note_count = 0;     //Note count integer.

// Blank note counter integer. Will be used for counting 5 times when we are forwarding after note_count reached its max. 
uint8_t blank_note_count = 0; 

uint8_t miss_penalty = 1; //miss penalty boolean. Becomes 0 when user catches the correct note.
uint8_t correct_note; //Correct note that should be pressed. 0->4  for RG0->RG4. -1 when a user should not press any note.

#define TIMER0_PRELOAD_LEVEL1 74
#define TIMER0_PRELOAD_LEVEL2 4
#define TIMER0_PRELOAD_LEVEL3 190


void init_ports() {
    //IO config
    TRISA = 0x00;
    TRISB = 0x00;
    TRISC = 0x01;
    TRISD = 0x00;
    TRISE = 0x00;
    TRISF = 0x00;
    TRISG = 0x1f;
    TRISH = 0x00;
    TRISJ = 0x00;
    
    //CLRF all
    LATA = 0x00;
    LATB = 0x00;
    LATC = 0x00;
    PORTC = 0x00;
    LATD = 0x00;
    LATE = 0x00;
    LATF = 0x00;
    LATG = 0x00;
    PORTG = 0x00;
    LATH = 0x00;
    LATJ = 0x00;
}

void init_variables(){
    health = 9;
    game_level = 1;
    press_state_rc0 = NOT_PRSS;
    press_state0=NOT_PRSS; 
    press_state1=NOT_PRSS; 
    press_state2=NOT_PRSS; 
    press_state3=NOT_PRSS;
    press_state4=NOT_PRSS;
    press_state5=NOT_PRSS;
    tmr0_count = 0;
    tmr0_startreq = 0; 
    tmr1_startreq = 0;
    level_max_note = 5; 
    note_count = 0; 
    blank_note_count = 0;
}

void init_irq() {
    // global & timer0 & timer1 interruptlarini enable et
    INTCONbits.TMR0IE = 1;
    INTCONbits.PEIE = 1;
    PIE1bits.TMR1IE = 1;
    INTCONbits.GIE = 1;
}

void config_tmr0() {
    T0CON = 0x47;
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
            if (tmr0_count < 76) {
                tmr0_count++;
                TMR0L = 0x00;
            }
            else {
                tmr0_count = 0;
                tmr0_state = TMR_DONE;
                tmr0_preload();
            }
            break;
        case 2:
            // 400 msec tamamlamak icin 61 kere overflow olmali
            if (tmr0_count < 61) {
                tmr0_count++;
                TMR0L = 0x00;
            }
            else {
                tmr0_count = 0;
                tmr0_state = TMR_DONE;
                tmr0_preload();
            }
            break;
        case 3:
            // 300 msec tamamlamak icin 45 kere overflow olmali
            if (tmr0_count < 45) {
                tmr0_count++;
                TMR0L = 0x00;
            }
            else {
                tmr0_count = 0;
                tmr0_state = TMR_DONE;
                tmr0_preload();
            }
            break;
    }
}

void tmr1_isr() {
    // interrupt flagini disable edelim ki 
    // tekrar interrupt gelmis gibi algilamasin
    PIR1bits.TMR1IF = 0;
    // overflow oldugu icin timer tamamlandi
    tmr1_state = TMR_DONE;
    TMR1L = 0x00;
    TMR1H = 0x00;
}

void init_tmr1() {
    T1CON = 0x00;
    PIR1bits.TMR1IF = 0;
    TMR1L = 0x00;
    TMR1H = 0x00;
    // INFO: read value of TIMER1 from TMR1H:TMR1L registers
}


uint16_t generate_random() {
    uint16_t random_note ;
    switch(game_level) {
        case 1:
            random_n_value = (random_n_value >> 1); 
            random_note = (random_n_value & 0x0007);
        case 2:
            random_n_value = (random_n_value >> 3); 
            random_note = (random_n_value & 0x0007);
        case 3:
            random_n_value = (random_n_value >> 7); 
            random_note = (random_n_value & 0x0007);
    }
    random_note %= 5;
    return random_note;
}


void seven_segment_display() {
    if (game_over_state == GAME_OVER_DEFAULT){
        if (tmr1_state == TMR_DONE) { 
            if(display_side == 1) display_side = 0;
            else display_side = 1;
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
    } else if (game_over_state == GAME_OVER_END) {
        if (tmr1_state == TMR_DONE) { 
            if (display_side < 3) display_side++;
            else display_side = 0;
            tmr1_state = TMR_IDLE;
            tmr1_startreq = 1;
        }
        switch (display_side) {
            case 0:
                PORTH = 0x01;
                PORTJ = 0x79;
                break;
            case 1:
                PORTH = 0x02;
                PORTJ = 0x54;
                break;
            case 2:
                PORTH = 0x04;
                PORTJ = 0x5e;
                break;
        }
    } else if (game_over_state == GAME_OVER_LOSE) {
        if (tmr1_state == TMR_DONE) { 
            if (display_side < 4) display_side++;
            else display_side = 0;
            tmr1_state = TMR_IDLE;
            tmr1_startreq = 1;
        }
        switch (display_side) {
            case 0:
                PORTH = 0x01;
                PORTJ = 0x38;
                break;
            case 1:
                PORTH = 0x02;
                PORTJ = 0x3f;
                break;
            case 2:
                PORTH = 0x04;
                PORTJ = 0x6d;
                break;
            case 3:
                PORTH = 0x08;
                PORTJ = 0x79;
                break;
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
            break;
        case TMR_RUN:
            break;
        case TMR_DONE:
            tmr0_state = TMR_IDLE; 
            break;
    }
    switch (tmr1_state) {
        case TMR_IDLE:
            if (tmr1_startreq) {
                tmr1_startreq = 0;
                TMR1L = 0x00;
                TMR1H = 0x00;
                T1CONbits.TMR1ON = 1;
                tmr1_state = TMR_RUN;
            }
            break;
        case TMR_RUN:
            break;
        case TMR_DONE:
            // TODO: Should it become TMR_IDLE just like tmr0 ? 
            break;
    }
}

void input_task() {
    if (game_started == 0){    
        if (PORTCbits.RC0) press_state_rc0 = PRSS; // RC0'ye basildi
        else if (press_state_rc0 == PRSS){ // RC0 release edildi
            press_state_rc0 = NOT_PRSS;
            game_started = 1; 
            TRISC = 0x00; // RC0'yu bundan sonra notlari gostermek icin output olarak kullanacagiz
            game_level = 1;
            health = 9;
            game_over_state = GAME_OVER_DEFAULT;
            random_n_value = (TMR1H << 8) | TMR1L;
        }
    }
}

//Forwarding previous notes to next notes. 
void forward_task(){
    LATF = LATD;
    LATD = LATC;
    LATC = LATB;
    LATB = LATA;
}

//Check press task every LATG register with the information of correct note that we assigned in the note_Task(). Same logic for all 4.
void check_press_task(){
    // LATG0
    if(PORTGbits.RG0){ 
        press_state0 = PRSS;
        if(correct_note == 0) LATF = 0x00;
    }
    else if(press_state0 == PRSS){ // If pressed and released check if it is correct note.
        press_state0 = NOT_PRSS;
        if(correct_note == 0) { //If correct note, no miss_penalty for this note (period) and corresponding RF output pin is gone.
            miss_penalty = 0;
            LATF = 0x00;
        }
        else {
            
            if(--health == 0) { // If wrong note, health penalty and game over.
                game_over_state = GAME_OVER_LOSE;
                game_over();
            }
        }
    }

    // LATG1
    if(PORTGbits.RG1){
        press_state1 = PRSS;
        if(correct_note == 1) LATF = 0x00;
    }
    else if(press_state1 == PRSS){
        press_state1 = NOT_PRSS;
        if(correct_note == 1) {
            miss_penalty = 0;
        }
        else {
            
            if(--health == 0) {
                game_over_state = GAME_OVER_LOSE;
                game_over();
            }
        }
    }

    // LATG2
    if(PORTGbits.RG2){
        press_state2 = PRSS;
        if(correct_note == 2) LATF = 0x00;
    }
    else if(press_state2 == PRSS){
        press_state2 = NOT_PRSS;
        if(correct_note == 0) {
            miss_penalty = 0;
        }
        else {
            
            if(--health == 0) {
                game_over_state = GAME_OVER_LOSE;
                game_over();
            }
        }
    }

    // LATG3
    if(PORTGbits.RG3){
        press_state3 = PRSS;
        if(correct_note == 3) LATF = 0x00;
    }
    else if(press_state3 == PRSS){
        press_state3 = NOT_PRSS;
        if(correct_note == 0) {
            miss_penalty = 0;
        }
        else {
            
            if(--health == 0) {
                game_over_state = GAME_OVER_LOSE;
                game_over();
            }
        }
    }

    // LATG4
    if(PORTGbits.RG4){
        press_state4 = PRSS;
        if(correct_note == 4) LATF = 0x00;
    }
    else if(press_state4 == PRSS){
        press_state4 = NOT_PRSS;
        if(correct_note == 0) {
            miss_penalty = 0;
        }
        else {
            
            if(--health == 0) { 
                game_over_state = GAME_OVER_LOSE;
                game_over();
            }
        }
    }
}

//Checks miss note. If note is missed, health -- . 
void check_miss_task(){
    if(miss_penalty){
        if(--health == 0) { // If health is over, health penalty and game over.
                game_over_state = GAME_OVER_LOSE;
                game_over();
            }
    }
}

// Gets the correct note according to LATF.
void get_correct_note(){
    
    switch(LATF){
        case 0x01 :
            correct_note = 0 ;
            break;
        case 0x02 :
            correct_note = 1 ;
            break;
        case 0x04 :
            correct_note = 2 ;
            break;
        case 0x08 :
            correct_note = 3 ;
            break;
        case 0x10 :
            correct_note = 4 ;
            break;
    }
}

// We light a blank note (do not light anything) for 5 times in order to make the last note reach to last output pins.
void blank_note_task(){
    miss_penalty = 1;
    LATA = 0x00;
}

// Note task manages the lighting the correct note at the A output pins, assignment of correct_note and miss_penalty. 
void note_task(){
    uint16_t random_note = generate_random();
    
    if(note_count<5){
        correct_note = -1; 
        miss_penalty = 0;
    }
    else{
        get_correct_note(); // correct_note becomes the correct note according to LATF.
        miss_penalty = 1;
    }
    
    switch(random_note){ //Lighting the new note. 
        case 0 :
            LATA = 0x01;
            break;
        case 1 :
            LATA = 0x02;
            break;
        case 2 :
            LATA = 0x04;
            break;
        case 3 :
            LATA = 0x08;
            break;
        case 4 :
            LATA = 0x10;
            break;
    }
}

//Game task manages levels. When TMR_IDLE for tmr0, note and forwarding tasks are done.
// When TMR_RUN, press checking is done. When TMR_DONE if a note is missed or not is checked. 

void game_task() {
    if(game_started){

        switch(level_state){
            case LVL_BEG : // No forward task as it is the first note. Note is lighted and press is checking in this state. No need to check miss as the first note is not reached the end.

                switch (tmr0_state){
                    case TMR_IDLE:
                        tmr0_startreq = 1;
                        note_task();
                        note_count++;
                        level_state = LVL_CONT;
                        break;
                    case TMR_RUN:
                        check_press_task();
                        break;
                }
                break;

            case LVL_CONT: // First forwarding, then lighting the new note. If all the notes are lighted,level_state becomes LVL_BLNK. 
                switch (tmr0_state){
                    case TMR_IDLE:
                        if(note_count != level_max_note){
                            tmr0_startreq = 1;
                            note_count++;
                            forward_task();
                            note_task();
                        }
                        else{
                            level_state = LVL_BLANK;
                        }

                        break;
                    case TMR_RUN:
                        check_press_task();
                        break;
                    case TMR_DONE:
                        check_miss_task();
                        break;
                }
                break;

            case LVL_BLANK: //No new notes (we have blank_notes). If we blanked 5 times, level is done. 
                switch (tmr0_state){
                    case TMR_IDLE:
                        if(blank_note_count<5){
                            tmr0_startreq = 1; 
                            blank_note_count++;
                            forward_task();
                            blank_note_task();
                        }
                        else{
                            level_state = LVL_END;
                        }
                        break;

                    case TMR_RUN:
                        check_press_task();
                        break;

                    case TMR_DONE:
                        check_miss_task();
                        break;
                }
                break;

            case LVL_END: // Checking if we finished the game. If not , next level.
                blank_note_count = 0 ;
                note_count = 0; 
                
                level_max_note += 5;
                
                if(++game_level > 3){
                    level_max_note = 5;
                    game_over_state = GAME_OVER_END;
                    game_over(); // END_GAME(), WE ARE IN THE END GAME NOW. 
                }
                
                level_state = LVL_BEG;
                break;
        }
    }
}

void game_over() {
    game_started = 0;
    init_ports();
    init_variables();
}

void main(void) {
    init_ports();
    init_variables();
    config_tmr0();
    init_tmr1();
    init_irq();
    tmr1_startreq = 1;
    while(1) {
        seven_segment_display();
        input_task();
        timer_task();
        game_task();
    }
}
