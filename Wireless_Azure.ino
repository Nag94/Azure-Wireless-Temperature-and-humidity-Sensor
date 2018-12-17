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

/*String containing Hostname, Device Id & Device Key in the format:                         */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessKey=<device_key>"                */
/*  "HostName=<host_name>;DeviceId=<device_id>;SharedAccessSignature=<device_sas_token>"    */
static const char* connectionString = "Enter Azure Connection String here";

const char *messageData = "{\"deviceId\":\"%s\", \"messageId\":%d, \"TemperatureF\":%.2f, \"Humidity\":%.2f, \"TemperatureC\":%.2f, \"Battery\":%.2f}";


int messageCount = 1;
static bool messageSending = true;


uint8_t data[29];
int k = 10;
int i;


static float humidity; 
static int16_t cTempint; 
static float cTemp ;
static float fTemp ;
static float battery ;
static float voltage ;

static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result)
{
  if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
  {
    Serial.println("Send Confirmation Callback finished.");
  }
}

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
  Esp32MQTTClient_Init((const uint8_t*)connectionString, true);
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


if(messageSending){
if(millis()-msg_Timer >= msg_Interval){
     msg_Timer = millis();
     Serial.println("Sending Message");
   // Send teperature data
      char messagePayload[MESSAGE_MAX_LEN];
      snprintf(messagePayload,MESSAGE_MAX_LEN, messageData, DEVICE_ID, messageCount++, fTemp,humidity,cTemp,voltage);
      Serial.println(messagePayload);
      EVENT_INSTANCE* message = Esp32MQTTClient_Event_Generate(messagePayload, MESSAGE);
      Esp32MQTTClient_Event_AddProp(message, "temperatureAlert", "true");
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
