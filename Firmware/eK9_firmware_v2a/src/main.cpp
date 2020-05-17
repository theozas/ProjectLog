/*
Paulius Miliunas
K9 firmware
final yeare project
code example was used from https://www.instructables.com/id/ESP32-BLE-Android-App-Arduino-IDE-AWESOME/
*/


#include <Arduino.h>
//libraries for bluetooth device
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>




//uuid number for data transmition
#define sensorService BLEUUID((uint16_t)0x180D)
//presetting up characteristicks
BLECharacteristic sensorCharacteristic(BLEUUID((uint16_t)0x2A37), 
                BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
BLEDescriptor sensorDescriptor(BLEUUID((uint16_t)0x2901));



unsigned int tNow,tOld;//variables for timer counter
double periodInSeconds,frequency,period,sum = 0,averige;
static int devider = 1000000;//variable uset to convert us to ms
int count = 0;
int nOfSamples = 25;//number of samples to read
int led = 19;//GPIO used for LED
int loverTreshold = 2970;
int higherTreshold = 3050;
bool clientConnected = false;
double testVal = 0;

void taskOne( void * parameter );//prototype for task function
void IRAM_ATTR ISR();
//void BLESetup();

TaskHandle_t taskOneHandle = NULL;//task hanler
hw_timer_t * timer = NULL;//creating variable of hw_timer_r type to hold setings
portMUX_TYPE tMux = portMUX_INITIALIZER_UNLOCKED;//this will do sinc between main lop and ISR, vhen modifieng variable

//---------------------------------
//calback function
//-----------------------------------
class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      clientConnected = true;
    }

    void onDisconnect(BLEServer* pServer) {
      clientConnected = false;
    }
};

//--------------------------------
//Bluetooth Module Setup
//--------------------------------
void BLESetup() {
  BLEDevice::init("esp32 ble test");
  // Create the BLE Server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  // Create the BLE Service
  BLEService *pSensor = pServer->createService(sensorService);

  //adding characteristiks and setuping that when new data awailable send to client
  pSensor->addCharacteristic(&sensorCharacteristic);
  sensorDescriptor.setValue("Frequency readings");
  sensorCharacteristic.addDescriptor(&sensorDescriptor);
  //ble2902 cheking if there is client conected 
  //if conected tehn notifications are send
  sensorCharacteristic.addDescriptor(new BLE2902());

  //Advertising service for clients 
  pServer->getAdvertising()->addServiceUUID(sensorService);

  pSensor->start();
  // Start advertising
  pServer->getAdvertising()->start();
}

//----------------------------------------------
//Arduino function runs befor loop function ones
//----------------------------------------------
void setup() {

  Serial.begin(112500);  //seriel comms 
    Serial.println("Entering void Setup\n");

  timer = timerBegin(0,80,true);//starting timer with prescales of 80 timer counts in period of 1us
  tOld = 0;
  pinMode(led, OUTPUT);//setin led pin as output
  attachInterrupt(T0,ISR,FALLING);//ataching interupt on pin 4 on esp32 dev kit v1 
  xTaskCreate(taskOne,"TaskOne",50000,NULL,2,&taskOneHandle);//creating task 1 task will calculate frequency
  BLESetup();
  digitalWrite(led, LOW);
  Serial.println("Characteristic defined! Now you can read it in your phone!\n");
}
void loop() {
  sensorCharacteristic.setValue(testVal);
  sensorCharacteristic.notify();
  testVal++;
  Serial.print("My uuid is: ");
  Serial.println();
  delay(1000);
}

//---------------------------------------------------------------------
//task converts time in to seconds and calculates frequency depending 
//on frequency lover treshhold (Ferite metals)
//and higher treshold (non-ferite metals) turns on led for debug porpose
//---------------------------------------------------------------------
void taskOne( void * parameter ){   
    for(;;){
      periodInSeconds = averige/devider; //converting us to s
      frequency = 1 / periodInSeconds;//calculate frequency
      
      Serial.printf("Frequency (%f)hz\n", frequency);//testing
      if(frequency > higherTreshold){
        digitalWrite(led, HIGH);
      }else if(frequency < loverTreshold){
        digitalWrite(led, HIGH);
      }else{
        digitalWrite(led, LOW);
      }
      //pCharacteristic->setValue(frequency);//seting value
      vTaskSuspend( NULL );
    }
 }

//-----------------------------------------
//GPIO hardware interupt pin setup
//-----------------------------------------
void IRAM_ATTR ISR() {//pin interupt hanler
    portENTER_CRITICAL_ISR(&tMux);//disable all incoming interups
    
    tNow = micros();
    period = tNow - tOld;//geting period betven this nad previus faling edge in us
    tOld = tNow;//saving this interupt time
    count++;//incrementing count
    //calculating fequency with averige vith sample rate defined with nOfSamples
    if(count < nOfSamples){
       sum = sum + period;
    }else if(count == nOfSamples){
       averige = sum/nOfSamples;
       count = 0;
       sum = 0;
     }
    //Serial.printf("Now(%u)us: Before(%u)us: Interval(%f)us: Frequency (%f)hz: Period(%f)s\r", tNow, tOld, period, frequency, periodInSeconds);//testing
    portEXIT_CRITICAL_ISR(&tMux);//enable ll  incoming interupts
    xTaskResumeFromISR(taskOneHandle); 
}

