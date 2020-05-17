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
//uuid numbers for data transmition
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"


//variebles used in the code
unsigned int tNow,tOld;//variables for timer counter
double periodInSeconds,frequency,period,sum = 0,averige;
static int devider = 1000000;//variable uset to convert us to ms
int count = 0;
int nOfSamples = 25;//number of samples to read
int led = 19;//GPIO used for LED
int loverTreshold = 2970;
int higherTreshold = 3050;
bool clientConnected = false;

void taskOne( void * parameter );//prototype for task function
void IRAM_ATTR ISR();

TaskHandle_t taskOneHandle = NULL;//task hanler
hw_timer_t * timer = NULL;//creating variable of hw_timer_r type to hold setings
portMUX_TYPE tMux = portMUX_INITIALIZER_UNLOCKED;//this will do sinc between main lop and ISR, vhen modifieng variable

//call back function for server
class ServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      clientConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      clientConnected = false;
    }
};


void setup() {
  Serial.begin(112500);  //seriel comms 
  timer = timerBegin(0,80,true);//starting timer with prescales of 80 timer counts in period of 1us
  tOld = 0;
  pinMode(led, OUTPUT);//setin led pin as output
  attachInterrupt(T0,ISR,FALLING);//ataching interupt on pin 4 on esp32 dev kit v1 

  xTaskCreate(taskOne,"TaskOne",20000,NULL,2,&taskOneHandle);//creating task 1 task will calculate frequency
  // xTaskCreate(taskTwo,"TaskTwo",configMINIMAL_STACK_SIZE,NULL,1,NULL);//creating task 2
  Serial.println("Setuo of BLE..!");
  //creating BLE device object with folowing name
  BLEDevice::init("eK9 transmiting");
  //creating BLE server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  //creating ble service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  //adding properties to ble dvice
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE
                                       );

  pCharacteristic->setValue(frequency);//
  pService->start();//staring sevice
  // BLEAdvertising *pAdvertising = pServer->getAdvertising();  // this still is working for backward compatibility
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  //pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  digitalWrite(led, LOW);
  Serial.println("Characteristic defined! Now you can read it in your phone!\n");
}
void loop() {
  //delayMicroseconds(1);
}

//task converts time in to seconds and calculates frequency depending on frequency lover treshhold (Ferite metals)
//and higher treshold (non-ferite metals) turns on led for debug porpose
void taskOne( void * parameter ){   
    for(;;){
      periodInSeconds = averige/devider; //converting us to s
      frequency = 1 / periodInSeconds;
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
