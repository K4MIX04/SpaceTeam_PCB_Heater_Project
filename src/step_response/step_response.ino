#include "max6675.h"

#define SSR_PIN 9
int thermoDO = 4;
int thermoCS = 5;
int thermoCLK = 6;

MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

int WindowSize = 1000;       
double TEST_POWER_PERCENT = 5.0; //Ustalane jest tutaj procent mocy (5% podgrzało do około 110 C)
double TARGET_OUTPUT = 0;         // Docelowa moc
double current_output = 0;        // Aktualna moc

unsigned long windowStartTime;

// Czekanie 15 sekund zanim grzanie.
unsigned long START_DELAY_MS = 15000; 

void setup() {
  Serial.begin(9600);
  pinMode(SSR_PIN, OUTPUT);
  digitalWrite(SSR_PIN, LOW);
  
  TARGET_OUTPUT = (TEST_POWER_PERCENT / 100.0) * WindowSize;
  
  windowStartTime = millis();
}

void loop() {
  unsigned long now = millis();

  if (now < START_DELAY_MS) {
    current_output = 0; 
  } else {
    current_output = TARGET_OUTPUT; 
  }

  if (now - windowStartTime > WindowSize) {
    windowStartTime += WindowSize;
  }
  
  if ((double)(now - windowStartTime) < current_output) {
    digitalWrite(SSR_PIN, HIGH);
  } else {
    digitalWrite(SSR_PIN, LOW);
  }

  static unsigned long last_print = 0;
  if (now - last_print >= 500) {
    last_print = now;
    double temp = thermocouple.readCelsius();
    
    Serial.print(current_output); 
    Serial.print(",");
    Serial.println(temp);        
  }
}