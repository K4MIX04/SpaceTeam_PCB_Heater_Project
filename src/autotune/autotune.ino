#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include "max6675.h"
#include <PID_v1.h>
#include <PID_AutoTune_v0.h>

#define I2C_ADDRESS 0x3C
#define SSR_PIN 9
int thermoDO = 4;
int thermoCS = 5;
int thermoCLK = 6;

// --------------------------------------------------
// KOD JESZCZE NIE ODPALANY, nie wiadomo czy działą, 
// można odpalić i przetestować
//-----------------------------------------------------
double targetTemp = 170.0; // Temperatura wokół której będziemy stroić

SSD1306AsciiWire oled;
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO);

double input, output, setpoint;
PID myPID(&input, &output, &setpoint, 2, 5, 1, DIRECT);
PID_ATune aTune(&input, &output);

int WindowSize = 1000; 
unsigned long windowStartTime;

bool tuning = true;
unsigned long serialTime;

void setup() {
  Wire.begin();
  Wire.setClock(400000L);
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.setFont(System5x7);
  oled.clear();
  oled.println("Autotune Start...");
  oled.println("Czekaj...");

  Serial.begin(9600);
  pinMode(SSR_PIN, OUTPUT);
  digitalWrite(SSR_PIN, LOW);

  input = thermocouple.readCelsius();
  setpoint = targetTemp;

  aTune.SetControlType(1); 
  
  aTune.SetNoiseBand(1.0); 

  aTune.SetOutputStep(WindowSize); 
  
  aTune.SetLookbackSec(100); 
  
  windowStartTime = millis();
  
  Serial.println("ROZPOCZYNAM STROJENIE...");
}

void loop() {
  unsigned long now = millis();

  double val = thermocouple.readCelsius();
  
  if (!isnan(val)) {
    input = val;
  } else {
    digitalWrite(SSR_PIN, LOW);
    return;
  }

  if (tuning) {
    byte val = (aTune.Runtime());
    
    if (val != 0) {
      tuning = false;
    }
    
    if (now - windowStartTime > WindowSize) {
      windowStartTime += WindowSize;
    }
    
    if (output > (now - windowStartTime)) {
      digitalWrite(SSR_PIN, HIGH);
    } else {
      digitalWrite(SSR_PIN, LOW);
    }
    
    static unsigned long lastOled = 0;
    if (now - lastOled > 200) {
      lastOled = now;
      oled.setCursor(0, 3);
      oled.print("Temp: "); 
      oled.print(input); 
      oled.print(" / ");
      oled.print((int)targetTemp);
      oled.print(" C   ");
      
      oled.setCursor(0, 5);
      oled.print("Moc: ");

      if (digitalRead(SSR_PIN)) oled.print("ON  "); else oled.print("OFF ");
      
      oled.setCursor(0, 7);
      oled.print("TRWA ANALIZA...");
    }
  } 
  else { 
    digitalWrite(SSR_PIN, LOW);
    
    double kp = aTune.GetKp();
    double ki = aTune.GetKi();
    double kd = aTune.GetKd();
    
    oled.clear();
    oled.setCursor(0,0); oled.println("WYNIKI:");
    oled.println("----------------");
    
    oled.print("Kp: "); oled.println(kp);
    oled.print("Ki: "); oled.println(ki);
    oled.print("Kd: "); oled.println(kd);

    Serial.println("============ WYNIKI ============");
    Serial.print("double Kp = "); Serial.print(kp); Serial.println(";");
    Serial.print("double Ki = "); Serial.print(ki); Serial.println(";");
    Serial.print("double Kd = "); Serial.print(kd); Serial.println(";");
    Serial.println("================================");
    
    while(1) {
        digitalWrite(13, HIGH); delay(100);
        digitalWrite(13, LOW); delay(100);
    }
  }

  if (millis() - serialTime > 500) {
    serialTime = millis();
    Serial.print("Temp:"); Serial.print(input);
    Serial.print(",Cel:"); Serial.print(targetTemp);
    Serial.println();
  }
}