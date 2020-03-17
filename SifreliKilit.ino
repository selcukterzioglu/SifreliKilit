/*
 Name:    SifreliKilit.ino
 Created: 12/28/2016 8:54:30 
 Author:  Selçuk TERZİOĞLU @strz35
*/

#include <EEPROM.h>
#include <TimerOne.h>

#define TUSA_BASILMADI  0
#define SIFRE_OK        1
#define SIFRE_HATA      2
#define SIFRE_DEGIS     3
#define TUSA_BASILDI    4
#define ZAMAN_ASIMI     5

#define TUS_YOK       'X'

#define TUS_TIMEOUT   10

#define BUZZER        9
#define LED_TAMAM     10
#define LED_HATA      11
#define LED_DEGISTIR  12
#define ROLE_PIN      13

#define STATUS_ADR    0
#define SIFRE_ADR     1
#define SIFRE_MAKS    8

#define ON            1
#define OFF           0
#define ROLE(val)     digitalWrite(ROLE_PIN,val)

unsigned int timerSay = 0;
boolean timerAktif = false;

char _sifre[SIFRE_MAKS],sifre[SIFRE_MAKS];

const char VARSAYILAN_SIFRE[8] = { '1', '2', '3', '4', TUS_YOK, TUS_YOK, TUS_YOK, TUS_YOK };
const char SIFRE_DEGISTIR_CHAR[2] = { '0','*' };

const char TUS_TABLOSU[4][3] = { {'1', '2', '3'},{ '4', '5', '6' },{ '7', '8', '9' },{ '*', '0', '#' } };

int satirlar[4] = { 2,3,4,5 };
int sutunlar[3] = { 6,7,8 };

void setup() {

  char ilkCalisma = EEPROM.read(STATUS_ADR);
  if (ilkCalisma != '1') {
    for (int i = 0; i < 8; i++) {
      EEPROM.write(SIFRE_ADR + i, VARSAYILAN_SIFRE[i]);
      _sifre[i] = VARSAYILAN_SIFRE[i];
    }
    EEPROM.write(STATUS_ADR, '1');
    sesliIkaz(SIFRE_OK);
  }
  else {
    for (int i = 0; i < 8; i++) {
      _sifre[i] = EEPROM.read(SIFRE_ADR + i);
    }
  }

  pinMode(LED_DEGISTIR, OUTPUT);
  pinMode(LED_HATA, OUTPUT);
  pinMode(LED_TAMAM, OUTPUT);
  pinMode(ROLE_PIN, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  for (int i = 0; i < 4; i++)
    pinMode(satirlar[i], OUTPUT);
  for (int i = 0; i < 3; i++)
    pinMode(sutunlar[i], INPUT_PULLUP);

  Timer1.initialize(500000);
  Timer1.attachInterrupt(TimerKesmesi, 500000);
  timerAktif = 0;
  timerSay = 0;
  sifreTemizle(sifre);
}

void loop() {
  int durum = tusDizisiniOku(sifre);
  if (durum != TUSA_BASILMADI) {
    switch (durum)
    {
    case SIFRE_OK:
      if (sifreKontrolEt(sifre, _sifre)) {
        ROLE(ON);
        sesliIkaz(SIFRE_OK);
        ROLE(OFF);
      }
      else {
        sesliIkaz(SIFRE_HATA);
      }
      sifreTemizle(sifre);
      break;
    case SIFRE_HATA:
      sesliIkaz(SIFRE_HATA);
      sifreTemizle(sifre);
      break;
    case SIFRE_DEGIS:
      digitalWrite(LED_DEGISTIR, HIGH);
      if (sifreyiDegistir())
        sesliIkaz(SIFRE_OK);
      else {
        sesliIkaz(SIFRE_HATA);
        timerAktif = 0;
        timerSay = 0;
      }
      sifreTemizle(sifre);
      digitalWrite(LED_DEGISTIR, LOW);
      break;
    case ZAMAN_ASIMI:
      sesliIkaz(SIFRE_HATA);
      sifreTemizle(sifre);
      break;
    default:
      break;
    }
  }
  delay(10);
}

void sifreTemizle(char *sfr) {
  for (int i = 0; i < SIFRE_MAKS; i++)
    sfr[i] = TUS_YOK;
}

boolean sifreyiDegistir() {
  char geciciSifre[SIFRE_MAKS], _geciciSifre[SIFRE_MAKS];
  sifreTemizle(sifre);
  sifreTemizle(geciciSifre);
  sifreTemizle(_geciciSifre);
  timerAktif = true;
  timerSay = 0; 
  int durum = TUSA_BASILMADI;
  while (durum == TUSA_BASILMADI) {
    durum = tusDizisiniOku(sifre);
    if (timerSay > ZAMAN_ASIMI || durum == ZAMAN_ASIMI || durum == SIFRE_DEGIS)
      return false;
  }
  timerSay = 0;
  timerAktif = false;
  if (sifreKontrolEt(sifre, _sifre)) {    
    sesliIkaz(SIFRE_DEGIS);
    timerAktif = true;
    durum = TUSA_BASILMADI;
    while (durum == TUSA_BASILMADI) {
      durum = tusDizisiniOku(geciciSifre);
      if (timerSay > ZAMAN_ASIMI || durum == ZAMAN_ASIMI || durum == SIFRE_DEGIS)
        return false;
    }
    timerAktif = false;
    sesliIkaz(SIFRE_DEGIS);

    timerSay = 0;
    timerAktif = true;
    durum = TUSA_BASILMADI;
    while (durum == TUSA_BASILMADI) {
      durum = tusDizisiniOku(_geciciSifre);
      if (timerSay > ZAMAN_ASIMI || durum == ZAMAN_ASIMI || durum == SIFRE_DEGIS)
        return false;
    }
    timerAktif = 0;
    timerSay = 0;
    if (sifreKontrolEt(geciciSifre, _geciciSifre)) {
      for (int i = 0; i < SIFRE_MAKS; i++) {
        EEPROM.write(SIFRE_ADR + i, geciciSifre[i]);
        _sifre[i] = geciciSifre[i];
      }
      return true;
    }
    else return false;
  }
  else return false;    
}

uint8_t tusDizisiniOku(char* tuslar) {  
  char c = tusTakiminiOku();
  if (c == TUS_YOK)
    return TUSA_BASILMADI;
  tuslar[0] = c;
  int index = 1;
  timerAktif = true;
  timerSay = 0;
  while (index < SIFRE_MAKS) {
    c = tusTakiminiOku();
    if (c != TUS_YOK) {
      timerSay = 0;
      if (c == '#') {     
        return SIFRE_OK;
      }
      else {
        tuslar[index] = c;
        index++;
      }

    }
    if (timerSay > TUS_TIMEOUT) {
      timerAktif = 0;
      timerSay = 0;
      return ZAMAN_ASIMI;
    }
    if (index == 2) {
      if (tuslar[0] == SIFRE_DEGISTIR_CHAR[0] && tuslar[1] == SIFRE_DEGISTIR_CHAR[1]) {
        timerSay = 0;
        timerAktif = 0;
        return SIFRE_DEGIS;
      }
    }
  }
  timerAktif = 0;
  timerSay = 0; 
}


boolean sifreKontrolEt(char* pin, char* _pin) {
  boolean sifreTamam = true;
  for (int i = 0; i < SIFRE_MAKS; i++)
    if (pin[i] != _pin[i])
      sifreTamam = false;
  return sifreTamam;
}

void TimerKesmesi() {
  if (timerAktif)
    timerSay++;
}

char tusTakiminiOku() {
  for (int i = 0;i < 4;i++)
    digitalWrite(satirlar[i], HIGH);
  for (int i = 0;i < 4;i++) {
    if (i > 0)
      digitalWrite(satirlar[i - 1], HIGH);
    digitalWrite(satirlar[i], LOW);
    for (int j = 0;j < 3;j++) {
      if (digitalRead(sutunlar[j])==LOW) {
        if (TUS_TABLOSU[i][j] != '#')
          sesliIkaz(TUSA_BASILDI);        
        delay(200);
        return TUS_TABLOSU[i][j];
      }
    }
  }
  return TUS_YOK;
}

void sesliIkaz(int mod) {
  switch (mod)
  {
  case SIFRE_OK:
    digitalWrite(BUZZER, HIGH);
    digitalWrite(LED_TAMAM, HIGH);
    delay(1000);
    digitalWrite(BUZZER, LOW);
    digitalWrite(LED_TAMAM, LOW);
    break;
  case SIFRE_HATA:
    digitalWrite(LED_HATA, HIGH);
    for (int i = 0;i < 6;i++) {
      digitalWrite(BUZZER, HIGH);     
      delay(250);
      digitalWrite(BUZZER, LOW);      
      delay(250);
    }
    digitalWrite(LED_HATA, LOW);
    break;
  case SIFRE_DEGIS:
    for (int i = 0;i < 2;i++) {
      digitalWrite(BUZZER, HIGH);
      delay(150);
      digitalWrite(BUZZER, LOW);
      delay(150);
    }
    break;
  case TUSA_BASILDI:
    digitalWrite(LED_TAMAM, HIGH);
    digitalWrite(BUZZER, HIGH);
    delay(100);
    digitalWrite(BUZZER, LOW);
    digitalWrite(LED_TAMAM, LOW);
    break;
  }
}

