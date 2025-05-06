#include <EasyButton.h>
#include <LiquidCrystal_74HC595.h>
#include <DHT.h>

#define LOCKED 0
#define UNLOCKED 1

#define PIN_DATA 6
#define PIN_CLOCK 8

//The shift registers share the same data and clk but use different lathces
#define PIN_LATCH_LCD 7
#define PIN_LATCH 2

//The enables for each digit of the 7 segment display
#define PIN_D1_ENABLE A5
#define PIN_D2_ENABLE A4
#define PIN_D3_ENABLE A3
#define PIN_D4_ENABLE A2

//The buttons for each lock input
#define PIN_LOCK_1 13
#define PIN_LOCK_2 12
#define PIN_LOCK_3 11
#define PIN_LOCK_4 10
#define PIN_LOCK_RST 9

// Lock/unlock status lights
#define PIN_LOCK_GREEN 4
#define PIN_LOCK_RED 5

#define PIN_TEMP_SENSOR A1
#define PIN_TEMP_SET A0
#define PIN_MOTOR 3

#define DHTTYPE DHT11

// Binary values coresponding to that display on the seven segment display
#define S_0 B01011111
#define S_1 B01000100
#define S_2 B10011101
#define S_3 B11010101
#define S_4 B11000110
#define S_5 B11010011
#define S_6 B11011011
#define S_7 B01000101
#define S_8 B11011111
#define S_9 B11000111
#define S_A B11001111
#define S_B B11011010  // lower-case b
#define S_C B00011011
#define S_D B11011100  // lower-case d
#define S_E B10011011
#define S_F B10001011
#define S_G B01011111  // same as 0
#define S_H B11001110
#define S_I B00001010
#define S_J B01011100
#define S_K B11101010  // approx
#define S_L B00011010
#define S_M B01101110  // approx
#define S_N B01001110  // approx
#define S_O B01011111  // same as 0
#define S_P B10001111
#define S_Q B11000111  // same as 9
#define S_R B10001110  // approx
#define S_S B11010011  // same as 5
#define S_T B00011000
#define S_U B01011110
#define S_V B01111110  // same as W
#define S_W B01111110
#define S_X B11101110  // approx
#define S_Y B11010110
#define S_Z B10111101  // approx
#define S_BLANK B00000000

bool lock_state = UNLOCKED;

int password[] = { 1, 2, 3, 4 };                                     //can be any combination of 1,2,3, or 4
const int password_length = sizeof(password) / sizeof(password[0]);  //Just in case we need to change the bounds of loop later
int attempt[password_length] = { 0, 0, 0, 0 };


//Timer Stuff
unsigned long int timer_global = 0;

unsigned long int timer_lock = 0;
unsigned long int timer_temp = 0;
unsigned long int timer_swap = 0;
unsigned long int timer_seven_seg = 0;
unsigned long int timer_status_lights = 0;

unsigned long int timeout_lock = 3000;       // Turns off the display
//For Some reason the timeout temp causes a flicker in the 7 segment when called
unsigned long int timeout_temp = 1000;
unsigned long int timeout_swap = 4 * timeout_temp;

//looks like I may have mis-named the seven_seg and lock timeouts
unsigned long int timeout_seven_seg = 4000;  //Resets the lock values to 0
unsigned long int timeout_status_lights =1000;

LiquidCrystal_74HC595 lcd(PIN_DATA, PIN_CLOCK, PIN_LATCH_LCD, 1, 2, 4, 5, 6, 7);

byte table[] = {
    S_0, S_1, S_2, S_3, S_4, S_5, S_6, S_7, S_8, S_9,
    S_A, S_B, S_C, S_D, S_E, S_F,
    S_G, S_H, S_I, S_J, S_K, S_L, S_M, S_N, S_O, S_P,
    S_Q, S_R, S_S, S_T, S_U, S_V, S_W, S_X, S_Y, S_Z,
    S_BLANK
};

byte seven_seg_enable[] = { PIN_D1_ENABLE, PIN_D2_ENABLE, PIN_D3_ENABLE, PIN_D4_ENABLE };  //Pins to turn on and off digits
byte seven_seg_msg[] = { 0, 0, 0, 0 };                                                     // Global message to display on the seven segment display


EasyButton lock1(PIN_LOCK_1);
EasyButton lock2(PIN_LOCK_2);
EasyButton lock3(PIN_LOCK_3);
EasyButton lock4(PIN_LOCK_4);

EasyButton lock_rst(PIN_LOCK_RST);

DHT dht(PIN_TEMP_SENSOR, DHTTYPE);
float current_temp;
float set_temp;


float convert_temp(int x) {
  return (.01368523949* float(x) + 16);
}

void display_temp() {
        lcd.setCursor(0,0);
    if (timer_global-timer_swap <0.5*timeout_swap) {
        lcd.print("Set Temp: ");
        lcd.print(set_temp);
        lcd.print("   ");
    } else{
        lcd.print("Temp: ");
        lcd.print(current_temp);
        lcd.print("     ");
    }
    if (timer_global-timer_swap >timeout_swap) {
        timer_swap = millis();
    }
}


void check_temp() {
    if(timer_global - timer_temp > timeout_temp) { // Every timerout # of seconds do this 
        timer_temp = millis();

        set_temp = convert_temp(analogRead(PIN_TEMP_SET));


        //This function causes flickering due to asynchronous reads 
        //that can take up to 250 ms (YIKES!!)
        current_temp= (dht.readTemperature());


        //This function causes flickering due to long screen writes
        //This is to be expected
        //During testing I wrote a customc LCD_UPDATE fucntion that 
        //would only write one character per loop iteration, which helped
        //some, but didn't completely prevent the issue due to the 
        //temp reads so I removed it 
        display_temp();

        
    }
}
void set_motor() {
    if (current_temp > set_temp) {
        digitalWrite(PIN_MOTOR, HIGH);
    } else {
        digitalWrite(PIN_MOTOR, LOW);
    }
}
// Fix the flash when it locks itself :?
void set_lock(bool state) {
    timer_status_lights = millis();
    lcd.setCursor(0,1);
    bool prev = lock_state;
    lock_state = state;
    if (state == UNLOCKED) {
        if (prev == LOCKED) {
            digitalWrite(PIN_LOCK_RED, LOW);
            digitalWrite(PIN_LOCK_GREEN, HIGH);
        }
        lcd.print("Unlocked :)");
    } else {
        if (prev == UNLOCKED) {
            digitalWrite(PIN_LOCK_RED, HIGH);
            digitalWrite(PIN_LOCK_GREEN, LOW);
        }
        lcd.print("Locked :(  ");
    }

    
}

void update_lock_attempt(int a) {
    timer_lock = millis();
    timer_seven_seg = millis();
    if (attempt[0] != 0) {
        return;
    }
    // Shift Left
    for (int j = 0; j < password_length - 1; j++) {
        attempt[j] = attempt[j + 1];
    }
    attempt[password_length - 1] = a;  //i is 0-3 but the expected values are 1-4
}

void lock1_callback() {
    update_lock_attempt(1);
}
void lock2_callback() {
    update_lock_attempt(2);
}
void lock3_callback() {
    update_lock_attempt(3);
}
void lock4_callback() {
    update_lock_attempt(4);
}

void reset() {
    for (int i = 0; i < password_length; i++) {
        attempt[i] = 0;
    }

    set_lock(LOCKED);
}

void lock_rst_callback() {
    timer_status_lights = millis();
    timer_seven_seg = millis();
    timer_lock = millis();
    for (int i = 0; i < 4; i++) {
        if (attempt[i] != password[i]) {
            set_lock(LOCKED);
            return;
        }
    }
    set_lock(UNLOCKED);
    return;
}

void write_seven_seg() {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            digitalWrite(seven_seg_enable[j], LOW);  //turns off every digit
        }

        digitalWrite(PIN_LATCH, LOW);  //Turns off changes

        shiftOut(PIN_DATA, PIN_CLOCK, MSBFIRST, seven_seg_msg[i]);  // Send Raw Byte Value

        digitalWrite(PIN_LATCH, HIGH);
        digitalWrite(seven_seg_enable[i], HIGH);  // turn the digit back on
        delay(1);                                 // one or two
    }

    for (int j = 0; j < 4; j++) {
        digitalWrite(seven_seg_enable[j], LOW);  //Turn off all digits
    }
}


void set_seg_display(byte a, byte b, byte c, byte d) {
    seven_seg_msg[0] = a;
    seven_seg_msg[1] = b;
    seven_seg_msg[2] = c;
    seven_seg_msg[3] = d;
}

void check_TO_status_lights() 
{
    if (timer_global - timer_status_lights > timeout_status_lights) {
        digitalWrite(PIN_LOCK_GREEN, LOW);
        digitalWrite(PIN_LOCK_RED, LOW);
    }
    else if(lock_state==LOCKED) 
    {
      digitalWrite(PIN_LOCK_RED, HIGH);
    }
    else 
    {
      digitalWrite (PIN_LOCK_GREEN,HIGH);
    }
}

//actually resets attempt to 0 0 0 0
void check_TO_lock() {
    if (timer_global - timer_lock > timeout_lock) {
        if (lock_state == UNLOCKED) {
            set_lock(LOCKED);
        }

        for (int i = 0; i < 4; i++) {
            attempt[i] = 0;
        }
    }
}

//only turns the display off
void check_TO_seven_seg() {
    if (timer_global - timer_seven_seg > timeout_seven_seg) {
        set_seg_display(S_BLANK, S_BLANK, S_BLANK, S_BLANK);
    } else {
        set_seg_display(table[attempt[0]],
                        table[attempt[1]],
                        table[attempt[2]],
                        table[attempt[3]]);
    }
}

void setup() {
    lcd.begin(16, 2);
    lcd.print("Hello, World!");

    set_lock(LOCKED);
    pinMode(PIN_LOCK_GREEN, OUTPUT);
    pinMode(PIN_LOCK_RED, OUTPUT);

    pinMode(PIN_D1_ENABLE, OUTPUT);
    pinMode(PIN_D2_ENABLE, OUTPUT);
    pinMode(PIN_D3_ENABLE, OUTPUT);
    pinMode(PIN_D4_ENABLE, OUTPUT);

    pinMode(PIN_LATCH_LCD, OUTPUT);

    pinMode(PIN_LATCH, OUTPUT);
    pinMode(PIN_DATA, OUTPUT);
    pinMode(PIN_CLOCK, OUTPUT);

    pinMode(PIN_MOTOR, OUTPUT);
    pinMode(PIN_TEMP_SET, INPUT);
    pinMode(PIN_TEMP_SENSOR, INPUT);


    for (int i = 0; i < 4; i++) {
        pinMode(seven_seg_enable[i], OUTPUT);
        digitalWrite(seven_seg_enable[i], LOW);  // Turns them all off
    }

    lock1.begin();
    lock2.begin();
    lock3.begin();
    lock4.begin();
    lock_rst.begin();

    lock1.onPressed(lock1_callback);
    lock2.onPressed(lock2_callback);
    lock3.onPressed(lock3_callback);
    lock4.onPressed(lock4_callback);

    lock_rst.onPressed(lock_rst_callback);
    lock_rst.onSequence(2, 500, reset);

    //Serial.begin(250000);

    dht.begin();

    //digitalWrite(PIN_MOTOR,HIGH);

    timer_temp = millis();
}

void loop() {

    timer_global = millis();

    write_seven_seg();
    check_TO_seven_seg();
    check_TO_lock();
    check_TO_status_lights();

    //check_temp causes flickering
    check_temp();
    set_motor();

    lock1.read();
    lock2.read();
    lock3.read();
    lock4.read();
    lock_rst.read();

    //Serial.print("timer_global ");
    //Serial.println(timer_global);
    //Serial.print("timer_temp ");
    //Serial.println(timer_temp);
//
//
    //Serial.print("ST: ");
    //Serial.println(set_temp);
//
//
    //Serial.print("T: ");
    //Serial.println(current_temp);
    //Serial.println();


    
}
