///this code is written and tested for ncd.io wireless temperature humidity sensor with arduino due
///sensor data structure can be found here https://ncd.io/long-range-iot-wireless-temperature-humidity-sensor-product-manual/
/// sesnro can be found here https://store.ncd.io/product/industrial-long-range-wireless-temperature-humidity-sensor/

/*to do1: reset connection on bad network(In Connection Callback)
  to do2: device method callback form azure portal
*/ 

#include <HardwareSerial.h>
//HardwareSerial Serial1(1); // use uart2

#include <WiFi.h>

//include these headers to setup link with azure IoT hub
#include "AzureIotHub.h"
#include "Esp32MQTTClient.h"

//keep alive interval
#define MQTT_KEEPALIVE_INTERVAL_S 120
#define DEVICE_ID "esp32demo1"
#define MESSAGE_MAX_LEN 256

// Please input the SSID and password of WiFi
const char* ssid     = "NETGEAR";
const char* password = "ab123456789";

 //Data upload timer will run for the interval of 20s
unsigned long msg_Interval = 120000;
unsigned long msg_Timer = 0;

unsigned long stream_Interval = 20000;
unsigned long stream_Timer = 0;
/*String containing Hostname, Device Id & Device Key in the format:                         */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"                */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"    */

//Connection String to interface your device with IoT hub
static const char* connectionString = "HostName=ncdio.azure-devices.net;DeviceId=esp32demo1;SharedAccessKey=XB21V5yiShM9X2OlBldstdfUi4DOZAJSTzSK6x/rtMo=";

//Create a char variable to store our JSON format
const char *messageData = "{\"deviceFormat\":\"%s\",\"deviceId\":\"%s\", \"messageId\":%d, \"TemperatureF\":%.2f, \"Humidity\":%.2f, \"TemperatureC\":%.2f, \"Battery\":%.2f}";

const char *messageRawData = "{\"deviceFormat\":\"%s\",\"deviceId\":\"%s\", \"messageId\":%d, \"Temp1\":%d, \"Temp2\":%d, \"Humid1\":%d, \"Humid2\":%d, \"Bat1\":%d, \"Bat2\":%d}";


//variable to keep the count of msg delivered
int messageCount = 1;

static bool messageRawSending = false;
static bool messageSending = true;


// Size of frame
uint8_t data[29];
int k = 10;
int i;

//variable to store temp,humid,battery and other values
static float humidity; 
static int16_t cTempint; 
static float cTemp ;
static float fTemp ;
static float battery ;
static float voltage ;


static uint8_t temp1;
static uint8_t temp2;

static uint8_t humid1;
static uint8_t humid2;

static uint8_t bat1;
static uint8_t bat2;

/*static void ConnectionStatusCallback(IOTHUB_CLIENT_CONNECTION_STATUS result, IOTHUB_CLIENT_CONNECTION_STATUS_REASON reason){
  if(reason == IOTHUB_CLIENT_CONNECTION_OK){
    if(result == IOTHUB_CLIENT_CONNECTION_AUTHENTICATED){
         clientConnected = true;
         Serial.println("clientConnected");
      }
    }
    
  }*/

  //device connection callback
static int  DeviceMethodCallback(const char *methodName, const unsigned char *payload, int size, unsigned char **response, int *response_size)
{
  Serial.printf("Try to invoke method %s", methodName);
  const char *responseMessage = "\"Successfully invoke device method\"";
  int result = 200;

  if (strcmp(methodName, "start") == 0)
  {
    LogInfo("Start sending temperature and humidity data");
    messageSending = true;
    messageRawSending = false;
  }
  else if (strcmp(methodName, "stop") == 0)
  {
    LogInfo("Stop sending temperature and humidity data");
    messageSending = false;
    messageRawSending = false;
  }
  else if (strcmp(methodName, "startRaw") == 0)
  {
    LogInfo("Start sending Raw temperature and humidity data");
    messageRawSending = true;
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
  if(updateState){
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
    else{
          Serial.println("Device State not updated");
      }
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
 
  //Keep Alive interval for device to cloud messages
  //Esp32MQTTClient_SetOption("keepalive", MQTT_KEEPALIVE_INTERVAL_S);
  
  //initialise
  //Will get Info message from IoT Hub
  //bool Esp32MQTTClient_Init(         conn String,               deviceTwin, logTrace)
  Serial.println(Esp32MQTTClient_Init((const uint8_t*)connectionString, true, true) ? "Connected to IoT Hub" : "Error Connecting to IoT Hub");
  
  //Set different callbacks
  //Esp32MQTTClient_SetConnectionStatusCallback(ConnectionStatusCallback);
  Esp32MQTTClient_SetSendConfirmationCallback(SendConfirmationCallback);
  Esp32MQTTClient_SetMessageCallback(MessageCallback);
  Esp32MQTTClient_SetDeviceTwinCallback(DeviceTwinCallback);
  Esp32MQTTClient_SetDeviceMethodCallback(DeviceMethodCallback);
}

void loop()
{
char messagePayload[MESSAGE_MAX_LEN]; 

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
  humidity = ((((data[24]) * 256) + data[25]) /100.0);
  cTempint = (((uint16_t)(data[26])<<8)| data[27]);
  cTemp = (float)cTempint /100.0;
  fTemp = cTemp * 1.8 + 32;
  battery = ((data[18] * 256) + data[19]);
 
  voltage = 0.00322 * battery;

  
 temp1 = data[26];
 temp2 = data[27];

 humid1 = data[24];
 humid2 = data[25];

 bat1 = data[18];
 bat2 = data[19];
 Serial.println(String(bat1,HEX));
 Serial.println(String(bat2,HEX)); 
  Serial.print("Sensor Number  ");
  Serial.println(data[16]);
  Serial.print("Sensor Type  ");
  Serial.println(data[22]);
  Serial.print("Firmware Version  ");
  Serial.println(data[17]);
  Serial.print("Relative Humidity :");
  Serial.print(humidity);
  Serial.println(" %RH");
  Serial.print("Temperature in Celsius :");
  Serial.print(cTemp);
  Serial.println(" C");
  Serial.print("Temperature in Fahrenheit :");
  Serial.print(fTemp);
  Serial.println(" F");
  Serial.print("ADC value:");
  Serial.println(battery);
  Serial.print("Battery Voltage:");
  Serial.print(voltage);
  Serial.println("\n");
  if (voltage < 1)
          {
    Serial.println("Time to Replace The Battery");
          }
        }
      }
    }
  }
  
//if message sending is true, send data to azure after 2 min
if(messageSending && !messageRawSending){
  if(millis()-msg_Timer> msg_Interval){
      msg_Timer= millis();
      Serial.println("Sending Message");
      String deviceFormat = "realValues"; 
      
      //snprintf stores the data in char buffer
      //snprintf(char buffer,  size of buffer,   format,         arg1,      arg2,        arg3,       arg4,  arg5,    arg6,  arg7)
      snprintf(messagePayload,MESSAGE_MAX_LEN, messageData, deviceFormat.c_str(), DEVICE_ID, messageCount++, fTemp,humidity,cTemp,voltage); 
      Serial.println(messagePayload);
      
      //create an event MESSAGE(message) to send the buffer(messagPayload) to Azure  
      EVENT_INSTANCE* message = Esp32MQTTClient_Event_Generate(messagePayload, MESSAGE);
      
      //send the event to azure
      Esp32MQTTClient_SendEventInstance(message); 
      delay(50);   
  }
}
  else if(!messageSending && messageRawSending){
     if(millis()-msg_Timer> msg_Interval){
        msg_Timer= millis();
        Serial.println("Sending Raw Message");
        
        String deviceFormat = "rawValues";
      
        //snprintf stores the data in char buffer
        //snprintf(char buffer,  size of buffer,   format,         arg1,      arg2,        arg3,       arg4,  arg5, arg6, arg7,  arg8,arg9)
        snprintf(messagePayload,MESSAGE_MAX_LEN, messageRawData, deviceFormat.c_str(), DEVICE_ID, messageCount++, temp1,temp2,humid1,humid2,bat1,bat2);
        
        //here we are printing the buffer
        Serial.println(messagePayload);
        
        //create an event MESSAGE(message) to send the buffer(messagPayload) to Azure  
        EVENT_INSTANCE* message = Esp32MQTTClient_Event_Generate(messagePayload, MESSAGE);
      
        //send the event to azure
        Esp32MQTTClient_SendEventInstance(message); 
        delay(50);      
    }    
  }
  /*else if(!messageSending && !messageRawSending){
          if(millis()- stream_Timer > stream_Interval){
                   stream_Timer = millis();
                   Serial.println("Stream is not running");            
            }
           }*/
  
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
