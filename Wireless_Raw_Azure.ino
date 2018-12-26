///this code is written and tested for ncd.io wireless temperature humidity sensor with arduino due
///sensor data structure can be found here https://ncd.io/long-range-iot-wireless-temperature-humidity-sensor-product-manual/
/// sesnro can be found here https://store.ncd.io/product/industrial-long-range-wireless-temperature-humidity-sensor/
#include <HardwareSerial.h>
//HardwareSerial Serial1(1); // use uart2

#include <WiFi.h>
#include "AzureIotHub.h"
#include "Esp32MQTTClient.h"

#define INTERVAL 10000
#define DEVICE_ID "Enter Device ID here"
#define MESSAGE_MAX_LEN 256

// Please input the SSID and password of WiFi
const char* ssid     = "Enter SSID here";
const char* password = "Enter Password here";

// Data upload timer will run for the interval of 20s
unsigned long msg_Interval = 120000;
unsigned long msg_Timer = 0;

static const char* connectionString = "Enter Azure Connection String here";

//Create a char variable to store our JSON format
const char *messageData = "{\"deviceId\":\"%s\", \"messageId\":%d, \"temp1\":%d, \"temp2\":%d, \"humid1\":%d, \"humid2\":%d,\"bat1\":%d,\"bat2\":%d}";

//variable to keep the count of msg delivered
int messageCount = 1;

static bool messageSending = true;

// Size of frame
uint8_t data[29];
int k = 10;
int i;

//variable to store temp,humid,battery and other values
static uint8_t temp1; 
static uint8_t temp2; 
static uint8_t humid1 ;
static uint8_t humid2 ;
static uint8_t bat1 ;
static uint8_t bat2 ;

//Callback to check the confirmation frome azure
static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result)
{
  if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
  {
    Serial.println("Send Confirmation Callback finished.");
  }
}

//payload callback
static void MessageCallback(const char* payLoad, int size)
{
  Serial.println("Message callback:");
  Serial.println(payLoad);
}

static void DeviceTwinCallback(DEVICE_TWIN_UPDATE_STATE updateState, const unsigned char *payLoad, int size)
{
  char *temp = (char *)malloc(size + 1);
  if (temp == NULL)
  {
    return;
  }
  memcpy(temp, payLoad, size);
  temp[size] = '\0';
  // Display Twin message.
  Serial.println(temp);
  free(temp);
}

//device connection callback
static int  DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size)
{
  LogInfo("Try to invoke method %s", methodName);
  const char *responseMessage = "\"Successfully invoke device method\"";
  int result = 200;

  if (strcmp(methodName, "start") == 0)
  {
    LogInfo("Start sending temperature and humidity data");
    messageSending = true;
  }
  else if (strcmp(methodName, "stop") == 0)
  {
    LogInfo("Stop sending temperature and humidity data");
    messageSending = false;
  }
  else
  {
    LogInfo("No method %s found", methodName);
    responseMessage = "\"No method found\"";
    result = 404;
  }

  *response_size = strlen(responseMessage) + 1;
  *response = (unsigned char *)strdup(responseMessage);

  return result;
}


void setup()
{
  
  Serial1.begin(115200, SERIAL_8N1, 16, 17); // pins 16 rx2, 17 tx2, 19200 bps, 8 bits no parity 1 stop bit​
  
  Serial.begin(9600);
  while(!Serial);
  Serial.println("ncd.io IoT Arduino wireless temperature Humidity sensor");
   
  Serial.println("ESP32 Device");
  Serial.println("Initializing...");

  // Initialize the WiFi module
  Serial.println(" > WiFi");
  InitWifi();
  
  Serial.println(" > IoT Hub");
  Esp32MQTTClient_SetOption(OPTION_MINI_SOLUTION_NAME, "GetStarted");
  //initialise
  Esp32MQTTClient_Init((const uint8_t*)connectionString, true);
  //Set different callbacks
  Esp32MQTTClient_SetSendConfirmationCallback(SendConfirmationCallback);
  Esp32MQTTClient_SetMessageCallback(MessageCallback);
  Esp32MQTTClient_SetDeviceTwinCallback(DeviceTwinCallback);
  Esp32MQTTClient_SetDeviceMethodCallback(DeviceMethodCallback);
}

void loop()
{
  if (Serial1.available())
  {
    data[0] = Serial1.read();
    delay(k);
    //chck for start byte
   if(data[0]==0x7E)
    {
    while (!Serial1.available());
    for ( i = 1; i< 29; i++)
      {
      data[i] = Serial1.read();
      delay(1);
      }
    if(data[15]==0x7F)  /////// to check if the recive data is correct
      {
  if(data[22]==1)  //////// make sure the sensor type is correct
         {  
  humid1 = data[24];
  humid2 = data[25];
  temp1 = data[26];
  temp2 = data[27];
  bat1 = data[18];
  bat2 = data[19];
  
  Serial.print("Sensor Number  ");
  Serial.println(data[16]);
  Serial.print("Sensor Type  ");
  Serial.println(data[22]);
  Serial.print("Firmware Version  ");
  Serial.println(data[17]);
  Serial.print("Relative Humidity :");
        }
      }
else
{
      for ( i = 0; i< 29; i++)
    {
      //Serial.print(data[i]);
      //Serial.print(" , ");
      delay(1);
    }
    //Serial.println("Reading Sensor Data");
}
    }
}

//if message sending is true, send data to azure after 2 min
if(messageSending){
if(millis()-msg_Timer >= msg_Interval){
     msg_Timer = millis();
     Serial.println("Sending Message");
   // Send teperature data
      char messagePayload[MESSAGE_MAX_LEN];
      //snprintf stores the data in char buffer
      //snprintf(char buffer,  size of buffer,   format,    arg1,      arg2,            arg3, arg4, arg5,  arg6, arg7,arg8)
      snprintf(messagePayload,MESSAGE_MAX_LEN, messageData, DEVICE_ID, messageCount++, temp1,temp2,humid1,humid2,bat1,bat2);
      //here we are printing the buffer
      Serial.println(messagePayload);
      //create an event MESSAGE(message) to send the buffer(messagPayload) to Azure  
      EVENT_INSTANCE* message = Esp32MQTTClient_Event_Generate(messagePayload, MESSAGE);
      // add an event property(optional)
      Esp32MQTTClient_Event_AddProp(message, "temperatureAlert", "true");
      //send the event to azure
      Esp32MQTTClient_SendEventInstance(message); 
      delay(50);     
  }
  
  
  }
else{
        Esp32MQTTClient_Check();
    }
}

void InitWifi(){
  
    Serial.println("Connecting...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  }
