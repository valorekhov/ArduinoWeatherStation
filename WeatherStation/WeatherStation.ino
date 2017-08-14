#include <Wire.h>
#include "Adafruit_Sensor.h"
//#include "Adafruit_TSL2561_U.h"
#include "Adafruit_BMP085_U.h"
#include "DHT.h"
#include "Narcoleptic.h"

#define LEDPIN 13    

//DHT Sensor Settings
#define DHTPIN 4     // what pin we're connected to
DHT dht;

#define XBEESLEEPPIN 2

Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);
//Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

void setup() {
  pinMode( LEDPIN , OUTPUT);
  pinMode( XBEESLEEPPIN , OUTPUT);

/* digitalWrite(XBEESLEEPPIN, LOW);  //Wake UP XBEE
*/

  Serial.begin(57600); 
  
  if(!bmp.begin())
  {
    Serial.print("Ooops, no BMP190 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }
  
  /*if(!tsl.begin())
  {
    Serial.print("Ooops, no TSL2561 detected ... Check your wiring or I2C ADDR!");
    while(1);
  }*/
  
  //configureLightSensor();
  
  //Delay for 15 sec at startup after a manual RST to allow for the wireless programmer to transition 
  // the board into the bootloader, or change other XBEE settings remotely, c.f. disable pin sleep mode to keep the XBEE up
  //blink(15, 500);
}

void blink(int times){
  blink(times, 250);
}

void blink(int times, int delayValue){
   for(int i=0; i<times; i++){
      digitalWrite(LEDPIN, LOW);
      delay(delayValue); 
      digitalWrite(LEDPIN, HIGH);
      delay(delayValue); 
   }
}

void loop() {
  digitalWrite(XBEESLEEPPIN, LOW);  //Wake UP XBEE
  //delay(2500);
  dht.setup(DHTPIN);


  blink(30, 50);                    //3 sec delay on wake up to allow the sample to be sent

  digitalWrite(LEDPIN, HIGH);  

    
  float bmpTemperature;
  float dhtTemperature = dht.getTemperature();
  bmp.getTemperature(&bmpTemperature);
  float humidity = dht.getHumidity();

  float tempAvg = NAN;
  if (!isnan(dhtTemperature) && !isnan(bmpTemperature)){
     tempAvg = (dhtTemperature + bmpTemperature) / 2;
  } else if (!isnan(bmpTemperature)){
    tempAvg = bmpTemperature;
  }

  sensors_event_t event;
  bmp.getEvent(&event); 

  Serial.print("EXW: ");
  Serial.print("T1="); printValue(bmpTemperature);
  Serial.print("T2="); printValue(dhtTemperature);
  Serial.print("RH="); printValue(humidity);
  Serial.print("T="); printValue( tempAvg );  
  Serial.print("P="); printValue(event.pressure);
  
  /*uint16_t broadband = 0;
  uint16_t infrared = 0;
 
  tsl.getEvent(&event); 
  long lx = event.light;
  tsl.getLuminosity (&broadband, &infrared);
  if (lx == 0 && broadband > 65000){ //Sensor appears to be in direct sunlight outside it's resolution limits. Set lx value to max to simplify downstream logic
      lx = 0xFFFF;
  }

  if (lx != 0xFFFF && broadband != 0xFFFF && infrared != 0xFFFF){ //If sensor is not connected, all values are 0xFFFF
    Serial.print("AL="); printValue(lx);
    Serial.print("BL="); printValue((long)broadband);
    Serial.print("IR="); printValue((long)infrared);
  }*/
  Serial.print("\n");

  delay(1500);   //Delay at the end to allow xbee to flush its send buffer 

  digitalWrite(LEDPIN, LOW);
  digitalWrite(XBEESLEEPPIN, HIGH);  //Put XBEE to sleep
  
  sleep(600000);
}

void printValue(float value){
  if (!isnan(value))
  {
    Serial.print(value); 
    Serial.print(" ");
  }
  else
  {
    Serial.print("NaN ");
  }
}

void printValue(long value){
  if (!isnan(value))
  {
    Serial.print(value); 
    Serial.print(" ");
  }
  else
  {
    Serial.print("NaN ");
  }
}

void configureLightSensor(void)
{
  /* You can also manually set the gain or enable auto-gain support */
  // tsl.setGain(TSL2561_GAIN_1X);      /* No gain ... use in bright light to avoid sensor saturation */
  // tsl.setGain(TSL2561_GAIN_16X);     /* 16x gain ... use in low light to boost sensitivity */
  ////tsl.enableAutoGain(true);             /* Auto-gain ... switches automatically between 1x and 16x */
  
  /* Changing the integration time gives you better sensor resolution (402ms = 16-bit data) */
  //tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_13MS);      /* fast but low resolution */
  // tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_101MS);  /* medium resolution and speed   */
  //tsl.setIntegrationTime(TSL2561_INTEGRATIONTIME_402MS);  /* 16-bit data but slowest conversions */
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
