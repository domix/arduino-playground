#include "LedControl.h"

LedControl lc=LedControl(11,9,10,1);
int X;
int Y;
int currentFila;
int currentColumna;

int buzzer = 12;
int tone_ok = 300;
int tone_bad = 100;
int PULSADOR = 8;
int SW;
int buttonState;
int lastButtonState = LOW;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50; 

int sound = 1;

bool silence = false;


void setup() {
  Serial.begin(9600);
  lc.shutdown(0,false);
  lc.setIntensity(0,4);
  lc.clearDisplay(0);
  currentFila = 0;
  currentColumna = 7;
  pinMode(buzzer, OUTPUT);
  pinMode(PULSADOR, INPUT_PULLUP);
  
  digitalWrite(PULSADOR, HIGH);
}

void loop(){
  X = analogRead(A0);
  Y = analogRead(A1);
  SW = digitalRead(PULSADOR);

  if(SW == 0) {
    silence = !silence;
  }

  if (X >= 0 && X < 480){          // si X esta en la zona izquierda
    if(currentColumna > 0) {
      sonido(tone_ok);
      currentColumna++;
     } else {
      sonido(tone_bad);
     }
  }

  if (X > 520 && X <= 1023){          // si X esta en la zona derecha
    if(currentColumna < 7) {
      sonido(tone_ok);
      currentColumna++;
     } else {
      sonido(tone_bad);
     }
  }
  
  if (Y >= 0 && Y < 480){         // si Y esta en la zona de abajo
    if(currentFila < 7) {
      sonido(tone_ok);
      currentFila++;
     } else {
      sonido(tone_bad);
     }
  }

  if (Y > 520 && Y <= 1023){          // si Y esta en la zona de arriba
    if(currentFila > 0) {
      sonido(tone_ok);
      currentFila--;
     } else {
      sonido(tone_bad);
     }
  }

  lc.setLed(0,currentFila,currentColumna,true);
  
  delay(100);
  lc.clearDisplay(0);
  noTone(buzzer);
  
}

void sonido(int tono) {
  if(silence == 0) {
    tone(buzzer, tono); 
  }
}
