//---------------------------------------------------------------------------------------------------------------------------
#include <HardwareSerial.h>
#include <iostream>
#include <PZEM004Tv30.h>
#include <Preferences.h>
#include <Separador.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLE2902.h>
#include <FirebaseESP32.h>
#include <ESP32Ping.h>
//#include <analogWrite.h>
#include <ESP32Time.h>
#include <Arduino_JSON.h>
//Librerias para tiempo
#include "time.h"

#define PIN_RED    25 // 
#define PIN_GREEN  26 // 
#define PIN_BLUE   27 //

#if !defined(PZEM_RX_PIN) && !defined(PZEM_TX_PIN)
#define PZEM_RX_PIN 16
#define PZEM_TX_PIN 17
#endif

#if !defined(PZEM_SERIAL)
#define PZEM_SERIAL Serial2
#endif

#define NUM_PZEMS 3

//PZEM004Tv30 pzems[NUM_PZEMS];

PZEM004Tv30 pzems[NUM_PZEMS];

ESP32Time rtc;

// Timer variables
unsigned long lastTime = 0;
unsigned long timerDelay = 10000;

/* ***************************************************************
 * Uncomment USE_SOFTWARE_SERIAL in order to enable Softare serial
 *
 * Does not work for ESP32
 *****************************************************************/
//#define USE_SOFTWARE_SERIAL

#if defined(USE_SOFTWARE_SERIAL) && defined(ESP32)
    #error "Can not use SoftwareSerial with ESP32"
#elif defined(USE_SOFTWARE_SERIAL)

#include <SoftwareSerial.h>

SoftwareSerial pzemSWSerial(PZEM_RX_PIN, PZEM_TX_PIN);
#endif

#define contador_num 1
BLECharacteristic *pCharacteristic;
Preferences preferences;

int lapso = 0;
unsigned long tiempo_lecturas;

String firebase_doc;
//fase 1
float voltaje1;
float corriente1;
float current;
float poder1;
float podersct;
float energiasct;
float energia1;
float frecuencia1;
float factor1;
int readingId;

//fase 2
float voltaje2;
float corriente2;
float current2;
float poder2;
float energia2;
float frecuencia2;
float factor2;

//fase 3
float voltaje3;
float corriente3;
float current3;
float poder3;
float energia3;
float frecuencia3;
float factor3;

float energiaTOT;
float energiaTOT_anterior = 0;
float energiaTOT_actual;

/*float estadoActualV1 = 0;
float estadoActualI1 = 0;
float estadoActualW1 = 0;
float estadoActualFP1 = 0;
float estadoAnteriorV1 = 0;
float estadoAnteriorI1 = 0;
float estadoAnteriorW1 = 0;
float estadoAnteriorFP1 = 0;

float estadoActualV2 = 0;
float estadoActualI2 = 0;
float estadoActualW2 = 0;
float estadoActualFP2 = 0;
float estadoAnteriorV2 = 0;
float estadoAnteriorI2 = 0;
float estadoAnteriorW2 = 0;
float estadoAnteriorFP2 = 0;

float estadoActualV3 = 0;
float estadoActualI3 = 0;
float estadoActualW3 = 0;
float estadoActualFP3 = 0;
float estadoAnteriorV3 = 0;
float estadoAnteriorI3 = 0;
float estadoAnteriorW3 = 0;
float estadoAnteriorFP3 = 0;*/


String lugar; //deben dedde la app
String medidor;

String proyecto = "SmartMeter";
bool banderaWifi;
String time_str;
String day_str;
String hoy;
String id;
String ano;
String dia;
String hora;
String mes;
#define pulpzem 21
#define puldata 22
int botonpzem = 0;
int botondatos = 0;

String datosbt;
String dato3;
String dato4;
int contador_callback = 0;
String fecha;
int longitud;
int humidity;
int temperature;
int contador_reset_wifi = 0;
int contador_de_boton = 0; 
int contador_hard = 0;
//para configuracion bluetooth
bool configured;
Separador s;
bool deviceConnected = false;
float txValue = 0;
char credentials[12];
const char* red;
const char* contra; 
String Red;
String password; // = "12345678";
String pass;
String datos;
String RED;
String CONTRA;
String timestamp;
String dataMessage;
String nomid;
bool bandera_confi = false;

// Variables de cnfoguracion de envio de notificacion diaria
String hora_lec; //= "20:00";
int dia_anterior = 0;
bool nuevo_dia = false;

// NTP server to request epoch time
const char* ntpServer = "co.pool.ntp.org";
const long  gmtOffset_sec = -18000;
const int daylightOffset_sec = 0;
// Variable to save current epoch time
unsigned long epochTime; 

#define SERVICE_UUID           "ffe0" // UART service UUID a5f81d42-f76e-11ea-adc1-0242ac120002
#define CHARACTERISTIC_UUID_RX "ffe1"
#define CHARACTERISTIC_UUID_TX "ffe2"

//Offset para el diferente uso de sensores---
//Sensor SCT-003
//calculo real(multimetro)/calculo aproximado (medidor)
float sct_offset1 = 1.78;
float sct_offset2 = 1.75;
float sct_offset3 = 1.74;

//for checking internet conection
//const IPAddress remote_ip(192, 168, 0, 1);
//-----------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------esto debe cambiar para cada usuario--------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------
String clave = "Trifasico";
//Base de Datos para cada cliente --> 
String FB_H;
String FB_A;
String FIREBASE_HOST = ""; // "https://testing-b8ea2-default-rtdb.firebaseio.com" //
String FIREBASE_Authorization_key = ""; //"sqnP5SJFhNswJW6FELpgxYrQ576KskOtky8jVtnc" //Y

//FirebaseFirestore firestore;

//Base de datos General -->
#define FIREBASE_HOST_GENERAL "https://volty-6503a-default-rtdb.firebaseio.com" // --> link de realtime database
#define FIREBASE_Authorization_key_GENERAL "1aC0P1kf5AG9JjB5e0bl0JL1gzH4ay0xjelspQeY"  //--> secreto de la base de datos en config

FirebaseData firebaseDataGEN;
FirebaseData firebaseData;
//FirebaseJson json;

void setColor(int R, int G, int B) {
  analogWrite(PIN_RED,   R);
  analogWrite(PIN_GREEN, G);
  analogWrite(PIN_BLUE,  B);
}

void upload(){
  Serial.println("Upload DATA");
  setColor(0, 255, 255);
  delay(500);
  setColor(0, 0, 0);
  delay(500);
  setColor(0, 255, 255);
  delay(500);
  setColor(0, 0, 0);
    delay(500);
  setColor(0, 255, 255);
  delay(500);
  setColor(0, 0, 0);
}

void failure(){
  Serial.println("failure");
  setColor(255, 0, 0);
  delay(1000);
  setColor(0, 0, 0);
  delay(500);
  setColor(255, 0, 0);
  delay(1000);
  setColor(0, 0, 0);
  delay(500);
  setColor(255, 0, 0);
  delay(500);
  setColor(0, 0, 0);
  reconect();
}

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();
      if (rxValue.length() > 0) {           
        Serial.println("**HOLA SI CAMBIE *");

  //YA NO RECIBE UN STRING SEPARADO POR COMAS... RECIBE 4 DATOS SEGUIDOS Y LOS VA GUARDANDO
        for (int i = 0; i < rxValue.length(); i++){
         // Serial.print(rxValue[i]);
         datosbt += rxValue[i];                                            
        }  
         if(contador_callback == 0 ){
           setColor(255,0,255);
            lugar = datosbt;
            String words = lugar; //reassign same string at the start of loop.          
            preferences.begin("credentials", false);
            preferences.putString("nombre",words);
            preferences.end();
            Serial.println("Nombre:" + lugar + "-");
            datosbt = " ";                          
         }      
         if(contador_callback == 1 ){
           setColor(0,0,0);
            medidor = datosbt;  
            String words = medidor; //reassign same string at the start of loop.
            Serial.println(words);
            char c;
            char no = ' '; //character I want removed.
            for (int i=0; i<words.length()-1;++i){
                c = words.charAt(i);
                if(c==no){
                    words.remove(i, 1);
                }
            }
            Serial.println(words);
           preferences.begin("credentials", false);
           preferences.putString("medidor",words);
           preferences.end(); 
           Serial.println("Medidor:" + medidor + "-");  
           datosbt = "";       
         }   
   if(contador_callback == 2 ){
           setColor(255,0,255);
           nomid = datosbt;  
           //nomid = "Soluciones Verticales SAS";      
            String words = nomid; //reassign same string at the start of loop.
            Serial.println(words);
            char c;
            char no = ' '; //character I want removed.
            for (int i=0; i<words.length()-1;++i){
                c = words.charAt(i);
                if(c==no){
                    words.remove(i, 1);
                }
            }
           preferences.begin("credentials", false);
           preferences.putString("ssid",nomid);
           preferences.end();
           Serial.println("SSID:" + nomid + "-");                                       
           datosbt = "";                
         }          
           if(contador_callback == 3 ){
             setColor(0,0,0);
             pass = datosbt;            
             String words = pass; //reassign same string at the start of loop.
             Serial.println(words);
             char c;
             char no = ' '; //character I want removed.   
              for (int i=0; i<words.length()-1;++i){
                c = words.charAt(i);
                if(c==no){
                  words.remove(i, 1);
                }
              }
             Serial.println(words);
             preferences.begin("credentials", false);
             preferences.putString("password",words);
             preferences.end(); 
             Serial.println("Password:" + words + "-");  
             datosbt = "";            
           }    
             if(contador_callback == 4 ){
                 setColor(0,0,0);
                 firebase_doc = datosbt;            
                 String words = firebase_doc; //reassign same string at the start of loop.
                Serial.println(words);
                char c;
                char no = ' '; //character I want removed.   
                for (int i=0; i<words.length()-1;++i){
                c = words.charAt(i);
                  if(c==no){
                   words.remove(i, 1);
                 }
                }
                Serial.println(words);
                 preferences.begin("credentials", false);
                 preferences.putString("firebase_doc",words);
                preferences.end(); 
               Serial.println("firebase_doc:" + words + "-");  
               datosbt = "";  
               bandera_confi = true;           
              contador_callback = 0;
                ESP.restart();     
               }                  

          contador_callback++;           
  } 
 }
};

void wifi_setup(String nomid, String pass){
  // Set device as a Wi-Fi Station
   WiFi.mode(WIFI_AP_STA);
     Serial.print("ssid: ");
  Serial.println(nomid);
  Serial.print("password: ");
  Serial.println(pass);
   red = nomid.c_str();
   contra = pass.c_str();
   WiFi.begin(red, contra);
   while (WiFi.status() != WL_CONNECTED) {
     delay(500);
     Serial.println("Setting as a Wi-Fi Station..");
     setColor(255,0,0);
     delay(1000);
     setColor(0,0,0);
    if (contador_reset_wifi < 5){
      contador_reset_wifi++;
      Serial.println(contador_reset_wifi);
    }else{
      ESP.restart();
      contador_reset_wifi = 0;
    }
  }
  Serial.print("WiFi connected with ip ");  
  Serial.println(WiFi.localIP());
 
  // color Azul
  Serial.print("Station IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());
  banderaWifi = false;
  configured = false;
  configTime(0, 0, "pool.ntp.org");  
  Firebase.begin(FIREBASE_HOST_GENERAL,FIREBASE_Authorization_key_GENERAL);                  
}

void save_data(String nomid, String pass){
  preferences.begin("credentials", false);
  preferences.putString("ssid", nomid); 
  preferences.putString("password", pass);
  Serial.println("Network Credentials Saved");
  preferences.end();
 // configured = true;
  delay(500);
  wifi_setup(nomid,pass);
}

void ble_setup(){
  // Create the BLE Device
  BLEDevice::init("SMARTMETER-SV"); // Give it a name

  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_TX,
                      BLECharacteristic::PROPERTY_NOTIFY
                    );
                      
  pCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
     CHARACTERISTIC_UUID_RX,
     BLECharacteristic::PROPERTY_WRITE
 );

    pCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();

   // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");
}

String printTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "Time Error";
  }
  //See http://www.cplusplus.com/reference/ctime/strftime/
  char output[80];
   // strftime(output, 80, "%H:%M:%S", &timeinfo);
   strftime(output, 80, "%d-%b-%y, %H:%M:%S", &timeinfo);
  time_str = String(output);

  return time_str;
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_RED,   OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);
  pinMode(PIN_BLUE,  OUTPUT);
  pinMode(pulpzem , INPUT);
  pinMode(puldata, INPUT);
  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)){
    rtc.setTimeStruct(timeinfo);
  }
  preferences.begin("credentials", false);
    //unsigned int counter = preferences.getUInt("counter", 0);
   lugar = preferences.getString("nombre", "");
   medidor = preferences.getString("medidor", "");
   nomid = preferences.getString("ssid", "");  
   pass = preferences.getString("password", ""); 
   String intervalo = preferences.getString("intervalo", "");
   String timestamp = preferences.getString("timestamp", "");
   firebase_doc = preferences.getString("firebase_doc", ""); //traido desde el BLe a la hora de confirugrar
   // preferences.putUInt("counter", contador);
    preferences.end();
    Serial.println("------------ DATOS GUARDADOS ------------");
    Serial.println(lugar);
    Serial.println(medidor);
    Serial.println(nomid); 
    Serial.println(pass);
    Serial.println("--"+firebase_doc+"--");
      if(touchRead(4) < 50){
        preferences.begin("credentials", false);
        preferences.clear();
        preferences.end();
        Serial.println("Datos eliminados por hardware");
        ESP.restart();
      } 
  //configuracion del PZEM con tres fases -------------------------
  #if defined(USE_SOFTWARE_SERIAL)
        // Initialize the PZEMs with Software Serial
        pzems[i] = PZEM004Tv30(pzemSWSerial, 0x10 + i);
        Serial.println("Software Serial");
  #elif defined(ESP32)
        // Initialize the PZEMs with Hardware Serial2 on RX/TX pins 16 and 17
      pzems[0] = PZEM004Tv30(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, 0x86);
      pzems[1] = PZEM004Tv30(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, 0x78);
      pzems[2] = PZEM004Tv30(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN, 0x76);
  #else
        // Initialize the PZEMs with Hardware Serial2 on the default pins

        /* Hardware Serial2 is only available on certain boards.
        *  For example the Arduino MEGA 2560
        */
        pzems[i] = PZEM004Tv30(PZEM_SERIAL, 0x10 + i);
        Serial.println("Hardware Serial");
  #endif
  //-----------------------------------------------------------------

   if (nomid == "" || pass == ""){   
    Serial.println("No values saved for ssid or password");
    configured = true;
    ble_setup();
  }else{
    setColor(0,255,0);
    delay(500);
    setColor(0,0,0);
    Serial.print("ssid: ");
    Serial.println(nomid);
    Serial.print("password: ");
    Serial.println(pass);
    wifi_setup(nomid,pass);
    delay(500);
    get_time();   
    get_date();
    FB_H = Firebase.getString(firebaseDataGEN,"/user_code/"+firebase_doc+"/fb_host/" );
    FIREBASE_HOST = firebaseDataGEN.stringData();
    FB_A = Firebase.getString(firebaseDataGEN,"/user_code/"+firebase_doc+"/api_key/"); 
    FIREBASE_Authorization_key  = firebaseDataGEN.stringData();  
    Serial.print("HOST: ");
    Serial.println(FIREBASE_HOST);
    Serial.print("API KEY: ");
    Serial.println(FIREBASE_Authorization_key);  
    if(FIREBASE_HOST == ""){
      Serial.println("No esta configurada ninguna BD");
    }else{
       delay(1000);
      Serial.println("Configurando BD PERSONAL");
       Firebase.begin(FIREBASE_HOST,FIREBASE_Authorization_key);
    }         
  } 
}

void reconect(){
  Serial.println("Reconectando wifi..........");
  const char* red_conect = nomid.c_str();
  const char* contra_conect = pass.c_str();
  WiFi.begin(red_conect, contra_conect);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Setting as a Wi-Fi Station..");
    if (contador_reset_wifi < 5){
      contador_reset_wifi++;
      Serial.println(contador_reset_wifi);
    }else{
        ESP.restart();
      contador_reset_wifi = 0;
    }
  }
}

void read_values(){
        voltaje1 = pzems[0].voltage();
        corriente1 = pzems[0].current();
        if(corriente1 < 1 ){
          current = pzems[0].current();
        }else{
          current = pzems[0].current()*sct_offset1;
        }
        factor1 = pzems[0].pf();
        //poder activo 1
        podersct = pzems[0].power()*sct_offset1;
        //poder aparente 1
        int aparente = poder1/factor1;
        energiasct = pzems[0].energy()*sct_offset1;
        frecuencia1 = pzems[0].frequency();
        // Check if the data is valid
        if(isnan(voltaje1)){
            Serial.println("Error reading voltage1");
        } else if (isnan(current)) {
            Serial.println("Error reading current1");
        } else if (isnan(podersct)) {
            Serial.println("Error reading power1");
        } else if (isnan(energiasct)) {
            Serial.println("Error reading energy1");
        } else if (isnan(frecuencia1)) {
            Serial.println("Error reading frequency1");
        } else if (isnan(factor1)) {
            Serial.println("Error reading power factor1");
        } else {
            // Print the values to the Serial console
            Serial.print("Voltage 1: ");      Serial.print(voltaje1);      Serial.println("V");
            Serial.print("Current 1: ");      Serial.print(current);      Serial.println("A");           
            Serial.print("Power 1: ");        Serial.print(podersct);        Serial.println("W");
            Serial.print("Energy 1: ");       Serial.print(energiasct,3);     Serial.println("kWh");      
            Serial.print("Frequency 1: ");    Serial.print(frecuencia1, 1); Serial.println("Hz");
            Serial.print("PF 1: ");           Serial.println(factor1);          
        }
        energiaTOT = energiasct;
        Serial.println("-------------------");
        Serial.println();

 if(clave == "bifasico"){

     //Fase 2 --------------------------------------

          // Print the Address of the PZEM
          // Serial.print("PZEM ");
          // Serial.print("2");
          // Serial.print(" - Address:");
          // Serial.println(pzems[1].getAddress(), HEX);
          // Serial.println("===================");

         // Read the data from the sensor
          voltaje2 = pzems[1].voltage();
          corriente2 = pzems[1].current();
          if(corriente2 < 1 ){
            current2 = pzems[1].current();
           } else{
           current2 = pzems[1].current()*sct_offset2;
           }
          factor2 = pzems[1].pf();
          poder2 = pzems[1].power()*sct_offset2;
          energia2 = pzems[1].energy()*sct_offset2;
          frecuencia2 = pzems[1].frequency();
    
        // Check if the data is valid
        if(isnan(voltaje2)){
            Serial.println("Error reading voltage 2");
        } else if (isnan(current2)) {
            Serial.println("Error reading current 2");
        } else if (isnan(poder2)) {
            Serial.println("Error reading power 2");
        } else if (isnan(energia2)) {
            Serial.println("Error reading energy 2");
        } else if (isnan(frecuencia2)) {
            Serial.println("Error reading frequency 2");
        } else if (isnan(factor2)) {
            Serial.println("Error reading power factor 2");
        } else {
            // Print the values to the Serial console
            Serial.print("Voltage 2: ");      Serial.print(voltaje2);      Serial.println("V");
            Serial.print("Current 2: ");      Serial.print(current2);      Serial.println("A");
            Serial.print("Power 2: ");        Serial.print(poder2);        Serial.println("W");
            Serial.print("Energy 2: ");       Serial.print(energia2,3);     Serial.println("kWh");
            Serial.print("Frequency 2: ");    Serial.print(frecuencia2, 1); Serial.println("Hz");
            Serial.print("PF 2: ");           Serial.println(factor2);

        }
        energiaTOT = energiasct + energia2;
        Serial.println("-------------------");
        Serial.println();

 }
 if(clave == "Trifasico"){
       //Fase 2 --------------------------------------

          // Print the Address of the PZEM
          // Serial.print("PZEM ");
          // Serial.print("2");
          // Serial.print(" - Address:");
          // Serial.println(pzems[1].getAddress(), HEX);
          // Serial.println("===================");

         // Read the data from the sensor
          voltaje2 = pzems[1].voltage();
          corriente2 = pzems[1].current();
          if(corriente2 < 1 ){
            current2 = pzems[1].current();
           } else{
           current2 = pzems[1].current()*sct_offset2;
           }
          factor2 = pzems[1].pf();
          poder2 = pzems[1].power()*sct_offset2;
          energia2 = pzems[1].energy()*sct_offset2;
          frecuencia2 = pzems[1].frequency();
    
        // Check if the data is valid
        if(isnan(voltaje2)){
            Serial.println("Error reading voltage 2");
        } else if (isnan(current2)) {
            Serial.println("Error reading current 2");
        } else if (isnan(poder2)) {
            Serial.println("Error reading power 2");
        } else if (isnan(energia2)) {
            Serial.println("Error reading energy 2");
        } else if (isnan(frecuencia2)) {
            Serial.println("Error reading frequency 2");
        } else if (isnan(factor2)) {
            Serial.println("Error reading power factor 2");
        } else {
            // Print the values to the Serial console
            Serial.print("Voltage 2: ");      Serial.print(voltaje2);      Serial.println("V");
            Serial.print("Current 2: ");      Serial.print(current2);      Serial.println("A");
            Serial.print("Power 2: ");        Serial.print(poder2);        Serial.println("W");
            Serial.print("Energy 2: ");       Serial.print(energia2,3);     Serial.println("kWh");
            Serial.print("Frequency 2: ");    Serial.print(frecuencia2, 1); Serial.println("Hz");
            Serial.print("PF 2: ");           Serial.println(factor2);

        }

        Serial.println("-------------------");
        Serial.println();

  //Fase 3 -------------------------------------------

        // Print the Address of the PZEM
        // Serial.print("PZEM ");
        // Serial.print("3");
        // Serial.print(" - Address:");
        // Serial.println(pzems[2].getAddress(), HEX);
        // Serial.println("===================");

        // Read the data from the sensor
        voltaje3 = pzems[2].voltage();
        corriente3 = pzems[2].current();
        if(corriente3 < 1 ){
          current3 = pzems[2].current();
        }else{
          current3 = pzems[2].current()*sct_offset3;
        }
        factor3 = pzems[2].pf();
        poder3 = pzems[2].power()*sct_offset3;
        energia3 = pzems[2].energy()*sct_offset3;
        frecuencia3 = pzems[2].frequency();
        
        // Check if the data is valid
        if(isnan(voltaje3)){
            Serial.println("Error reading voltage3");
        } else if (isnan(current3)) {
            Serial.println("Error reading current3");
        } else if (isnan(poder3)) {
            Serial.println("Error reading power3");
        } else if (isnan(energia3)) {
            Serial.println("Error reading energy3");
        } else if (isnan(frecuencia3)) {
            Serial.println("Error reading frequency3");
        } else if (isnan(factor3)) {
            Serial.println("Error reading power factor3");
        } else {
            // Print the values to the Serial console
            Serial.print("Voltage 3: ");      Serial.print(voltaje3);      Serial.println("V");
            Serial.print("Current 3: ");      Serial.print(current3);      Serial.println("A");
            Serial.print("Power 3: ");        Serial.print(poder3);        Serial.println("W");
            Serial.print("Energy 3: ");       Serial.print(energia3,3);     Serial.println("kWh");
            Serial.print("Frequency 3: ");    Serial.print(frecuencia3, 1); Serial.println("Hz");
            Serial.print("PF 3: ");           Serial.println(factor3);
         }

        energiaTOT = energiasct + energia2 + energia3;
        Serial.println("Next -------------------");
        Serial.println();
      Serial.println();
      delay(1000);
 }
}  
     
void uploadData(){
    Serial.println("Datos a enviar");     
      Serial.print("Voltage 1: ");      Serial.print(voltaje1);      Serial.println("V");
            Serial.print("Current 1: ");      Serial.print(current);      Serial.println("A");           
            Serial.print("Power 1: ");        Serial.print(podersct);        Serial.println("W");
            Serial.print("Energy 1: ");       Serial.print(energiasct,3);     Serial.println("kWh");      
            Serial.print("Frequency 1: ");    Serial.print(frecuencia1, 1); Serial.println("Hz");
            Serial.print("PF 1: ");           Serial.println(factor1);       
   bool a = true;
   if(a){ 
   Serial.println("Uploading DATA");
   // succes();
    int valor = Firebase.getInt(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/Reset_data/reset/");
    int valor_fb = firebaseData.intData();
    Serial.print("El valor de Valor ------>>>>");
    Serial.println(valor);
    Serial.print("El valor de Valor_FB ------>>>>");
    Serial.println(valor_fb); // 1
    delay(200);
 if(valor_fb == 1){
       pzems[0].resetEnergy();
       pzems[1].resetEnergy();
       pzems[2].resetEnergy(); 
       //se crea o se modifica el reset
       int dato = 0;
      Firebase.setInt(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/Reset_data/reset/",dato);
      valor = 0;        
      Serial.println("Los 3 Pzem se resetean y se reinicia el resteo en cero");
        setColor(255, 255, 0);
        delay(250);
        setColor(0, 0, 0);
        delay(250);
        setColor(255, 255, 0);
        delay(250);
        setColor(0, 0, 0);        
    }
  if(proyecto == "SmartMeter"){
    Serial.println("Proyecto SmartMeter");
    if (voltaje1 > 60){
      Serial.println("Voltaje adecuado");
      if(factor1 < 1 && factor1 >= 0){
        Serial.println("factor de potencia adecuada");
         // if(tipo == "Voltaje"){
                Serial.println("Subiendo datos de Voltaje)");
                //estado anteior   ------------------------ 
                Firebase.setString(firebaseData,"/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/timestamp", hora );             
                
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/Voltaje1/", voltaje1 );
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/Corriente/1", current );
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/KW1/", podersct );
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/KWh1/", energiasct );
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/frecuencia1/", frecuencia1 );
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/FactorPotencia1/", factor1 );

            Firebase.setString(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/zfecha", fecha );
            Firebase.setString(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/zhora", hora);
            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/Voltaje1", voltaje1 );
            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/Corriente1", current );
            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/KW1", podersct );
            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/KWh1", energiasct );
            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/frecuencia1", frecuencia1 ); 
            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/FactorPotencia1", factor1 );
        if(clave == "bifasico"){
                               
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/Voltaje2", voltaje2 );
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/Corriente2", current2 );
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/KW2", poder2 );
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/KWh2", energia2 );
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/frecuencia2", frecuencia2 );
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/FactorPotencia2", factor2 );

            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/Voltaje2", voltaje2 );
            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/Corriente2", corriente2 );
            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/KW2", poder2 );
            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/KWh2", energia2 );
            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/frecuencia2", frecuencia2 ); 
            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/FactorPotencia2", factor2 );
        }    
         if(clave == "Trifasico"){      

                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/Voltaje2", voltaje2 );
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/Corriente2", current2 );
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/KW2", poder2 );
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/KWh2", energia2 );
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/frecuencia2", frecuencia2 );
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/FactorPotencia2", factor2 );  

                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/Voltaje3", voltaje3 );
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/Corriente3", current3 );
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/KW3", poder3 );
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/KWh3", energia3 );
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/frecuencia3", frecuencia3 );
                Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/20"+ano+"/"+mes+"/"+"/"+dia+"/"+hora+"/FactorPotencia3", factor3 ); 

            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/Voltaje2", voltaje2 );
            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/Corriente2", corriente2 );
            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/KW2", poder2 );
            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/KWh2", energia2 );
            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/frecuencia2", frecuencia2 ); 
            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/FactorPotencia2", factor2 );

            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data/Voltaje3", voltaje3 );
            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/Corriente3", corriente3 );
            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/KW3", poder3 );
            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/KWh3", energia3 );
            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/frecuencia3", frecuencia3 ); 
            Firebase.setFloat(firebaseData, "/"+lugar+"/"+proyecto+"/"+medidor+"/last_data"+"/FactorPotencia3", factor3 );
            Serial.println("Data 3 phase Uploaded!");    
            }      
        //GUARDANDO LAST DATA -------   
      }else{
        Serial.println("Mal formato de FP");
      }
     }else{
      Serial.println("Mal formato de voltaje");
    }
  }
    Serial.println("Data Placed");

    upload();
    } else {
    Serial.println("Error, no internet :(");
    failure();
  }
}

String get_time(){ 
   setenv("TZ", "UTC+05:00", 1);
   String tiempo = printTime();
   hora = tiempo.substring(11,20); 
  //  Serial.println("  COLOMBIAN Time = "+ hora);    
  //  Serial.println(hora);             
  //  Serial.println("----HORA LOCAL DE COLOMBIA SUMERCÉ-----");
   return hora;
}

void get_date(){ 
   setenv("TZ", "UTC+05:00", 1);
   String tiempo = printTime();
   ano = tiempo.substring(7,9);
   mes = tiempo.substring(3,6);
   dia = tiempo.substring(0,2);
  //  Serial.println("  COLOMBIAN year = "+ ano);
  //  Serial.println("  COLOMBIAN Month = "+ mes);
  //  Serial.println("  COLOMBIAN Day = "+ dia);
   fecha = dia+"-"+mes+"-20"+ano;    
  //  Serial.println(fecha);             
  //  Serial.println("----FECHA LOCAL DE COLOMBIA SUMERCÉ-----");
}

void loop() {
     botondatos = digitalRead(puldata);
     botonpzem = digitalRead(pulpzem);
    if (configured == true){
     setColor(255, 0, 255);
     delay(1000);
     setColor(0, 0, 0);
     Serial.println("Modo configuracion");
     delay(700);
    }else{    
      Serial.println("-----------LOOP MODO OPERACION--------------- ");
      setColor(0, 255, 0);      
      Serial.println(fecha);
      Serial.println(hora);
    if(lapso >= 15 ){
      lapso = 0;
      banderaWifi = true;
    }
    else{       
      lapso++;
      //delay(1000);
      read_values();
  /*    
       ////-----------------------/-----------------------/-----------------------/-----------------------
  ////-----------------------/--------------FASE 1--------/-----------------------/-----------------------
  ////-----------------------/-----------------------/-----------------------/-----------------------
     estadoActualV1 = voltaje1;
     estadoActualI1 = current; 
     estadoActualW1 = podersct;
     estadoActualFP1 = factor1;

     estadoActualV2 = voltaje2;
     estadoActualI2 = current2; 
     estadoActualW2 = poder2;
     estadoActualFP2 = factor2;

     estadoActualV3 = voltaje3;
     estadoActualI3 = current3; 
     estadoActualW3 = poder3;
     estadoActualFP3 = factor3;
    //Voltaje -----------------------------------------------
      float resV1 = estadoActualV1 - estadoAnteriorV1; //130 - 0 = 10 volitos
      float resV2 = estadoActualV2 - estadoAnteriorV2;
      float resV3 = estadoActualV3 - estadoAnteriorV3;
      Serial.println(resV1);
      //la resta debe dar positivo cuando voltaje sube y negativo cuando voltaje baja
        if ((resV1 > 2 || resV1 < -2) || (resV2 > 2 || resV2 < -2) || (resV3 > 2 || resV3 < -2) ){
          Serial.println("el voltaje de la fase 1 vario");
          //el voltaje subio a comparacion con la anterior lectura
          //get_time();
          get_date();
          String hora_actual = get_time();
          delay(3000);
          String hora_anterior = get_time();
          delay(1000);
          Serial.println("HORAS");
          Serial.println(hora_actual);
          Serial.println(hora_anterior);
          uploadData(
            hora_actual,
            hora_anterior,
            estadoActualV1,
            estadoAnteriorV1,
            estadoActualW1,
            estadoAnteriorW1,
            estadoActualFP1,
            estadoAnteriorFP1,

                     
            "Voltaje");
        }
        /*estadoActualV2,
            estadoAnteriorV2,
            estadoActualW2,
            estadoAnteriorW2,
            estadoActualFP2,
            estadoAnteriorFP2, 

            estadoActualV3,
            estadoAnteriorV3,
            estadoActualW3,
            estadoAnteriorW3,
            estadoActualFP3,
            estadoAnteriorFP3, */
    //Corriente ------------------------------------------------------------------
   /*     float resI1 = estadoActualI1 - estadoAnteriorI1; // estado actual = 0.5 estado anterior= 3,48
        float resI2 = estadoActualI2 - estadoAnteriorI2;
        float resI3 = estadoActualI3 - estadoAnteriorI3;
        
        //la resta debe dar positivo cuando voltaje sube y negativo cuando voltaje baja
          if ((resI1 >  1||resI1 < -1) || (resI2 >  1||resI2 < -1) || (resI3 >  1||resI3 < -1)){
              Serial.println("La corriente de la fase 1 vario");
            //el voltaje subio a comparacion con la anterior lectura
            get_date();
            String hora_actual = get_time();
            delay(3000);
            String hora_anterior = get_time(); 
            delay(1000); 
            Serial.println(hora_actual);
            uploadData(
            hora_actual,
            hora_anterior,
            estadoActualI1,
            estadoAnteriorI1,
            estadoActualW1,
            estadoAnteriorW1,
            estadoActualFP1,
            estadoAnteriorFP1,


            "Corriente");
          }
    estadoAnteriorV1 = estadoActualV1;
    estadoAnteriorI1 = estadoActualI1;  
    estadoAnteriorW1 = estadoActualW1;
    estadoAnteriorFP1 = estadoActualFP1;

    estadoAnteriorV2 = estadoActualV2;
    estadoAnteriorI2 = estadoActualI2;  
    estadoAnteriorW2 = estadoActualW2;
    estadoAnteriorFP2 = estadoActualFP2;

    estadoAnteriorV3 = estadoActualV3;
    estadoAnteriorI3 = estadoActualI3;  
    estadoAnteriorW3 = estadoActualW3;
    estadoAnteriorFP3 = estadoActualFP3;    
      /*          estadoActualI2,
            estadoAnteriorI2,
            estadoActualW2,
            estadoAnteriorW2,
            estadoActualFP2,
            estadoAnteriorFP2,
            
            estadoActualI3,
            estadoAnteriorI3,
            estadoActualW3,
            estadoAnteriorW3,
            estadoActualFP3,
            estadoAnteriorFP3,*/
  ////-----------------------/-----------------------/-----------------------/-----------------------
      Serial.println(lapso);
    }
    if ( banderaWifi == true ){
        get_time();   
        get_date();
 //funcion que verifica si ya cambio el dia o si ya amanecio----------------------------------------------------
       int dia_int = dia.toInt();
       int resta = dia_int - dia_anterior;
       Serial.println(dia_anterior);
       Serial.println(dia_int);
        Serial.println(resta);
      if(resta == dia_int){
        Serial.println("CONFIGURACION DE DIA ANTERIOR");
        dia_anterior = dia_int-1;
      }else if(resta == 1){
        Serial.println("CAMBIO DE DIAA ");
        nuevo_dia = true;
        dia_anterior = dia_int;         
      } 
      if(nuevo_dia == true){
        String sub_hora = hora.substring(0,5); 
        Serial.println(sub_hora);
        hora_lec = "20:30";
        if(sub_hora == hora_lec){
          Firebase.begin(FIREBASE_HOST_GENERAL,FIREBASE_Authorization_key_GENERAL);  
          Serial.println("LLEGO LA HORA DE LA NOTIFICACION!!!!!!");
          nuevo_dia = false;
          energiaTOT_actual = energiaTOT;
          float consumo_dia = energiaTOT_actual-energiaTOT_anterior;
          // const String consumo_dia = "54.23";
          energiaTOT_anterior = energiaTOT_actual;
          char miString[10];
          dtostrf(consumo_dia, 4, 2, miString);
          String topic_info = firebase_doc+","+miString;
          Firebase.setString(firebaseDataGEN,"/user_code/"+firebase_doc+"/medidores/"+fecha+"/topic", topic_info );
          Firebase.setString(firebaseDataGEN,"/user_code/"+firebase_doc+"/medidores/"+fecha+"/consumo", miString );
          delay(1000);
          Firebase.begin(FIREBASE_HOST,FIREBASE_Authorization_key);  
       }
      }   
  //------------------------------------------------------------------------------------------------------
        uploadData();
        //uploadDataGEN();
        banderaWifi = false; 
       }    
 }  

 // Reseteo de datos PZEM
      if(botonpzem == 1){                 
        contador_de_boton++;
        Serial.println("Reset PZEM: " + contador_de_boton);  
        Serial.println(contador_de_boton);      
        if(contador_de_boton >= 10){
           String hora_ya = get_time();   
           Serial.println(fecha);
            Serial.println(hora_ya);       
           Firebase.setString(firebaseData,"/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/Reset_data/rst_date", fecha );
           Firebase.setString(firebaseData,"/"+lugar+"/"+proyecto+"/"+medidor+"/"+"/Reset_data/rst_time", hora_ya );
           contador_de_boton = 0;
           pzems[0].resetEnergy();
           pzems[1].resetEnergy();
           pzems[2].resetEnergy(); 
           Serial.println("Pzem reseteados por Hardware");
        setColor(255, 255, 0);
        delay(500);
        setColor(0, 0, 0);
        delay(500);
        setColor(255, 255, 0);
        delay(500);
        setColor(0, 0, 0);            
         }            
     } 

 // ESTE ES EL RESETEO DE LOS DATOS POR HARDWARE
    if(botondatos == 1){
      contador_hard++;
      Serial.print("Reset de credenciales: ");
      Serial.println(contador_hard);
      if(contador_hard >= 10){
        setColor(255, 255, 0);
        delay(500);
        setColor(0, 0, 0);
        delay(500);
        setColor(255, 255, 0);
        delay(500);
        setColor(0, 0, 0);
        preferences.begin("credentials", false);
        preferences.clear();
        preferences.end();
        Serial.println("Datos eliminados por hardware");
        ESP.restart();
      }     
    }
}