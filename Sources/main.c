// Author: Adam Dalibor Jurcik
// Login:  xjurci08
// Project: Alarm Clock (RTC)

#include "MK60D10.h"

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdbool.h>

#define LED_D9  0x20 // bit 5
#define LED_D10 0x10 // bit 4
#define LED_D11 0x8  // bit 3
#define LED_D12 0x4  // bit 2

#define MAX_STRING 101

#define NEW_LINE(){ 	\
	SendStr("\r\n"); 	\
}

#define PRINT_HASH(){											\
	PRINT("##############################################");	\
}

#define PRINT(msg) { 	\
    SendStr(msg);    	\
    SendStr("\r\n"); 	\
}

#define INPUT_TIME_ERROR(msg) { 	\
    SendStr(msg);              		\
    SendStr("\r\n");           		\
    return false;              		\
}

/*			VARIABLES
_________________________________________________________________________________________________________________________________________________________
_________________________________________________________________________________________________________________________________________________________
_________________________________________________________________________________________________________________________________________________________
*/

char input[MAX_STRING];

unsigned int sec_init;
unsigned int sec_alarm;
unsigned int sec_temp;
int song;
int LEDs;
int count_rep;
int wait;

// Finite-state machine states
enum FSM_State {
    START,  // 1 - start state
    SONG,  	// 2
    DIODS,  // 3
    REPEAT, // 4
    DELAY,  // 5
    ALARM,  // 6
    ON, 	// 7
    END		// 8 - final state
};

/*			BASIC FUNKCE
_________________________________________________________________________________________________________________________________________________________
_________________________________________________________________________________________________________________________________________________________
_________________________________________________________________________________________________________________________________________________________
*/

// delay
void delay(unsigned long long int bound) {
    for(unsigned long long int i=0; i<bound; i++);
}

// funkce pro pipani
void beep() {
    for (unsigned int q=0; q<500; q++) {
        PTA->PDOR = GPIO_PDOR_PDO(0x0010);
        delay(500);
        PTA->PDOR = GPIO_PDOR_PDO(0x0000);
        delay(500);
    }
}

// funkce pro poslani pismene
void SendCh(char c) {
    while( !(UART5->S1 & UART_S1_TDRE_MASK) && !(UART5->S1 & UART_S1_TC_MASK) );
    UART5->D = c;
}

// funkce pro poslani slova
void SendStr(char* s) {
    unsigned int i = 0;
    while (s[i] != '\0') {
        SendCh(s[i++]);
    }
}

// funkce pro prijem pismene
unsigned char ReceiveCh() {
    while( !(UART5->S1 & UART_S1_RDRF_MASK) );
    return UART5->D;
}

// funkce pro prijem slova
void ReceiveStr() {

    unsigned char c;
    unsigned int i = 0;

    for (unsigned int i = 0; i < MAX_STRING; i++) {
        input[i] = '\0';
    }

    while (i < (MAX_STRING - 1) ) { // Maximum charu
        c = ReceiveCh();
        SendCh(c);

        if (c == '\r') {
            break;
        }

        input[i] = c;
        i++;
    }

    input[i] = '\0'; // char 101 je \0

    NEW_LINE();
}


/*			TIME
_________________________________________________________________________________________________________________________________________________________
_________________________________________________________________________________________________________________________________________________________
_________________________________________________________________________________________________________________________________________________________
*/

// funkce ktera konvertuje cas z unsigned int na char
void convert(unsigned int* user_input, char* save_time){
	time_t temp = *user_input;
	struct tm L_time = *localtime(&temp);
	for (unsigned int i = 0; i < MAX_STRING; i++){
		save_time[i]='\0';
	}
	strftime(save_time, MAX_STRING, "%Y-%m-%d %H:%M:%S", &L_time);
}



// funkce pro ziskani user inputu (casu)
bool Input_time(char* user_input, unsigned int* save_time){
	int ret;
	struct tm Time;

	ret = strlen(user_input);
	if(ret < 14 || 19 < ret){
		INPUT_TIME_ERROR("Wrong lenght!");
	}

	ret = sscanf(user_input, "%d-%d-%d %d:%d:%d", &Time.tm_year, &Time.tm_mon , &Time.tm_mday, &Time.tm_hour, &Time.tm_min , &Time.tm_sec);
	if(ret != 6){
		INPUT_TIME_ERROR("Wrong format!");
	}

	// Test year
	if (Time.tm_year < 1970 || 2038 < Time.tm_year) {
		INPUT_TIME_ERROR("Wrong year!");
	}

	// Test month
    if (Time.tm_mon  < 1    || 12   < Time.tm_mon ) {
    	INPUT_TIME_ERROR("Wrong month!");
    }

    // Test month days 31
    if (Time.tm_mon==1 || Time.tm_mon==3 || Time.tm_mon==5 || Time.tm_mon==7 || Time.tm_mon==8 || Time.tm_mon==10 || Time.tm_mon==12) {
    	if (Time.tm_mday < 1 || 31 < Time.tm_mday) {
    		INPUT_TIME_ERROR("Wrong day!");
    	}
    }

    // Test month days 30
    if (Time.tm_mon==4 || Time.tm_mon==6 || Time.tm_mon==9 || Time.tm_mon==11) {
        if (Time.tm_mday < 1 || 30 < Time.tm_mday) {
        	INPUT_TIME_ERROR("Wrong day!");
        }
    }

    // Test feb and gap year
    if (Time.tm_mon==2) {
            if (Time.tm_year % 4) { // gap year
                if (Time.tm_mday < 1 || 29 < Time.tm_mday) INPUT_TIME_ERROR("Wrong day!");
            }
            else {
                if (Time.tm_mday < 1 || 28 < Time.tm_mday) INPUT_TIME_ERROR("Wrong day!");
            }
        }

    // Test hour
    if (Time.tm_hour < 0 || 23 < Time.tm_hour){
    	INPUT_TIME_ERROR("Wrong hour!");
    }

    // Test minute
    if (Time.tm_min  < 0 || 59 < Time.tm_min ){
    	INPUT_TIME_ERROR("Wrong minute!");
    }

    // Test seconds
    if (Time.tm_sec  < 0 || 59 < Time.tm_sec ){
    	INPUT_TIME_ERROR("Wrong second!");
    }

    Time.tm_year = Time.tm_year - 1900;
    Time.tm_mon = Time.tm_mon - 1;
    Time.tm_isdst = -1;
    time_t temp;
    temp = mktime(&Time);
    *save_time = (unsigned int)temp;
    return true;
}


/*			INIT
_________________________________________________________________________________________________________________________________________________________
_________________________________________________________________________________________________________________________________________________________
_________________________________________________________________________________________________________________________________________________________
*/

// MCU INIT
void MCUInit() {
    MCG_C4 |= ( MCG_C4_DMX32_MASK | MCG_C4_DRST_DRS(0x01) );
    SIM_CLKDIV1 |= SIM_CLKDIV1_OUTDIV1(0x00);
    WDOG_STCTRLH &= ~WDOG_STCTRLH_WDOGEN_MASK; // turn off watchdog
}

// UART INIT
void UART5Init() {
    UART5->C2  &= ~(UART_C2_TE_MASK | UART_C2_RE_MASK);
    UART5->BDH =  0x00;
    UART5->BDL =  0x1A; 	// Baud rate 115 200 Bd, 1 stop bit
    UART5->C4  =  0x0F; 	// Oversampling ratio 16, match address mode disabled
    UART5->C1  =  0x00; 	// 8 data bitu, bez parity
    UART5->C3  =  0x00;
    UART5->MA1 =  0x00; 	// no match address (mode disabled in C4)
    UART5->MA2 =  0x00; 	// no match address (mode disabled in C4)
    UART5->S2  |= 0xC0;
    UART5->C2  |= ( UART_C2_TE_MASK | UART_C2_RE_MASK ); // Zapnout vysilac i prijimac
}

// Port INIT
void PortsInit() {

    // Enable CLOCKs for: UART5, RTC, PORT-A, PORT-B, PORT-E
    SIM->SCGC1 = SIM_SCGC1_UART5_MASK;
    SIM->SCGC5 = SIM_SCGC5_PORTA_MASK | SIM_SCGC5_PORTB_MASK | SIM_SCGC5_PORTE_MASK;
    SIM->SCGC6 = SIM_SCGC6_RTC_MASK;

    // PORT A
    PORTA->PCR[4] = PORT_PCR_MUX(0x01);

    // PORT B
    PORTB->PCR[5] = PORT_PCR_MUX(0x01); 	// D9  LED
    PORTB->PCR[4] = PORT_PCR_MUX(0x01); 	// D10 LED
    PORTB->PCR[3] = PORT_PCR_MUX(0x01); 	// D11 LED
    PORTB->PCR[2] = PORT_PCR_MUX(0x01); 	// D12 LED

    // PORT E
    PORTE->PCR[8]  = PORT_PCR_MUX(0x03); 	// UART0_TX
    PORTE->PCR[9]  = PORT_PCR_MUX(0x03); 	// UART0_RX
    PORTE->PCR[10] = PORT_PCR_MUX(0x01); 	// SW2
    PORTE->PCR[12] = PORT_PCR_MUX(0x01); 	// SW3
    PORTE->PCR[27] = PORT_PCR_MUX(0x01); 	// SW4
    PORTE->PCR[26] = PORT_PCR_MUX(0x01); 	// SW5
    PORTE->PCR[11] = PORT_PCR_MUX(0x01); 	// SW6

    // set ports as output
    PTA->PDDR =  GPIO_PDDR_PDD(0x0010);
    PTB->PDDR =  GPIO_PDDR_PDD(0x3C);
    PTB->PDOR |= GPIO_PDOR_PDO(0x3C); 		// turn all LEDs OFF
}


/*			HUDBA
_________________________________________________________________________________________________________________________________________________________
_________________________________________________________________________________________________________________________________________________________
_________________________________________________________________________________________________________________________________________________________
*/

void Play_song(int num){
	// jedno dlouhe pipnuti
	if(num == 1){
		for(unsigned int i = 0; i < 20; i++) {
			beep();
		}
	}
	// 20x pipnuti
	else if (num == 2) {
		for(unsigned int i = 0; i < 20; i++) {
			beep();
			delay(40000);
		}
	}
	// buzz
	else if (num == 3) {
        for(unsigned int i = 0; i < 5; i++) {
            beep();
            delay(90000);
            beep();
            delay(90000);
            beep();
            delay(3000000); // cca 1 sek
        }
	}
}


/*			LEDKY
_________________________________________________________________________________________________________________________________________________________
_________________________________________________________________________________________________________________________________________________________
_________________________________________________________________________________________________________________________________________________________
*/

void LED_on(int num){
	// flash
	if (num == 1) {
		for(unsigned int i = 0; i < 20; i++) {
			PTB->PDOR &= ~GPIO_PDOR_PDO(0x3C);
			delay(200000);
			PTB->PDOR |= GPIO_PDOR_PDO(0x3C);
			delay(200000);
		}
	}
	// jedna po druhe
	else if (num == 2) {
		for(unsigned int i = 0; i < 20; i++) {
			GPIOB_PDOR ^= LED_D12;
			PTB->PDOR &= ~GPIO_PDOR_PDO(0x1);
			delay(200000);
			PTB->PDOR |= GPIO_PDOR_PDO(0x3C);

			GPIOB_PDOR ^= LED_D11;
			PTB->PDOR &= ~GPIO_PDOR_PDO(0x1);
			delay(200000);
			PTB->PDOR |= GPIO_PDOR_PDO(0x3C);

			GPIOB_PDOR ^= LED_D10;
			PTB->PDOR &= ~GPIO_PDOR_PDO(0x1);
			delay(200000);
			PTB->PDOR |= GPIO_PDOR_PDO(0x3C);

			GPIOB_PDOR ^= LED_D9;
			PTB->PDOR &= ~GPIO_PDOR_PDO(0x1);
			delay(200000);
			PTB->PDOR |= GPIO_PDOR_PDO(0x3C);
		}
	}
	// flash pro 2 a 2 (majak)
	else if(num == 3){
        for(unsigned int i=0; i<20; i++) {

            GPIOB_PDOR ^= (LED_D9 | LED_D10);
            PTB->PDOR &= ~GPIO_PDOR_PDO(0x1);
            delay(200000);
            PTB->PDOR |= GPIO_PDOR_PDO(0x3C);
            delay(200000);

            GPIOB_PDOR ^= (LED_D11 | LED_D12);
            PTB->PDOR &= ~GPIO_PDOR_PDO(0x1);
            delay(300000);
            PTB->PDOR |= GPIO_PDOR_PDO(0x3C);
            delay(300000);
        }
	}
}


/*			RTC
_________________________________________________________________________________________________________________________________________________________
_________________________________________________________________________________________________________________________________________________________
_________________________________________________________________________________________________________________________________________________________
*/

// RTC interrupt handler
void RTC_IRQHandler() {
    if(RTC_SR & RTC_SR_TAF_MASK) {

    	// Time Alarm Flag
    	NEW_LINE();
    	PRINT_HASH();PRINT_HASH();
    	PRINT("=== ALARM (wait for finish) ===");

        Play_song(song);   // launch sound effect
        LED_on(LEDs); 	// launch light effect

        PRINT("=== ALARM FINISHED ===");
        PRINT_HASH();PRINT_HASH();
        NEW_LINE();

        SendStr("INPUT: ");

        if (count_rep > 0) {
            count_rep--;
            RTC_TAR += wait;
        }
        else {
            RTC_TAR = 0;
        }

    }
}


void RTCInit() {

    RTC_CR |= RTC_CR_SWR_MASK;  	// SWR = 1, reset all RTC's registers
    RTC_CR &= ~RTC_CR_SWR_MASK; 	// SWR = 0

    RTC_TCR = 0x0000; 				// reset CIR and TCR

    RTC_CR |= RTC_CR_OSCE_MASK; 	// enable 32.768 kHz oscillator

    delay(0x600000);

    RTC_SR &= ~RTC_SR_TCE_MASK; 	// turn OFF RTC

    RTC_TSR = 0x00000000; 			// MIN value in 32bit register
    RTC_TAR = 0xFFFFFFFF; 			// MAX value in 32bit register

    RTC_IER |= RTC_IER_TAIE_MASK;

    NVIC_ClearPendingIRQ(RTC_IRQn);
    NVIC_EnableIRQ(RTC_IRQn);

    RTC_SR |= RTC_SR_TCE_MASK; 		// turn ON RTC
}


/*			MAIN
_________________________________________________________________________________________________________________________________________________________
_________________________________________________________________________________________________________________________________________________________
_________________________________________________________________________________________________________________________________________________________
*/

int main(){
    MCUInit();
    PortsInit();
    UART5Init();
    RTCInit();

    delay(500);

    int state = START;
    bool ret;
    int state_before;

    while(1){
    	switch(state){
    	case START:
    		PRINT("----------------------------------------------");
    		PRINT("ENTER date and time [YYYY-MM-DD HH:MM:SS].\r\nIf you erase something or click on arrows to move, it won't save the time!");
    		PRINT("----------------------------------------------");
    		SendStr("INPUT: ");
    		ReceiveStr();

    		ret = Input_time(input, &sec_init);
    		if(ret == true){
    			RTC_SR &= ~RTC_SR_TCE_MASK; 	// turn off rtc
    			RTC_TSR = sec_init;
    			RTC_SR |= RTC_SR_TCE_MASK; 		// turn on rtc
    			NEW_LINE();
    			PRINT_HASH();PRINT_HASH();
    			PRINT("SAVED.");
    			PRINT_HASH();PRINT_HASH();
    			NEW_LINE();
    			state_before = START;
    			state = ON;
    		}
    		else {
    			NEW_LINE();
    			PRINT_HASH();PRINT_HASH();
    			PRINT("Please, repeat the initialization date and time process.");
    			PRINT_HASH();PRINT_HASH();
    			NEW_LINE();
    		}
    		break;
    	case SONG:
    		PRINT("----------------------------------------------");
    		PRINT("Choose alarm song, type one of numbers [1-3].");
    		PRINT("You can try out songs by typing [try1] (same for song 2 and 3).");
    		PRINT("----------------------------------------------");
    		SendStr("INPUT: ");
    		ReceiveStr();
    		if(strcmp(input, "try1") == 0) {
    			Play_song(1);
    			NEW_LINE();
    		}
    		else if(strcmp(input, "try2") == 0) {
    			Play_song(2);
    			NEW_LINE();
    		}
    		else if(strcmp(input, "try3") == 0) {
    			Play_song(3);
    			NEW_LINE();
    		}
    		else if(strcmp(input, "1") == 0) {
    			song = 1;
    			NEW_LINE();
    			PRINT_HASH();PRINT_HASH();
    			PRINT("SAVED.");
    			PRINT_HASH();PRINT_HASH();
    			NEW_LINE();
    			state = DIODS;
    		}
    		else if(strcmp(input, "2") == 0) {
    		   	song = 2;
    		   	NEW_LINE();
    		   	PRINT_HASH();PRINT_HASH();
    			PRINT("SAVED.");
    			PRINT_HASH();PRINT_HASH();
      			NEW_LINE();
       			state = DIODS;
   	   		}
    		else if(strcmp(input, "3") == 0) {
    		   	song = 3;
    		   	NEW_LINE();
    		   	PRINT_HASH();PRINT_HASH();
     			PRINT("SAVED.");
     			PRINT_HASH();PRINT_HASH();
       			NEW_LINE();
       			state = DIODS;
       		}
    		else{
    			NEW_LINE();
    			PRINT_HASH();PRINT_HASH();
    			PRINT("I didn't get that. Please repeat.\r\nIt is possible that you pressed backspace.");
    			PRINT_HASH();PRINT_HASH();
    			NEW_LINE();
    		}
    		break;
    	case DIODS:
    		PRINT("----------------------------------------------");
    		PRINT("Choose alarm light, type one of numbers [1-3].");
    		PRINT("You can try out light types by typing [try1] (same for light 2 and 3).");
    		PRINT("----------------------------------------------");
    		SendStr("INPUT: ");
    		ReceiveStr();
    		if(strcmp(input, "try1") == 0) {
    		   	LED_on(1);
    		   	NEW_LINE();
     		}
       		else if(strcmp(input, "try2") == 0) {
    			LED_on(2);
    			NEW_LINE();
       		}
       		else if(strcmp(input, "try3") == 0) {
  	   			LED_on(3);
  	   			NEW_LINE();
       		}
       		else if(strcmp(input, "1") == 0) {
       			LEDs = 1;
       			NEW_LINE();
       			PRINT_HASH();PRINT_HASH();
    		   	PRINT("SAVED.");
    		   	PRINT_HASH();PRINT_HASH();
    		   	NEW_LINE();
    			state = REPEAT;
       		}
       		else if(strcmp(input, "2") == 0) {
       		   	LEDs = 2;
       		   	NEW_LINE();
       		   	PRINT_HASH();PRINT_HASH();
     			PRINT("SAVED.");
     			PRINT_HASH();PRINT_HASH();
      			NEW_LINE();
       			state = REPEAT;
       		}
     		else if(strcmp(input, "3") == 0) {
       		   	LEDs = 3;
       		   	NEW_LINE();
       		   	PRINT_HASH();PRINT_HASH();
   	   			PRINT("SAVED.");
   	   			PRINT_HASH();PRINT_HASH();
      			NEW_LINE();
       			state = REPEAT;
       		}
     		else {
     			NEW_LINE();
     			PRINT_HASH();PRINT_HASH();
     			PRINT("I didn't get that. Please repeat.\r\nIt is possible that you pressed backspace.");
     			PRINT_HASH();PRINT_HASH();
     			NEW_LINE();
     		}
    		break;
    	case REPEAT:
    		PRINT("----------------------------------------------");
    		PRINT("ENTER count how many times alarm repeats (MIN=0, MAX=5).");
    		PRINT("Zero means no repetition. When you choose not zero, you will choose delay after which the alarm start again.");
    		PRINT("Actual alarm + repeat (number which you choose).");
    		PRINT("----------------------------------------------");
    		SendStr("INPUT: ");
    		ReceiveStr();
    		ret = sscanf(input, "%d", &count_rep);
    		if(ret == 1){
    			if(0 <= count_rep && count_rep <= 5) {
    				if (count_rep == 0) {
    					state = ALARM;
    				}
    				else {
    					state = DELAY;
    				}
    				NEW_LINE();
    				PRINT_HASH();PRINT_HASH();
                    PRINT("SAVED.");
                    PRINT_HASH();PRINT_HASH();
                    NEW_LINE();
    			}
    			else{
    				NEW_LINE();
    				PRINT_HASH();PRINT_HASH();
    				PRINT("WRONG NUMBER OF REPTETITION!");
    				PRINT_HASH();PRINT_HASH();
    				NEW_LINE();
    			}
    		}
    		else {
    			NEW_LINE();
    			PRINT_HASH();PRINT_HASH();
    			PRINT("WRONG INPUT!");
    			PRINT_HASH();PRINT_HASH();
    			NEW_LINE();
    		}
    		break;
    	case DELAY:
    		PRINT("----------------------------------------------");
    		PRINT("Enter delay between alarm start trying to wake up you again, in seconds (MIN=30, MAX=300).");
    		PRINT("----------------------------------------------");
    		SendStr("INPUT: ");
            ReceiveStr();

            ret = sscanf(input, "%d", &wait);
            if(ret == 1){
            	if(30 <= wait && wait <= 300) {
            		NEW_LINE();
            		PRINT_HASH();PRINT_HASH();
            		PRINT("SAVED.");
            		PRINT_HASH();PRINT_HASH();
            		NEW_LINE();
            		state = ALARM;
            	}
            	else{
            		NEW_LINE();
            		PRINT_HASH();PRINT_HASH();
            		PRINT("WRONG DELAY!");
            		PRINT_HASH();PRINT_HASH();
            		NEW_LINE();
            	}
            }
            else{
            	NEW_LINE();
            	PRINT_HASH();PRINT_HASH();
            	PRINT("WRONG INPUT");
            	PRINT_HASH();PRINT_HASH();
            	NEW_LINE();
            }
            break;
    	case ALARM:
    		PRINT("----------------------------------------------");
    		PRINT("Enter ALARM date & time in format [YYYY-MM-DD HH:MM:SS].\r\nIf you erase something or click on arrows to move, it won't save the time!");
    		SendStr("Current date & time: ");
    		sec_temp = RTC_TSR;
    		convert(&sec_temp, input);
    		PRINT(input);
    		PRINT("----------------------------------------------");
    		SendStr("INPUT: ");
    		ReceiveStr();

    		ret = Input_time(input, &sec_alarm);
    		if((ret == true) && (RTC_TSR < sec_alarm)) {
    			RTC_TAR = sec_alarm;
    			NEW_LINE();
    			PRINT_HASH();PRINT_HASH();
    			PRINT("SAVED.");
    			PRINT_HASH();PRINT_HASH();
    			NEW_LINE();
    			state_before = ALARM;
    			state = ON;
    		}
    		else {
    			NEW_LINE();
    			PRINT_HASH();PRINT_HASH();
    			PRINT("Please, repeat the alarm time.");
    			PRINT_HASH();PRINT_HASH();
    			NEW_LINE();
    		}
    		break;
    	case ON:
    		PRINT("----------------------------------------------");
    		if (state_before == START){
    			PRINT("Commands: [ turnoff / restart / alarmoff / newalarm / help ].");
    			SendStr("Current date & time ---->>>> ");
    			sec_temp = RTC_TSR;
    			convert(&sec_temp, input);
    			PRINT(input);

    			SendStr("Alarm date & time ------>>>> ");
    			PRINT("OFF");
    		}
    		else {
    			PRINT("Commands: [ turnoff / restart / alarmoff / newalarm / help ].");
    			SendStr("Current date & time ---->>>> ");
    			sec_temp = RTC_TSR;
    			convert(&sec_temp, input);
    			PRINT(input);

    			SendStr("Alarm date & time ------>>>> ");
    			sec_temp = RTC_TAR;
    			if (sec_temp == 0) {
    				PRINT("OFF");
    			}
    			else {
    				convert(&sec_temp, input);
    				PRINT(input);
    			}
    		}

    		PRINT("----------------------------------------------");
    		SendStr("INPUT: ");
    		ReceiveStr();
    		if(strcmp(input, "turnoff") == 0) {
    			RTC_TAR = 0; // turn off alarm
    			state = END;
    		}
    		else if(strcmp(input, "restart") == 0) {
    			RTC_TAR = 0; // turn off alarm
    			state = START;
    		}
    		else if(strcmp(input, "alarmoff") == 0) {
    			RTC_TAR = 0; // turn off alarm
    			PRINT("Alarm disabled.");
    		}
    		else if(strcmp(input, "newalarm") == 0) {
    			RTC_TAR = 0; // turn off alarm
    			state = SONG;
    		}
    		else if(strcmp(input, "help") == 0) {
    			NEW_LINE();
    			PRINT("----------------------------------------------");
    			PRINT("Command: turnoff ---> Clock goes off.");
    			PRINT("Command: restart ---> Clock restarts time.");
    			PRINT("Command: alarmoff --> Alarm time deletes it self.");
    			PRINT("Command: newalarm --> New alarm time set.");
    			PRINT("Command: help ------> Prints commands.");
    			PRINT("----------------------------------------------");
    			NEW_LINE();
    		}
    		else {
    			NEW_LINE();
    			PRINT_HASH();PRINT_HASH();
    			PRINT("I didn't get that. Please repeat.\r\nIt is possible that you pressed backspace.");
    		}
    		PRINT_HASH();PRINT_HASH();
    		NEW_LINE();
    		break;
    	case END:
    		PRINT("System off");
    		while(1);
    		break;
    	default:
    		break;
    	}
    }
    return 0;
}




