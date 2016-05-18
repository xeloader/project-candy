#include <SoftwareSerial.h>

#define MODE_ADD 0x03
#define MODE_LOCK 0x04
#define MODE_REM 0x05

#define COLOR_RED 1
#define COLOR_GREEN 2

#define BEEP_GOOD 1
#define BEEP_BAD 2
#define BEEP_HELLO 3
#define BEEP_MODE 4

/*
 * CUSTOMIZE AS YOU WANT
 */
#define BUTTON_DELAY 25
#define DISPLAY_DELAY 1
#define LED_DURATION 250
#define STARTING_MODE MODE_LOCK //initialize the reader in this mode.

//RGB LED
const int rgb_red = 9;
const int rgb_green = 10;

//BUTTON
const int button = 4;
int button_state = LOW;
int prev_button_state = button_state;

//SPEAKER
const int speaker = 5;

//RFID READER
const int rfid_rx = 2;
const int rfid_tx = 3;
SoftwareSerial rfid(rfid_rx, rfid_tx);

//SHIFT REGISTER
const int sr_latch = 6;
const int sr_clock = 7;
const int sr_data = 8;

//LED MATRIX
const int lm_rows = 5;
int lm_row[5] = {0, 1, 11, 12, 13}; //arduino pins (.. 14, 8, 5, 1, 7 ..)

/*
 * Det som behövs är att flytta kolumnerna till pins.
 * Raderna ska flyttas till shift-registret.
 * Tog bort pin 9 och 2 eftersom vi inte behöver dom.
 * Tog bort blå led.
 */

//SYMBOLS
const byte sym_cross[] = {0b10001000, 
                          0b01010000, 
                          0b00100000,
                          0b01010000,
                          0b10001000};

const byte sym_plus[] = { 0b00100000, 
                          0b00100000, 
                          0b11111000,
                          0b00100000,
                          0b00100000};

const byte sym_lock[] = { 0b01010000, 
                          0b11111000, 
                          0b01010000,
                          0b11111000,
                          0b01010000};

//MODE
byte mode = STARTING_MODE;
String msg_ok = "ok";
String msg_bad = "bad";

void setup() {

  //RGB LED
  pinMode(rgb_red, OUTPUT);
  pinMode(rgb_green, OUTPUT);

  //BUTTON
  pinMode(button, INPUT);

  //SHIFT REGISTER
  pinMode(sr_latch, OUTPUT);
  pinMode(sr_clock, OUTPUT);
  pinMode(sr_data, OUTPUT);

  //DISPLAY
  for(int i = 0; i < lm_rows; i++) {
    pinMode(lm_row[i], OUTPUT);
  }
  reset_lm();

  //RFID READER
  rfid.begin(9600);
  delay(10);
  rfid.write(mode);

  //SERIAL MONITOR (fucks up the display).
  //Serial.begin(9600);

  //SPEAKER
  beep(BEEP_HELLO);

}

void loop() {

  prev_button_state = button_state;
  button_state = digitalRead(button); //HIGH eller LOW.
  if(button_state == HIGH && button_state != prev_button_state) {
    switch_mode();
    delay(BUTTON_DELAY);
  }

  update_status(); //keep the led matrix going.

  while(rfid.available()) {
    byte response = rfid.read();
    switch(mode) {
      case MODE_ADD:
        msg_ok = "added card";
        msg_bad = "cant add card";
      break;

      case MODE_REM:
        msg_ok = "removed card";
        msg_bad = "no such card to remove";
      break;

      case MODE_LOCK:
        msg_ok = "welcome back";
        msg_bad = "access denied";
      break;
    }

    reset_lm();

    if(response == mode) {
      set_rgb_color(COLOR_GREEN);
      beep(BEEP_GOOD);
      //Serial.println(msg_ok);
    } else if(response == (0xFF - mode)) {
      set_rgb_color(COLOR_RED);
      beep(BEEP_BAD);
      //Serial.println(msg_bad); 
    } else {
      set_rgb_color(COLOR_RED);
      //Serial.println("Shit.");
    }

    delay(LED_DURATION);
    
  }

  kill_rgb();

}

void beep(int type) {

  switch(type) {

    case BEEP_HELLO:
      tone(speaker, 2500, 50);
      delay(100);
      tone(speaker, 2500, 50);
    break;
    
    case BEEP_BAD:
      tone(speaker, 2000, 250);
    break;

    case BEEP_MODE:
      tone(speaker, 2000, 10);
    break;

    case BEEP_GOOD:
      tone(speaker, 2500, 50);
      delay(100);
      tone(speaker, 2500, 100);
    break;
  }
  
}

void switch_mode() {
  mode++;
  mode = mode % 3;
  mode += 3;
  rfid.write(mode);
  beep(BEEP_MODE);

}

void kill_rgb() {
  digitalWrite(rgb_red, HIGH);
  digitalWrite(rgb_green, HIGH);
}

void set_rgb_color(int color) {
  switch(color) {
    case COLOR_RED:
      digitalWrite(rgb_red, LOW);
      digitalWrite(rgb_green, HIGH);
    break;

    case COLOR_GREEN:
      digitalWrite(rgb_red, HIGH);
      digitalWrite(rgb_green, LOW);
    break;
  }
}

void reset_lm() {
   for(int i = 0; i < lm_rows; i++) {
    digitalWrite(lm_row[i], HIGH);
   }
}

void update_status() {

  byte row_bits;

  for(int i = 0; i < lm_rows; i++) {

    reset_lm();
    digitalWrite(sr_latch, LOW); //wont update while sending bits.

    switch(mode) {
      case MODE_ADD:
        row_bits = sym_plus[i];
      break;
      
      case MODE_REM:
        row_bits = sym_cross[i];
      break;
  
      case MODE_LOCK:
        row_bits = sym_lock[i];
      break;
    }

    shiftOut(sr_data, sr_clock, LSBFIRST, row_bits);
    digitalWrite(sr_latch, HIGH);
    digitalWrite(lm_row[i], LOW);
    delay(DISPLAY_DELAY);
    
  }
}

