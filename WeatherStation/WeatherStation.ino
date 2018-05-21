// **********************************************************************************************************
// Feather 32u4 RFM69 + SHT31-D seonsor node
// It sends periodic weather readings (temp, hum, atm pressure) from WeatherShield to the base node

#include "RFM69.h"         //get it here: https://github.com/lowpowerlab/rfm69
#include "RFM69_ATC.h"     //get it here: https://github.com/lowpowerlab/rfm69
#include <SPI.h>           //included in Arduino IDE (www.arduino.cc)
#include <Wire.h>          //included in Arduino IDE (www.arduino.cc)
#include "Adafruit_SHT31.h"
#include "LowPower.h"      //get it here: https://github.com/lowpowerlab/lowpower
                           //writeup here: http://www.rocketscream.com/blog/2011/07/04/lightweight-low-power-arduino-library/

//*********************************************************************************************
//************ IMPORTANT SETTINGS - YOU MUST CHANGE/CONFIGURE TO FIT YOUR HARDWARE ************
//*********************************************************************************************

#define FREQUENCY     RF69_433MHZ
//#define FREQUENCY     RF69_868MHZ
//#define FREQUENCY       RF69_915MHZ //Match this with the version of your Moteino! (others: RF69_433MHZ, RF69_868MHZ)
#define IS_RFM69HW_HCW  1//uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!
#include "Network_Config.h"

// for Feather 32u4 Radio
#define RFM69_CS      8
#define RFM69_IRQ     7
#define RFM69_IRQN    4  // Pin 7 is IRQ 4!
#define RFM69_RST     4

//*********************************************************************************************
#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI      -70
//*********************************************************************************************
#define SEND_LOOPS   75 //send data this many sleep loops (15 loops of 8sec cycles = 120sec ~ 2 minutes)
#define SLEEP_FASTEST SLEEP_15MS
#define SLEEP_FAST SLEEP_250MS
#define SLEEP_SEC SLEEP_1S
#define SLEEP_LONG SLEEP_2S
#define SLEEP_LONGER SLEEP_4S
#define SLEEP_LONGEST SLEEP_8S
period_t sleepTime = SLEEP_LONGEST; //period_t is an enum type defined in the LowPower library (LowPower.h)
//*********************************************************************************************

#define VBATPIN A9

#define BATT_LOW      3.4  //(volts)
#define BATT_READ_LOOPS  SEND_LOOPS*450  // read and report battery voltage every this many sleep cycles (ex 30cycles * 8sec sleep = 240sec/4min). For 450 cycles you would get ~1 hour intervals between readings
//*****************************************************************************************************************************

#define SHT31_VIN_PIN 6   // The sensor is wired up to Feather pin 6 -> SHT31-D VIN 

#define LED LED_BUILTIN

#define BLINK_EN                 //uncomment to blink LED on every send
// #define SERIAL_EN                //comment out if you don't want any serial output

#ifdef SERIAL_EN
  #define SERIAL_BAUD   115200
  #define DEBUG(input)   {Serial.print(input);}
  #define DEBUGln(input) {Serial.println(input);}
  #define SERIALFLUSH() {Serial.flush();}
#else
  #define DEBUG(input);
  #define DEBUGln(input);
  #define SERIALFLUSH();
#endif
//*****************************************************************************************************************************

#ifdef ENABLE_ATC
  RFM69_ATC radio = RFM69_ATC(RFM69_CS, RFM69_IRQ, IS_RFM69HW_HCW, RFM69_IRQN);
#else
  RFM69 radio = RFM69(RFM69_CS, RFM69_IRQ, IS_RFM69HW_HCW, RFM69_IRQN);
#endif

Adafruit_SHT31 sht31 = Adafruit_SHT31();

char buffer[100];

void setup(void)
{
#ifdef SERIAL_EN
  while (!Serial) {Blink(LED, 500); Blink(LED, 1500); } // wait until serial console is open, remove if not tethered to computer. 
  Serial.begin(SERIAL_BAUD);
#endif
  pinMode(LED, OUTPUT);
  pinMode(SHT31_VIN_PIN, OUTPUT);

  for(uint8_t i = 0; i < 5; i++){ 
    Blink(LED, 2000 + i*100);
  }
  DEBUGln("Started");

  pinMode(RFM69_RST, OUTPUT);
  digitalWrite(RFM69_RST, HIGH);
  delay(100);
  digitalWrite(RFM69_RST, LOW); 
  delay(100);
  
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
#ifdef IS_RFM69HW_HCW
  radio.setHighPower(); //must include this only for RFM69HW/HCW!
#endif
  radio.encrypt(ENCRYPTKEY);

//Auto Transmission Control - dials down transmit power to save battery (-100 is the noise floor, -90 is still pretty good)
//For indoor nodes that are pretty static and at pretty stable temperatures (like a MotionMote) -90dBm is quite safe
//For more variable nodes that can expect to move or experience larger temp drifts a lower margin like -70 to -80 would probably be better
//Always test your ATC mote in the edge cases in your own environment to ensure ATC will perform as you expect
#ifdef ENABLE_ATC
  radio.enableAutoPower(ATC_RSSI);
#endif

#ifdef SERIAL_EN
  sprintf(buffer, "WeatherMote - transmitting at: %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  DEBUGln(buffer);
#endif

  Wire.begin();
  Wire.setClock(400000); //Increase to fast I2C speed!

  digitalWrite(SHT31_VIN_PIN, HIGH);
  if (!sht31.begin(0x44)) {   // Set to 0x45 for alternate i2c addr
    Serial.println("Couldn't find SHT31");
    while (1) {Blink(LED, 30000);}
  }
  digitalWrite(SHT31_VIN_PIN, LOW);

  //DEBUGln("Sending....")
  //radio.sendWithRetry(GATEWAYID, "START", 6, 2);
  //Blink(LED, 3000);Blink(LED, 3000);Blink(LED, 3000);
  //DEBUGln("Sent...")
  
  for (uint8_t i=0; i<=A5; i++)
  {
    if (i == RF69_SPI_CS) continue;
    if (i == RFM69_IRQ) continue;
    if (i == RFM69_IRQN) continue;
    
    pinMode(i, OUTPUT);
    digitalWrite(i, LOW);
  }
  
  SERIALFLUSH();
  readBattery();
}

unsigned long doorPulseCount = 0;
char input=0;
byte sendLoops=0;
byte battReadLoops=0;
float batteryVolts = 5;
char* BATstr="BAT:5.00v"; //longest battery voltage reading message = 9chars

void loop()
{
  if (battReadLoops--<=0) //only read battery every BATT_READ_LOOPS cycles
  {
    readBattery();
    battReadLoops = BATT_READ_LOOPS-1;
  }
  
  if (sendLoops--<=0)   //send readings every SEND_LOOPS
  {
    sendLoops = SEND_LOOPS-1;

    digitalWrite(SHT31_VIN_PIN, HIGH); //Power the sensor
    while (!sht31.begin(0x44)) {
      delay(200);
    }

    //read SHT sensor
    float t = sht31.readTemperature();
    float h = sht31.readHumidity();
    digitalWrite(SHT31_VIN_PIN, LOW); // Shut off the sensor

    uint16_t bat = (uint16_t)(batteryVolts*100);
    uint16_t temp = (uint16_t)(t*100);
    uint16_t humidity = (uint16_t)(h*100);

    byte sendbuf[6];
    sendbuf[0] = bat >> 8 & 0xFF;
    sendbuf[1] = bat & 0xFF;
    sendbuf[2] = temp >> 8 & 0xFF;
    sendbuf[3] = temp & 0xFF;
    sendbuf[4] = humidity >> 8 & 0xFF;
    sendbuf[5] = humidity & 0xFF;

#ifdef SERIAL_EN    
    sprintf(buffer, "BAT=%s T=%d H=%d", BATstr, (uint16_t)(t*100), (uint16_t)(h*100));
    DEBUGln(buffer); 
#endif
  
    radio.sendWithRetry(GATEWAYID, sendbuf, 6, 1); //retry one time

    #ifdef BLINK_EN
      Blink(LED, 1500);
    #endif
  }
 
  SERIALFLUSH();
  radio.sleep(); //you can comment out this line if you want this node to listen for wireless programming requests

  LowPower.powerDown(sleepTime, ADC_OFF, BOD_OFF);
  DEBUGln("WAKEUP");
}

void readBattery()
{
  float readings=0;

  pinMode(VBATPIN, INPUT);
  for (byte i=0; i<5; i++) {
    float measuredvbat = analogRead(VBATPIN);
    measuredvbat *= 2;    // we divided by 2, so multiply back
    measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
    measuredvbat /= 1024; // convert to voltage
    readings += measuredvbat;
  }
   
  batteryVolts = readings / 5.0;
  sprintf(BATstr, "%d", (uint16_t)(batteryVolts*100));
  if (batteryVolts <= BATT_LOW) BATstr = "LOW";

  pinMode(VBATPIN, OUTPUT);
  digitalWrite(VBATPIN,LOW);
}

void Blink(byte pin, unsigned int delayMs)
{
  pinMode(pin, OUTPUT);
  digitalWrite(pin,HIGH);
  delay(delayMs/2);
  digitalWrite(pin,LOW);
  delay(delayMs/2);  
}
