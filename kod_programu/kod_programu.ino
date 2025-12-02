#include <Wire.h>
#include "SSD1306Ascii.h"
#include "SSD1306AsciiWire.h"
#include "max6675.h"
#include <PID_v1.h>

//----------DEFINICJA HARDWAROWYCH WARTOŚCI--------------------
#define I2C_ADDRESS 0x3C  //adres I2C - sprawdzić czy poprawny
#define RESET_BUTTON 2
#define SCREEN_CHOICE_BUTTON 7
#define TEMP_SET_BUTTON 8
#define POTENTIOMETER A0
#define SSR_PIN 9
int thermoDO = 4;  // SO (Data Out)
int thermoCS = 5;  // CS (Chip Select)
int thermoCLK = 6; // SCK (Clock)
//-------------------------------------------------------------

//---------wybór trybu pracy-------------------
bool PID_mode = false;
bool hysteresis_mode = true;
bool draw_plot = false;
//---------------------------------------------

//PID - ewentualnie--------------------------------------------
double Setpoint, Input, Output;
double Kp = 100, Ki = 0.5, Kd = 2;
PID myPID(&Input, &Output, &Setpoint, Kp, Ki, Kd, P_ON_M, DIRECT);
int WindowSize = 1000; // okno czsaowe 1s
unsigned long windowStartTime;
//-------------------------------------------------------------

// peryferia
SSD1306AsciiWire oled;  // oled - obiekt wyświetlacza
MAX6675 thermocouple(thermoCLK, thermoCS, thermoDO); // thermocouple - obiekt termopary

double temp_set;
int action = -1; // - definiujemy możliwe stany, -1 to żaden ze stanów, 1 to gzranie do momentu wejscia w histereze, 2 to utrzymywanie temperatury
bool temp_set_defined = false;
unsigned long previous_time = 0;
int active_screen = -1; // -1 - ekran głowny, 1 - ekran do zadwaania temperatury
double temp_current = 0;


//obsluga przycisku
int last_change_button_state = HIGH; 
int last_set_button_state = HIGH;

//obsluga potenjometru
int potentiometer_value = 0;
int last_pot_value = -1;
int last_raw_reading = 0;

// obsluga przerwania od resetu
volatile bool reset_request = false;

void oled_setup(){
  Wire.begin();
  Wire.setClock(400000L);

  oled.begin(&Adafruit128x64, I2C_ADDRESS); // inicjalizacja wyswietlacza
  oled.setFont(System5x7);  // ustawienie czcionki
  oled.clear();
}

void starting_screen(){
  const char* text1 = "Projekt";
  const char* text2 = "Zelazko";
  int x1_position = (128 - oled.strWidth(text1)) / 2;
  int y1_position = 3;
  int x2_position = (128 - oled.strWidth(text2)) / 2;
  int y2_position = 4;

  oled.setCursor(x1_position, y1_position);
  oled.print(text1);
  oled.setCursor(x2_position, y2_position);
  oled.print(text2);

  oled.setCursor(0, 7);
  if(PID_mode && !hysteresis_mode) oled.print("Tryb: PID");
  else oled.print("Tryb: Histereza");

  delay(2000);
  oled.clear();
}

void main_screen(double temp_current){
  oled.setCursor(0, 2);
  oled.print("Cel =    ");
  if (temp_set_defined){
    oled.print(temp_set, 0);
  }
  else{
    oled.print("...   ");
  }
  oled.print(" ");                    //wypisywanie temperatur zadanej i aktualnej
  oled.write(0xF7);
  oled.print("C   ");

  oled.setCursor(0, 4);
  oled.print("Obecna = ");

  if (isnan(temp_current)) {
     oled.print("BLAD!   ");  //obsluga ewentualnych bledów
  } else {
     oled.print(temp_current);
     oled.print(" ");
     oled.write(0xF7);
     oled.print("C   ");
  }

  oled.clear(0, 127, 6, 7); // czyszczenie 6 i 7 linii

  switch(action){
    case 1:
      {
        if(hysteresis_mode){
          const char* line1 = "Osiaganie";
          const char* line2 = "zadanej temp";
          
          oled.setCursor((128 - oled.strWidth(line1)) / 2, 6);
          oled.print(line1);
          
          oled.setCursor((128 - oled.strWidth(line2)) / 2, 7);
          oled.print(line2);
        }
        else if(PID_mode){
          oled.setCursor(0, 6);
          oled.print("PID Moc: ");
          oled.print(map(Output, 0, WindowSize, 0, 100)); //wyjscie PID przeliczamy na procenty
          oled.print("%");
        }
      }
      break;
    case 2:
      {
        if(hysteresis_mode){
          const char* line1 = "Utrzymywanie";
          const char* line2 = "zadanej temp";
          
          oled.setCursor((128 - oled.strWidth(line1)) / 2, 6);
          oled.print(line1);
          
          oled.setCursor((128 - oled.strWidth(line2)) / 2, 7);
          oled.print(line2);
        }
      }
      break;
    default:
      {
        const char* text = "Brak aktywnej akcji";
        oled.setCursor((128 - oled.strWidth(text)) / 2, 6);
        oled.print(text);
      }
      break;
  }
}

void parameter_screen(){
  const char* text1 = "Ustawienia";
  int x1_position = (128 - oled.strWidth(text1)) / 2;
  oled.setCursor(x1_position, 0);
  oled.print(text1);

  oled.setCursor(0, 1);
  oled.print("---------------------");

  oled.setCursor(0, 3);   //oblsuga potencjometru
  oled.print("Nowa Temp:");

  oled.setFont(lcdnums12x16);
  oled.setCursor(40, 5); //wyswietlenie wiekszej liczby
  oled.print(potentiometer_value);
  oled.print(" ");
  oled.setFont(System5x7);

  oled.print(" ");
  oled.write(0xF7);
  oled.print("C   ");

  const char* text3 = "[SET] Akceptuj";
  int x3 = (128 - oled.strWidth(text3)) / 2;
  oled.setCursor(x3, 7);
  oled.print(text3);
}

void choose_screen(int screen){
  switch(screen){
    case -1:
      main_screen(temp_current);
      break;
    case 1:
      parameter_screen();
      break;
  }
}

void reset_flag(){
  reset_request = true;
}

void perform_reset(){
  digitalWrite(SSR_PIN, LOW);
  oled.clear();
  active_screen = -1;
  action = -1;
  temp_set_defined = false;
  last_change_button_state = HIGH; 
  last_set_button_state = HIGH;
  potentiometer_value = 0;
  last_pot_value = -1;
  if(PID_mode){
    myPID.SetMode(MANUAL); // Wyłaczenie PID
    Output = 0;
  }

  choose_screen(active_screen);
}

void control_hysteresis(){
  if(isnan(temp_current)){
    digitalWrite(SSR_PIN, LOW);
    return;
  }
  if(action == 1){
    if(temp_current < temp_set){
      digitalWrite(SSR_PIN, HIGH);
    }
    else{
      action = 2;
    }
  }
  if(action == 2){
    if(temp_current < temp_set){
      digitalWrite(SSR_PIN, HIGH);
    }
    else{
      digitalWrite(SSR_PIN, LOW);
    }
  }
  if(action == -1){
    digitalWrite(SSR_PIN, LOW);
  }
}

void control_PID(){
  if (isnan(temp_current) || action == -1) {
    digitalWrite(SSR_PIN, LOW);
    return;
  }

  Input = temp_current;
  Setpoint = temp_set;
  myPID.Compute();  // obliczanie sterowania PID z biblioteki

  unsigned long now = millis();

  if (now - windowStartTime > WindowSize) {
    windowStartTime += WindowSize;
  }

  if (Output > (now - windowStartTime)) {
    digitalWrite(SSR_PIN, HIGH);
  } else {
    digitalWrite(SSR_PIN, LOW);
  }
}

void setup() {
  oled_setup();
  pinMode(SCREEN_CHOICE_BUTTON, INPUT_PULLUP);
  pinMode(TEMP_SET_BUTTON, INPUT_PULLUP);
  pinMode(RESET_BUTTON, INPUT_PULLUP);
  pinMode(SSR_PIN, OUTPUT);
  digitalWrite(SSR_PIN, LOW);
  attachInterrupt(digitalPinToInterrupt(RESET_BUTTON), reset_flag, FALLING);

  //PID
  if(PID_mode) {
    windowStartTime = millis();
    myPID.SetOutputLimits(0, WindowSize);
    myPID.SetMode(AUTOMATIC);
  }

  if(draw_plot){
    Serial.begin(9600);
  }

  starting_screen();
}

void loop() {
  //sprawdzanire czy reset
  if (reset_request) {
    perform_reset();
    reset_request = false;
    delay(200);
  }

  //odczyt termopary
  if((millis() - previous_time) >= 500){
    previous_time = millis();
    temp_current = thermocouple.readCelsius();
    if(active_screen == -1) {
       main_screen(temp_current);
    }
    if(hysteresis_mode){
      control_hysteresis();
    }

    if(draw_plot){
      Serial.print("Cel:");
      if(temp_set_defined){
        Serial.print(temp_set);
      }
      else{
        Serial.print(0);
      }
      Serial.print(",");
      Serial.print("Obecna:");
      if(isnan(temp_current)){
        Serial.println(0);
      }
      else{
        Serial.println(temp_current);
      }
    }

  }

  if(!hysteresis_mode && PID_mode){
    control_PID();
  }

  //zmiana ekranu
  int currentReading = digitalRead(SCREEN_CHOICE_BUTTON);

  if (last_change_button_state == HIGH && currentReading == LOW) { // zabezpieczenie przed zmiana ekrtanow podczas przytrzymania
    
    active_screen = -active_screen;
    oled.clear();
    choose_screen(active_screen);
    last_pot_value = -1;
    delay(20); 
  }
  last_change_button_state = currentReading;

  if(active_screen == 1){
    long sum = 0;
    for(int i=0; i<64; i++) {
      sum += analogRead(POTENTIOMETER);
    }
    int current_raw = sum / 64;

    if (abs(current_raw - last_raw_reading) > 1 || last_raw_reading == 0) {
       
       potentiometer_value = map(current_raw, 0, 1023, 150, 250); //zakladamy zakres temperatur 150 - 250
       
       if (potentiometer_value != last_pot_value) {
           last_pot_value = potentiometer_value;
           parameter_screen();
       }
       
       last_raw_reading = current_raw;
    }

    //przycisk SET
    int currentReading2 = digitalRead(TEMP_SET_BUTTON);

    if(last_set_button_state == HIGH && currentReading2 == LOW){
      temp_set = (double)potentiometer_value;
      temp_set_defined = true;
      active_screen = -1;
      action = 1;

      if(PID_mode){
        myPID.SetMode(MANUAL);
        Output = 0; // reset PID
        myPID.SetMode(AUTOMATIC);
      }

      oled.clear();
      choose_screen(active_screen);
      delay(20);
    }
    last_set_button_state = currentReading2;
  }
}