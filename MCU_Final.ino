#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <FirebaseESP32.h>
#include <ArduinoJson.h>

#define FIREBASE_HOST "cycle-parking-system-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "Rz08OAaI2g1WNoszbrHobCk7ueHqef32byun95T1"

FirebaseData firebaseData;
FirebaseESP32 firebase;

Servo servo1;
Servo servo2;
#define Servo_0 14 // Pin for Slot 0 LED
#define Servo_1 27 // Pin for Slot 1 LED

#define LED_0 26
#define LED_1 25

#define buzzer 12 
#define button1 33
#define button2 32


struct AuthorizedUser {
  String uid;
  String name;
  String regNumber;
};

AuthorizedUser authorizedUsers[] = {
 {"AA65FF30", "ALDRIN", "URK22EC1019"},
 {"7535ABAC", "THIMOTHY", "URK22EC1018"},
 {"2D889A67", "BENNY", "URK22EC1017"},
  //{"DD65FF30", "Sangeeth", "URK22EC1017"}
  // Add more authorized users as needed
};

bool slot_state[2] = {false, false}; // Initial slot state for 2 slots
String uid_slot[2] = {"nil0", "nil1"};
bool slotFull = false;

LiquidCrystal_I2C lcd(0x27, 16, 2); // Initialize the I2C LCD display with its address

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Wire.begin();
  lcd.begin(16, 2);   // Initialize the LCD display
  lcd.backlight();
  
  servo1.attach(Servo_0);
  //servo2.attach(Servo_1);
  //pinMode(Servo_0, OUTPUT); // Set LED pin as output
  pinMode(Servo_1, OUTPUT);
  pinMode(LED_0, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);

  WiFi.begin("Virus#404", "password1");
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");

  firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  // Update the slot status in the Firebase database
  firebase.setBool(firebaseData, "/CycleParking/slot1", false);
  firebase.setBool(firebaseData, "/CycleParking/slot2", false);
  
  lcd.setCursor(1, 0);
  lcd.print("<Device Ready>");
  lcd.setCursor(4, 1);
  lcd.print("WELCOME!");  
  delay(1000);
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("#Scan Your ID#");
  Serial.println("Ready to read RFID tags...");
}  

bool isAuthorizedUID(String uid) {
  for (int i = 0; i < sizeof(authorizedUsers) / sizeof(authorizedUsers[0]); i++) {
    if (uid == authorizedUsers[i].uid) {
      return true; // UID is authorized
    }
  }
  return false; // UID is not authorized
}

void openLed(int ledPin) {
  digitalWrite(ledPin, HIGH); 
}

void closeLed(int ledPin) {
  digitalWrite(ledPin, LOW);
}

void sound(){
  digitalWrite(buzzer, HIGH);
  delay(500);
  digitalWrite(buzzer, LOW);
}

int BookSlot(int slot) {
  switch (slot) {
    int state;
    case 0:
      state = 0;
      //openLed(2);
      servo1.write(0);
      digitalWrite(LED_0, HIGH);
      for(int i=0; i<20; i++){
        if(digitalRead(button1) == LOW){
          delay(1000);
          state = 1; 
          break;
        }
        delay(500);
      }
      //closeLed(2);
      servo1.write(180);
      digitalWrite(LED_0, LOW);
      sound();
      return state;
      break;

    case 1:
      state = 0;
      openLed(27);
      //servo2.write(0);
      digitalWrite(LED_1, HIGH);
      for(int i=0; i<20; i++){
        if(digitalRead(button2) == LOW){
          delay(1000);
          state = 1;          
          break;
        }
        delay(500);
      }
      closeLed(27);
      //servo2.write(180);
      digitalWrite(LED_1, LOW);      
      sound();
      return state;
      break;

    default:
      break;
  }
 }

int WithdrawSlot(int slot) {
  switch (slot) {
    int state;    
    case 0:
      state = 1;
      openLed(2);
      servo1.write(0);
      digitalWrite(LED_0, HIGH);
      for(int i=0; i<20; i++){
        if(digitalRead(button1) == HIGH){
          delay(1000);
          state = 0;
          break;
        }
        delay(500);
      }
      closeLed(2);
      servo1.write(180);
      digitalWrite(LED_0, LOW);
      sound();
      return state;
      break;

    case 1:
      state = 1;
      openLed(27);
      //servo2.write(0);
      digitalWrite(LED_1, HIGH);
      for(int i=0; i<20; i++){
        if(digitalRead(button2) == HIGH){
          delay(1000);
          state = 0;
          break;
        }
        delay(500);
      }
      closeLed(27);
      //servo2.write(180);
      digitalWrite(LED_1, LOW);
      sound();
      return state;
      break;

    default:
      break;
  }
}

void scanID() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("#Scan Your ID#");
}

int P(String uid) {
  for (int i = 0; i < sizeof(authorizedUsers) / sizeof(authorizedUsers[0]); i++) {
    if (uid == authorizedUsers[i].uid) {
      return i; // UID is authorized
    }
  }
}

void loop() {
  // put your main code here, to run repeatedly:
String Data = "";
bool dataInput = false;

if(WiFi.status() != WL_CONNECTED){
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("[!Server'Error!]");

  WiFi.begin("Virus#404", "password1");
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
  delay(500);
  Serial.print(".");
  }  
  Serial.println();
  Serial.println("WiFi connected");

  firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  scanID();
}

if (Firebase.getString(firebaseData, "/CycleParking/input")) {
  if (firebaseData.dataType() == "string") {
    String temp = firebaseData.stringData();
    temp.trim();  // Remove leading and trailing whitespaces
    
    if (temp.length() > 7) {  // Check if the string length is greater than 8
      Data = temp.substring(2, temp.length() - 2);
      dataInput = true;
    }
  }
}


if (!(Serial.available() > 0) && slotFull == true) {
    scanID();
    lcd.setCursor(0, 1);
    lcd.print("            ");
    delay(500);
    lcd.setCursor(2, 1);
    lcd.print("[SlotsFuLL!]");
    delay(1000);  
}
if ((Serial.available() > 0) || (dataInput)){
    String uid = "";
    if(!dataInput){
    uid = Serial.readString();
    uid.trim();
    lcd.clear();
    lcd.setCursor(1, 0);  
    lcd.print("...ScanninG...");
    delay(500);
    }
    else{
    uid = Data;
    lcd.clear();
    lcd.setCursor(1, 0);  
    lcd.print("UID: " + uid);
    delay(500);
    }

    uid.toUpperCase();
    Serial.print("UID: ");
    Serial.println(uid);

    if (!isAuthorizedUID(uid)) {
      lcd.clear();
      lcd.setCursor(1, 0);
      lcd.print("!Unknown UseR!");
      lcd.setCursor(2, 1);
      lcd.print("AccessDenieD");
      delay(2000);

      scanID();
      
      Serial.println("Unauthorized access!\n");
      return;
    }
        
    for (int i = 0; i < 2; i++) {

      if (slot_state[i] == true && uid_slot[i] == uid) {
        slot_state[i] = false;
        uid_slot[i] = "nil";
        uid_slot[i].concat(String(i));

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(authorizedUsers[P(uid)].name);
        lcd.setCursor(0, 1);
        lcd.print(authorizedUsers[P(uid)].regNumber);
        delay(1000);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(authorizedUsers[P(uid)].name);
        lcd.setCursor(0, 1);
        lcd.print("Slot: ");
        lcd.print(i+1);
        delay(1000);
        
        if(WithdrawSlot(i) == 0){
        slotFull = false;
        
        // Update the slot status in the Firebase database
        firebase.setBool(firebaseData, "/CycleParking/slot" + String(i+1), slot_state[i]);

        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("*Cycle UnParkeD*");
        lcd.setCursor(2,1);
        lcd.print("[ Slot: ");
        lcd.print(i+1);
        lcd.print(" ]");
        delay(2000);
        }
        else{
        slot_state[i] = true;
        uid_slot[i] = uid;
        
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Unparking FaileD");
        lcd.setCursor(3,1);
        lcd.print("Try AgaiN!");
        delay(2000);}
        
        scanID();
        break;
      }
      if (!(slot_state[0] && slot_state[1])) {
       if (slot_state[i] == false && uid != uid_slot[0] && uid != uid_slot[1]) {
          uid_slot[i] = uid;
          slot_state[i] = true;

          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(authorizedUsers[P(uid)].name);
          lcd.setCursor(0, 1);
          lcd.print(authorizedUsers[P(uid)].regNumber);
          delay(2000);

          lcd.clear();
          lcd.setCursor(2, 0);
          lcd.print("Alloted SloT");
          lcd.setCursor(4, 1);
          lcd.print("Slot:  ");
          lcd.print(i+1);
          delay(1000);
              
          if(BookSlot(i) == 1){
             
            // Update the slot status in the Firebase database
            firebase.setBool(firebaseData, "/CycleParking/slot" + String(i+1), slot_state[i]);

            lcd.clear();
            lcd.setCursor(1,0);
            lcd.print("*Cycle ParkeD*");
            lcd.setCursor(3,1);
            lcd.print("[Slot:  ");
            lcd.print(i+1);
            lcd.print("]");
            delay(2000);
            scanID();
            if(slot_state[0] && slot_state[1]){
            scanID();
            slotFull = true;          
           }
           else{
            scanID();
            slotFull = false;
            }
          }
          else{
            slot_state[i] = false;
            uid_slot[i] = "nil";
            uid_slot[i].concat(String(i));

            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("*Parking FaileD*");
            lcd.setCursor(3,1);
            lcd.print("Try AgaiN!");
            delay(2000);
            scanID();
            break;
          }        
        }
      }
      else if (slot_state[0] && slot_state[1] && i == 1) {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("No Slots");
        lcd.setCursor(0, 1);
        lcd.print("Available!");
        delay(2000);
      }
    }
  }
}