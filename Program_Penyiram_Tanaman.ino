#include <Keypad.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <EEPROM.h>


// Inisialisasi Pin Sensor dan LED
int pinPompa = 12;
int pinSensor = A0;

RTC_DS1307 RTC;
bool ledTriggeredA = false;
bool ledTriggeredB = false;

// Inisialisasi Keypad
const uint8_t BARIS = 4;
const uint8_t KOLOM = 4;

uint8_t pinBaris[BARIS] = {9,8,7,6};
uint8_t pinKolom[KOLOM] = {5,4,3,2};

char keys[BARIS][KOLOM] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};


// Inisialisasi LCD dengan alamat 0x27, 16 kolom, 2 baris
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Keypad
Keypad keypad = Keypad(makeKeymap(keys), pinBaris, pinKolom, BARIS, KOLOM);

// Variabel untuk menyimpan Input Nilai Sensor
String inputBatasSensor = "";


float setSensor = 50.0,tempSensor;
bool statusSetSensor = false;
int input_step_sensor = 0;

// Status Flag Lampu
bool statusLed = false;

// Menyimpan Nilai Last Kelembapan
float lastKelembapan = -1;
float lastSetSensor = -1;

// VARIABEL UNTUK MENYIMPAN WAKTU SENSOR
String t1_jamInput = "";
String t1_menitInput = "";
String t2_jamInput = "";
String t2_menitInput = "";

// Variabel Untuk Mengedit Jam
String setJam = "";
String setMenit = "";
int input_step_time = 0;
bool setNow = false;
int setJamNow = 0;
int setMenitNow = 0;

int setJamA = 0, tempJamA;
int setMenitA = 0, tempMenitA;
int setJamB = 0, tempJamB;
int setMenitB = 0, tempMenitB;

bool settingTimer = false;
int inputStep = 0; // 0: jam, 1: menit, 2: jam,3: menit

bool modeSensorAktif = true;
bool modeTimerAktif = false;

// Inisialisasi Pin Buzzer
const int buzzerpin = 10;

void setup()
{
  digitalWrite(pinPompa, HIGH);// Pompa selalu mati kdulu ketika arduino dihidupkan
  pinMode(pinPompa, OUTPUT);
  pinMode(pinSensor, INPUT);
  
  lcd.init();  // Gunakan begin() di Tinkercad
  lcd.backlight();  // Nyalakan lampu latar
  
  RTC.begin();

  
  bacaDariEEPROM(); //Membaca nilai terakhir

  // Mengatur waktu RTC ke waktu yang diinginkan (hanya untuk testing)
  // RTC.adjust(DateTime(2025, 4, 3, 9, 38, 0));
}

void loop(){
  char key = keypad.getKey();
  int humi = analogRead(pinSensor);
  float kelembapan = (100 - ((humi/1023.00) * 100));
  
  if(key){
      tone(buzzerpin, 900);
      delay(10);
      settingSensor(key);
      setTimer(key);
  }

  if (!key){
    noTone(buzzerpin);
  }

  if(modeSensorAktif && !statusSetSensor){
     setModeSensor(setSensor, kelembapan, key);
  }
  if(modeTimerAktif && !settingTimer){
    timeNow(key);
  }

  if (key == '*' and !settingTimer and !setNow and !statusSetSensor){
    lcd.clear();
    DateTime now = RTC.now();
    lcd.setCursor(0, 0);
    lcd.print("Time Now: ");
    lcd.setCursor(0, 1);
    lcd.print((now.hour() < 10 ? "0" : "") + String(now.hour()) + ":");
    lcd.print((now.minute() < 10 ? "0" : "") + String(now.minute()));
    noTone(buzzerpin);
    delay(2500);
  }
}


void settingSensor(char key){
	if (key == 'B'){
      modeSensorAktif = true;
      modeTimerAktif = false;
      statusSetSensor = true;
      simpanKeEEPROM();
      input_step_sensor = 0;
      inputBatasSensor = "";
      digitalWrite(pinPompa, HIGH);
      

      tempSensor = setSensor;
      
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Set Bts Sensor:");
      lcd.setCursor(0, 1);
      lcd.print("00");
      return;
    }
    if (key == 'C' && statusSetSensor) {
    statusSetSensor = false;
    setSensor = tempSensor;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Batal!");
    delay(1000);
    digitalWrite(pinPompa, LOW);  //TRIGGER BUAT POMPA
    lcd.clear();
    return;
    }
    
    if (statusSetSensor) {
      if (key >= '0' && key <= '9') {
        if (input_step_sensor == 0) {
          inputBatasSensor += key;
          float sensorTemp = inputBatasSensor.toFloat();
          if (sensorTemp > 100.0) sensorTemp = 100.0;
          setSensor = sensorTemp;
          lcd.setCursor(0, 1);
          lcd.print(setSensor);
        } 
      } else if (key == '#') {
        input_step_sensor++;
        if(input_step_sensor > 0) {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Berhasil!!");
          lcd.setCursor(1,1);
          lcd.print("Tunggu...");
          delay(1000);
          statusSetSensor = false;
          digitalWrite(pinPompa, LOW);  //TRIGGER BUAT POMPA
          simpanKeEEPROM();
          lcd.clear();
        } 
      }
      return;
  	}
}

void setModeSensor(float setSensor, float kelembapan, char key) {
  // Update LCD hanya jika nilai berubah
  if (kelembapan != lastKelembapan || setSensor != lastSetSensor) {
    lcd.setCursor(0, 0);
    lcd.print("md Sensor:" + String(kelembapan, 1) + "%");
    lcd.setCursor(0, 1);
    lcd.print("Bts:" + String(setSensor, 1) + "%");

    lastKelembapan = kelembapan;
    lastSetSensor = setSensor;
  }

  // Kontrol LED seperti biasa
  if (kelembapan < setSensor && !statusLed && key != 'B') {
    digitalWrite(pinPompa, LOW);
    statusLed = true;
  } else if (kelembapan > setSensor) {
    statusLed = false;
    digitalWrite(pinPompa, HIGH);
  }
}


void setTimer(char key){
  if (key == 'A') {
      modeSensorAktif = false;
      modeTimerAktif = true;
      settingTimer = true;
      simpanKeEEPROM();
      inputStep = 0;
      digitalWrite(pinPompa, HIGH);  
      
      t1_jamInput = "";
      t1_menitInput = "";
      t2_jamInput = "";
      t2_menitInput = "";

      tempJamA = setJamA;
      tempMenitA = setMenitA;
      tempJamB = setJamB;
      tempMenitB = setMenitB;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Set T1:");
      lcd.setCursor(8, 0);
      lcd.print("00");
      lcd.setCursor(0, 1);
      lcd.print("Set T2:");
      return; // Mencegah pemanggilan timeNow() setelah tombol 'A' ditekan
    }
  if (key == 'C' && settingTimer) {
    settingTimer = false;
    setJamA = tempJamA;
    setMenitA = tempMenitA;
    setJamB = tempJamB;
    setMenitB = tempMenitB;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Batal!");
    delay(1000);
    lcd.clear();
    return;
    }
    if (settingTimer) {
      if (key >= '0' && key <= '9') {
        if (inputStep == 0) {
          t1_jamInput += key;
          int jamTemp = t1_jamInput.toInt();
          if (jamTemp > 23) jamTemp = 23;
          setJamA = jamTemp;
          lcd.setCursor(8, 0);
          lcd.print((setJamA < 10 ? "0" : "") + String(setJamA) + ":");
        
        } else if (inputStep == 1) {
          t1_menitInput += key;
          int menitTemp = t1_menitInput.toInt();
          if (menitTemp > 59) menitTemp = 59;
          setMenitA = menitTemp;
          lcd.setCursor(11, 0);
          lcd.print((setMenitA < 10 ? "0" : "") + String(setMenitA));

        } else if (inputStep == 2) {
          t2_jamInput += key;
          int jamTemp = t2_jamInput.toInt();
          if (jamTemp > 23) jamTemp = 23;
          setJamB = jamTemp;
          lcd.setCursor(8, 1);
          lcd.print((setJamB < 10 ? "0" : "") + String(setJamB) + ":");
          
        } else if (inputStep == 3) {
          t2_menitInput += key;
          int menitTemp = t2_menitInput.toInt();
          if (menitTemp > 59) menitTemp = 59;
          setMenitB = menitTemp;
          lcd.setCursor(11, 1);
          lcd.print((setMenitB < 10 ? "0" : "") + String(setMenitB));
        }
      } else if (key == '#') {
        inputStep++;
        if (inputStep > 3) {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("Berhasil!!");
          lcd.setCursor(1,1);
          lcd.print("Tunggu...");
          delay(1000);
          settingTimer = false;
          simpanKeEEPROM();
          lcd.clear();
        } else if(inputStep == 0) {
          lcd.setCursor(8, 0);
          lcd.print("00");
        } else if(inputStep == 1) {
          lcd.setCursor(11, 0);
          lcd.print("00");
        } else if(inputStep == 2) {
          lcd.setCursor(8, 1);
          lcd.print("00");
        }else if(inputStep == 3) {
          lcd.setCursor(11, 1);
          lcd.print("00");
        }
      }
      return; // Mencegah pemanggilan timeNow() saat setting timer
    }
}

void timeNow(char key) {
  if (key == 'D') {  // Periksa jika tombol 'D' ditekan
    setNow = true;
    input_step_time = 0;  // Reset langkah input
    setJam = "";
    setMenit = "";
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Set TimeNow:");
    lcd.setCursor(0, 1);
    lcd.print("00:00");
    return;
  }

  if (setNow) {
    if (key >= '0' && key <= '9') {  
      if (input_step_time == 0) {  // Input jam
        setJam += key;
        int nowJam = setJam.toInt();
        if (nowJam > 23) nowJam = 23;  // Batas 23 jam
        setJamNow = nowJam;
        lcd.setCursor(0, 1);
        lcd.print((setJamNow < 10 ? "0" : "") + String(setJamNow) + ":");
      } 
      else if (input_step_time == 1) {  // Input menit
        setMenit += key;
        int nowMenit = setMenit.toInt();
        if (nowMenit > 59) nowMenit = 59;  // Batas 59 menit
        setMenitNow = nowMenit;
        lcd.setCursor(3, 1);
        lcd.print((setMenitNow < 10 ? "0" : "") + String(setMenitNow));
      }
    } 
    
    else if (key == '#') {  // Tombol '#' untuk lanjut input
      input_step_time++;
      
      if (input_step_time == 1) {  
        lcd.setCursor(3, 1);
        lcd.print("00");
      } 
      else if (input_step_time > 1) {  
        RTC.adjust(DateTime(2025, 4, 3, setJamNow, setMenitNow, 0));  // Set RTC

        setNow = false;  // Matikan mode pengaturan waktu
        setJam = "";  // Reset nilai input
        setMenit = "";  
        
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Waktu Tersimpan!");
        delay(1000);
        lcd.clear();
      }
    }
    return;  // Hindari pemrosesan lebih lanjut saat sedang setting waktu
  }

    if(!setNow){
    DateTime now = RTC.now();
    lcd.setCursor(0, 0);
    lcd.print("md Timer: ");
    lcd.print((now.hour() < 10 ? "0" : "") + String(now.hour()) + ":");
    lcd.print((now.minute() < 10 ? "0" : "") + String(now.minute()));
    
    lcd.setCursor(0, 1);
    lcd.print((setJamA < 10 ? "0" : "") + String(setJamA) + ":");
    lcd.print((setMenitA < 10 ? "0" : "") + String(setMenitA) + " | ");
    lcd.print((setJamB < 10 ? "0" : "") + String(setJamB) + ":");
    lcd.print((setMenitB < 10 ? "0" : "") + String(setMenitB));
    
    // Logic untuk LED Triggering
    if (now.hour() == setJamA && now.minute() == setMenitA && !ledTriggeredA) {
      digitalWrite(pinPompa, LOW);
      tone(buzzerpin,900);
      delay(5000);
      digitalWrite(pinPompa, HIGH);
      noTone(buzzerpin);
      ledTriggeredA = true;
    } else if (now.hour() != setJamA || now.minute() != setMenitA) {
      ledTriggeredA = false;
    }

    if (now.hour() == setJamB && now.minute() == setMenitB && !ledTriggeredB) {
      digitalWrite(pinPompa, LOW);
      tone(buzzerpin,900);
      delay(5000);
      digitalWrite(pinPompa, HIGH);
      noTone(buzzerpin);
      ledTriggeredB = true;
    } else if (now.hour() != setJamB || now.minute() != setMenitB) {
      ledTriggeredB = false;
    }
  }
}

void simpanKeEEPROM() {
  EEPROM.write(0, modeSensorAktif ? 0 : 1); // mode
  EEPROM.put(1, setSensor); // float
  EEPROM.write(5, setJamA);
  EEPROM.write(6, setMenitA);
  EEPROM.write(7, setJamB);
  EEPROM.write(8, setMenitB);
}

void bacaDariEEPROM() {
  byte lastMode = EEPROM.read(0);
  modeSensorAktif = (lastMode == 0);
  modeTimerAktif = (lastMode == 1);

  EEPROM.get(1, setSensor); // Baca float mulai dari address 1
  setJamA = EEPROM.read(5);
  setMenitA = EEPROM.read(6);
  setJamB = EEPROM.read(7);
  setMenitB = EEPROM.read(8);
}
