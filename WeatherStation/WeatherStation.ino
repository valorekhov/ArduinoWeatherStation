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

Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);
Adafruit_TSL2561_Unified tsl = Adafruit_TSL2561_Unified(TSL2561_ADDR_FLOAT, 12345);

int sample=0;

void setup() {
  pinMode( LEDPIN , OUTPUT);
  pinMode( XBEESLEEPPIN , OUTPUT);

/* digitalWrite(XBEESLEEPPIN, LOW);  //Wake UP XBEE
*/

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
  
  configureLightSensor();
  
  //Delay for 30 sec at startup after a manual RST to allow for the wireless programmer to transition 
  // the board into the bootloader, or change other XBEE settings remotely, c.f. disable pin sleep mode to keep the XBEE up
  blink(30, 500);
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
  delay(2000);

  blink(25, 50);
  digitalWrite(LEDPIN, LOW);  
  delay(1000);

  digitalWrite(LEDPIN, HIGH);  

  sample += 1;

  if (!pingCoordinator()){          //check if xbee woke up and got its association address. blink LED rapidly and quit loop otherwise
    blink(10);
    digitalWrite(LEDPIN, LOW);
    digitalWrite(XBEESLEEPPIN, LOW);  //Wake UP XBEE
    sleep(60000); 
    return;
  }

  Serial.print("{\"_sensor\": \"weather\", \"_sample\": {\"_id\":"); 
  Serial.print(sample); 
  Serial.print(", "); 
  
  Serial.print("\"Temp\":"); printValue(dht.getTemperature());
  Serial.print("\"RH\":"); printValue(dht.getHumidity());
  
  sensors_event_t event;
  bmp.getEvent(&event); 
  Serial.print("\"AtmoPressure\":"); printValue(event.pressure);
  
  uint16_t broadband = 0;
  uint16_t infrared = 0;
 
  tsl.getEvent(&event); 
  long lx = event.light;
  tsl.getLuminosity (&broadband, &infrared);
  if (lx == 0 && broadband > 65000){ //Sensor appears to be in direct sunlight outside it's resolution limits. Set lx value to max to simplify downstream logic
      lx = 0xFFFF;
  }
  Serial.print("\"AmbientLight\":"); printValue(lx);
  Serial.print("\"BroadbandLight\":"); printValue((long)broadband);
  Serial.print("\"Infrared\":"); printValue((long)infrared, 0);

  Serial.println("}}");
  
  delay(2000);   //Delay at the end to allow xbee to flush its send buffer 

  digitalWrite(LEDPIN, LOW);
  digitalWrite(XBEESLEEPPIN, HIGH);  //Put XBEE to sleep
  
  sleep(60000);
}

void printValue(float value){
  if (!isnan(value))
  {
    Serial.print(value); Serial.print(", ");
  }
  else
  {
    Serial.print("'NaN', ");
  }
}

void printValue(long value){
  printValue(value, 1);
}

void printValue(long value, short separator){
  if (!isnan(value))
  {
    Serial.print(value); 
  }
  else
  {
    Serial.print("'NaN'");
  }
  
  if (separator){
    Serial.print(", ");
  }
}

int pingCoordinator(void){
    
    for(int i = 0; i<10; i++){
      blink(i);
      while(getCh() != NULL) ;       //Drain any pending chars on the serial.
      Serial.println("@PING");
      int c=0;
      while(Serial.available()<2 && c++<10){
          delay(400);
          blink(2, 50);
      }
      if (getCh()=='O' && getCh()=='K'){
          //while(getCh() != NULL) ;       //Drain any pending messages on the serial.
          return 1;
      }
      delay(500);
    }
    return 0;
}

char getCh(){
  if (Serial.available()){
    return Serial.read();
  }
  return NULL;
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
