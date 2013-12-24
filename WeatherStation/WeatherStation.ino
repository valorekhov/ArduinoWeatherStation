#include <Wire.h>
#include "Adafruit_Sensor.h"
#include "Adafruit_TSL2561_U.h"
#include "Adafruit_BMP085_U.h"
#include "DHT.h"
#include "Narcoleptic.h"

#define LEDPIN 13    

//DHT Sensor Settings
#define DHTPIN 4     // what pin we're connected to
DHT dht;

#define XBEESLEEPPIN 2

// Connect pin 1 (on the left) of the sensor to +5V
// Connect pin 2 ouf the sensor to whatever your DHTPIN is
// Connect pin 4 (on the right) of the sensor to GROUND
// Connect a 10K resistor from pin 2 (data) to pin 1 (power) of the sensor
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

int sample=0;

void setup() {
  pinMode( LEDPIN , OUTPUT);
  pinMode( XBEESLEEPPIN , OUTPUT);
  
  Serial.begin(57600); 
  dht.setup(DHTPIN);
  
  if(!bmp.begin())
  {
    Serial.print("Ooops, no BMP190 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
  
  if(!tsl.begin())
  {
    /* There was a problem detecting the ADXL345 ... check your connections */
    Serial.print("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
}

void loop() {
  digitalWrite(LEDPIN, HIGH);
  digitalWrite(XBEESLEEPPIN, LOW);  //Wake UP XBEE

  configureLightSensor();
   
  Serial.print("{'_sensor': 'weather', '_sample': {'_id':"); 
  Serial.print(sample++); 
  Serial.print(", "); 
  
  float h = dht.getHumidity();
  float t = dht.getTemperature();

  // check if returns are valid, if they are NaN (not a number) then something went wrong!
  if (!isnan(t) && !isnan(h)) {
   if (!isnan(t)){
    Serial.print("'Temp':"); 
    Serial.print(t);
    Serial.print(", ");
   }
   if (!isnan(h)){
    Serial.print("'RH':"); 
    Serial.print(h);
    Serial.print(", ");
   }
  } 
//  else {
//   Serial.println("Failed to read from DHT");
//  }
  
  
  sensors_event_t event;
  bmp.getEvent(&event);
 
  Serial.print("'AtmoPressure':");
  if (event.pressure)
  {
     Serial.print(event.pressure); Serial.print(", ");
  }
  else
  {
    Serial.print("'NaN', ");
  }
  
  tsl.getEvent(&event); 
  Serial.print("'AmbientLight':"); 
  if (event.light)
  {
    Serial.print(event.light); Serial.print(", ");
  }
  else
  {
    Serial.print("'NaN' ");
  }
  Serial.println("}}");
  
  digitalWrite(LEDPIN, LOW);
  digitalWrite(XBEESLEEPPIN, HIGH);  //Put XBEE to sleep
  
  sleep(30000);
}

void configureLightSensor(void)
{
  /* You can also manually set the gain or enable auto-gain support */
  // tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
  // tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
  tsl.enableAutoGain(true);             /* Auto-gain ... switches automatically between 1x and 16x */
  
  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
  //tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
  tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */
}

void sleep(long milliseconds)
{
   while(milliseconds > 0)
   {
      if(milliseconds > 8000)
      {
         milliseconds -= 8000;
         Narcoleptic.delay(8000);
      }
      else
      {
        Narcoleptic.delay(milliseconds);
        break;
      }
   }
}
