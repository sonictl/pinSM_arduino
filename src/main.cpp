/**
 * This pin status timer is implemented with the Finite State Machine (FSM). 
 *   read more about FSM: https://arduinoplusplus.wordpress.com/2019/07/19/finite-state-machine-programming-basics-part-2/
 * If a pin status is toggled and a time is kept, status will change.
 * This code is for monitoring the working status, if working too long, it will alarm.
 * 
 * logic: IR_PIN == LOW => working
 * 
 * This code runs on esp8266 board 'NodeMCU with esp8266 12E module'
 */


#include <Arduino.h>

#define ALARM_PIN 14
#define IR_PIN 13

static uint32_t time_record = 0;
const uint32_t REST_TIME = 60*1000;    // parameters for RESTING state (msec)
bool rest_enough = false;
const uint32_t WORKING_TIME = 40*60*1000;   //params for WORKING state (msec)
const uint32_t ALARM_TIME = 60*1000;     //parames for ALARM2RESTING state (msec)
static enum { RESTING, WORKING, ALARMING, ALARM2RESTING, WORKING2RESTING } status = RESTING;

////////////////////////////////////
void blink(bool reset = false, const uint32_t LED_DELAY = 1000)  //the blink func is used to toggle the alarm_spker
{
  static enum { LED_TOGGLE, WAIT_DELAY } state = LED_TOGGLE;
  static uint32_t timeLastTransition = 0;
 
  if (reset)
  { 
    state = LED_TOGGLE;
    digitalWrite(ALARM_PIN, LOW);
  }
 
  switch (state)
  {
    case LED_TOGGLE:   // toggle the LED
      digitalWrite(ALARM_PIN, !digitalRead(ALARM_PIN));
      timeLastTransition = millis();
      state = WAIT_DELAY;
      break;
 
    case WAIT_DELAY:   // wait for the delay period
      if (millis() - timeLastTransition >= LED_DELAY)
        state = LED_TOGGLE;
      break;
 
    default:
      state = LED_TOGGLE;
      break;
  }
}

////////////////////////////////////////////////////////////////////////

void setup() 
{
  pinMode(ALARM_PIN, OUTPUT);
  pinMode(IR_PIN, INPUT);

  digitalWrite(ALARM_PIN, LOW);
  Serial.begin(115200);

  // initialize the state machine
  if (digitalRead(IR_PIN)==HIGH){
      status = RESTING;
      time_record = millis();
  }
  else{
      status = WORKING;
      time_record = millis();
  }
}

void loop(){
  switch(status){
    case RESTING:
      Serial.println("Resting..." + String( REST_TIME - millis() + time_record ));
      delay(500);

      if (millis() - time_record >= REST_TIME && rest_enough == false){
        rest_enough = true;
        digitalWrite(ALARM_PIN, HIGH);
        delay(200);
        digitalWrite(ALARM_PIN, LOW);
      }
      if (digitalRead(IR_PIN)==LOW) {
        if (rest_enough == true){
            status = WORKING;
            time_record = millis();
            rest_enough = false;
        }
        else{
            status = WORKING;
            // time_record = millis();
        }
      }
      break;
    
    case WORKING:
      Serial.println("Working...");
      delay(500);
      if (millis()-time_record >= WORKING_TIME){
        status = ALARMING;
        time_record = millis();
      }
      // if (millis()-time_record < WORKING_TIME && digitalRead(IR_PIN)==LOW){
      if (millis()-time_record < WORKING_TIME && digitalRead(IR_PIN)==HIGH){
        status = WORKING2RESTING;
        time_record = millis();
      }
      break;
    
    case ALARMING:
      Serial.println("Alarming...");
      delay(500);
      blink(false, 500);
      if (digitalRead(IR_PIN)==HIGH){
        status = ALARM2RESTING;
        time_record = millis();
        digitalWrite(ALARM_PIN, LOW);
      }
      break;

    case ALARM2RESTING:
      Serial.println("Alarming silent...");
      delay(500);
      if ((millis() - time_record >= ALARM_TIME) && digitalRead(IR_PIN)==HIGH){   //condition for jump next
        time_record = millis();
        status = RESTING;
      }
      if ((millis() - time_record < ALARM_TIME) && digitalRead(IR_PIN)==LOW){     //condition for jump back
        time_record = millis();
        status = ALARMING;
      }
      break;

    case WORKING2RESTING:
      Serial.println("WORKING2RESTING...");
      delay(500);
      if ((millis() - time_record >= 20000) && digitalRead(IR_PIN)==HIGH){
        time_record = millis();
        status = RESTING;
      }
      if ((millis() - time_record < 20000) && digitalRead(IR_PIN)==LOW){
        time_record = millis();
        status = WORKING;
      }
      break;
  }
}