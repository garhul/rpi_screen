#include <Arduino.h>
#include <Wire.h>
#include <FastLED.h>
#include "./defaults.h"
#include "U8g2lib.h"


#define __DEBOUNCE(ms) { \
  static unsigned long last_check = 0; \
  if(millis() > (last_check + ms))  { \
    last_check = millis(); \
    return true; \
  } \
  return false; \
}

#define BTN_PIN 10
#define LED_PIN 7
#define RGB_LED_PIN 2
#define PWM_FAN_PIN 0
#define RPM_FAN_PIN 1

#define SHUTDOWN_MODE 0xFF
#define REBOOT_MODE 0xF0

#define DEBOUNCE_DELAY_MS 50 //50ms
#define SHORT_PRESS_DELAY_MS 3000 //0ms
#define LONG_PRESS_DELAY_MS 6000 //50ms

#define MAIN_SCREEN 0
#define RPI_B_1 1
#define RPI_A_2 2
#define RPI_B_2 3
#define REBOOT_MENU 4
#define SHUTDOWN_MENU 5
#define ROTATING_VIEW 6
#define SCROLL_SPD 2 

#define RPI_A_ADDR 0x45
#define RPI_B_ADDR 0x42

#define FONT u8g2_font_6x13_mf

byte ADDRESSES[] = {RPI_A_ADDR, RPI_B_ADDR};

byte mode = 0;

CRGB leds[2];

void scan() {
  byte error,
      address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++) {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");

  delay(5000);  // wait 5 seconds for next scan
}

U8G2_SH1106_128X64_NONAME_F_HW_I2C display1(U8G2_R3, /* clock=*/SCL, /* data=*/SDA, /* reset=*/U8X8_PIN_NONE);

void blink_led(int times) {
  while (times > 0) {
    times--;
    digitalWrite(7, HIGH);
    delay(600);
    digitalWrite(7, LOW);
    delay(600);
  }
}

// bool read_button
inline bool debounce(int ms) {
  static unsigned long last_check = 0;

  if(millis() > (last_check + ms))  {
    last_check = millis();
    return true;
  }
  return false;
}

inline bool debounce_display(int ms) {
  static unsigned long last_check = 0;

  if(millis() > (last_check + ms))  {
    last_check = millis();
    return true;
  }
  return false;
}

bool debounce_fetch(int ms) {
  static unsigned long last_check = 0;

  if(millis() > (last_check + ms))  {
    last_check = millis();
    return true;
  }
  return false;
}

String getDataFromRPI(int addr) {

}

/**
 * interrupt routine for button press deboucing, if the buton is pressed once it will set the flag ? or what?
 * */
void isr() {  
  static bool last_state = HIGH;

  if (!debounce(DEBOUNCE_DELAY_MS)) return;

  if (debounce(LONG_PRESS_DELAY_MS)) {
    if ((last_state == LOW) && (digitalRead(BTN_PIN) == HIGH)) {
      mode = SHUTDOWN_MODE;
    }

    last_state = digitalRead(BTN_PIN);
    return;
  }


  if (debounce(SHORT_PRESS_DELAY_MS)) {
    if ((last_state == LOW) && (digitalRead(BTN_PIN) == HIGH)) {
      mode = REBOOT_MODE;
    }

    last_state = digitalRead(BTN_PIN);    
    return;
  }

  // screen = ++screen % 4; //4 screens so far
}

void setup(void) {
  Serial.begin(115200);
  Serial.println("Starting up");
  Wire.begin();
  
  FastLED.addLeds<WS2812, RGB_LED_PIN, GRB>(leds, 2);
    
  display1.begin();
  display1.setFont(FONT);  // choose a suitable fontu8g2_font_ncenB08_tr
  display1.setContrast(120);
  display1.clearDisplay();
  delay(2000);

  pinMode(LED_PIN, OUTPUT);
  pinMode(RGB_LED_PIN, OUTPUT);
  pinMode(PWM_FAN_PIN, OUTPUT);
  pinMode(RPM_FAN_PIN, INPUT_PULLUP);
  pinMode(BTN_PIN, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BTN_PIN), isr, CHANGE);

}


inline void centerText(byte y, String text){
  // FONT widht is 6 px 
  // max text width is 21 chars
  display1.drawBox(0, y - 7 , 128, 1);
  text = " "+text+" ";
  display1.drawStr((128 - (text.length() * 6)) / 2 , y, text.c_str());
}

inline void clr() {
  display1.clearBuffer();  // clear the internal memory
  display1.setDisplayRotation(U8G2_R2);
  display1.setDrawColor(0);
  display1.drawBox(0, 0, 128, 64);
  display1.setDrawColor(1);
  // display1.setDrawColor(0);
  // display1.drawBox(0, 30, 128, 1);
  // display1.setDrawColor(1);
}

String hostname[] = {" XxXxXxX "," XxXxXxX "};
String text[] = {"-- NO DATA --","-- NO DATA --"};
bool blink_text[] = {false, false};

void show_screen(int screen) {
  static int frame = 0;
  static int text_pos[] = {128,128};
  static unsigned int char_pos[] = {0,0};
  static bool blink = false;
  debounce_display(500);
  frame++;

  if (frame % 1 == 0) {
    for(byte i =0; i<2; i++) {
      text_pos[i] -= SCROLL_SPD;
      
      if (text_pos[i] < -5) {
        text_pos[i] = 0;
        char_pos[i]++;    
      }

      if (char_pos[i] > text[i].length()) {
        char_pos[i] = 0;
        text_pos[i] = 128;
      }
    }
  }
  
  if (frame % 10 == 0) blink = !blink; //blink it every 100 frames

  clr();
 

  FastLED.setBrightness(10);
  leds[0] = CRGB::Blue;
  leds[1] = CRGB::SeaGreen;

  FastLED.show();

  switch (screen) {
    case REBOOT_MENU:
      centerText(10, hostname[0]);
      centerText(42, hostname[1]);      
      // display1.drawStr(text_pos, 25, String("reboot").substring(text_pos, char_pos + 128 / 6).c_str());   
      // display1.drawStr(text_pos, 58, String("reboot") text_2.substring(char_pos, char_pos + 128 / 6).c_str());         
    break;

    case MAIN_SCREEN:      
      centerText(10, hostname[0]);
      centerText(42, hostname[1]);            
      if (blink_text[0] && blink) {
          centerText(25, text[0]);
      } else if (!blink_text[0]) {
        display1.drawStr(text_pos[0], 25, text[0].substring(char_pos[0], char_pos[0] + 128 / 6).c_str());   
      }
      if (blink_text[0] && blink) {
        centerText(58, text[1]);
      } else if (!blink_text[0]) {
        display1.drawStr(text_pos[1], 58,  text[1].substring(char_pos[1], char_pos[1] + 128 / 6).c_str());      
      }
    break;

 
    default:
    break;
  }

  display1.sendBuffer();

}

String s = "";

void get_rpi_data(byte pi) {  
  char buffer[16];
  char cmds[] = {'H','I','C','T','M','U'}; //'I'
  bool complete = false;
  bool errors = false;
  String data[sizeof(cmds)];
  
  byte i = 0;
  // delay(2000);
  if (!debounce_fetch(2000)) return; // Request rpi updates every 2 seconds
  
  for(byte cmd_i = 0; cmd_i < sizeof(cmds); cmd_i++) {
    delay(10);
    
    Wire.beginTransmission(ADDRESSES[pi]);  
    Wire.write(cmds[cmd_i]);  
    Wire.endTransmission();
    delay(10);
  
    Wire.requestFrom(ADDRESSES[pi], 16, true);
    delay(20);
  
    i = 0;
    complete = false;
    while(Wire.available() && !complete) {      
      buffer[i] = Wire.read();
            
      if(buffer[i] == 0x0A) {       
        complete = true;
        break;
      }
      i++;
    }
    if (i < 2) errors = true;    
    data[cmd_i] = String(buffer).substring(0, i);        
  }

  hostname[pi] = data[0];
  if (!errors) {
    text[pi] = "ip: " + data[1] + ", load: " + data[2] + ", temp: " + data[3] + "c, mem: " + data[4] + "%, uptime: " + data[5];
    blink_text[pi] = false;
  } else {
    text[pi] = "Communication error!";
    blink_text[pi] = true;
  }
  Serial.println(hostname[pi]);
  Serial.println("ip: " + data[1] + ", load: " + data[2] + ", temp: " + data[3] + "c, mem: " + data[4] + "%, uptime: " + data[5]);
}


void loop(void) {
  // blink_led(2);
  show_screen(0);
  get_rpi_data(0);
  get_rpi_data(1);

  // delay(1000);

 

  
  // Serial.println("---------");
  // clr();
  // display1.drawStr(25, 15, s.c_str());
  // display1.sendBuffer();
  
  // delay(10);
}