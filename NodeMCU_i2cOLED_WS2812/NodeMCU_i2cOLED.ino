#include <ESP8266WiFi.h>
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"// Include the UI lib
#include "images.h"
#include <Adafruit_NeoPixel.h>
#include "DHT.h"

/****************System setup****************/
#define SERIAL_BAUDRATE 115200
#define CLK_PIN 14
#define DT_PIN 12
#define PB 0
/****************WiFi setup****************/
#define ssid  "TZUWAE-X1C"
#define password "12345678"
#define WiFitimeout 20
#define TCPprocessTime 500
#define WiFiChecktime 4
bool wifiConnect = false;
/****************server setup****************/
#define port  12345			// target server TCP socket port
#define host  "192.168.137.1"	// target server ip or dns
#define serverretry 5
/****************UI setup****************/
#define frameCount 5
/****************NEO Pixel setup****************/
#define NEO_PIN	2
#define NEO_PIXELS	12
/****************DHT11 setup****************/
#define DHTPIN 13
/****************VAR****************/
#define ISRTime 10000
int isrCounter = 0;
String dpline;
String overlayString = "";
unsigned long t = 0;
int RSSI;
float temperature;
float humidity;
float fahrenheit;
float hif;
float hic;
int lightIntensity = 0;
int PrevIntensity = 0;
/****************Flag****************/
bool isInit = false; 
bool PrevPBState;
bool Light = false;

// Initialize the OLED display using Wire library
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NEO_PIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);
SSD1306  display(0x3c, 5, 4);
OLEDDisplayUi ui     ( &display );
WiFiClient client;
FrameCallback frames[] = { drawFrame1, drawFrame2, drawFrame3, drawFrame4, drawFrame5 };
OverlayCallback overlays[] = { msOverlay };
DHT dht(DHTPIN, DHT11);
int overlaysCount = 1;
uint16_t a, j;




void setup() {
	Serial.begin(SERIAL_BAUDRATE);
	Serial.println();
	displaySetup();
	display.displayOn();
	ui.switchToFrame(0);
	ui.update();
	ui.switchToFrame(0);
	strip.begin();
	dht.begin();
	/****************starting WiFi connection****************/
  	Serial.println();
	Serial.print("Connecting to WiFi:");
	Serial.println(ssid);
	WiFi.begin(ssid, password);	
	pinMode(CLK_PIN, INPUT_PULLUP);
	pinMode(DT_PIN, INPUT_PULLUP);
	pinMode(PB, INPUT_PULLUP);
	for(long i=0;i<WiFitimeout*(1000/WiFiChecktime);i++)
	{		
		Serial.print(".");
		ui.update();
		if(WiFi.status() == WL_CONNECTED)
		{
			Serial.println("");
			Serial.println("WiFi connected");  
			Serial.print("Local IP address:");
			Serial.println(WiFi.localIP());
			Serial.print("connecting to Server:");
			Serial.println(host);
			ui.switchToFrame(0);
			ui.update();
			ui.switchToFrame(0);
			colorWipe(strip.Color(0, 0, 50), 25); // Blue
			colorWipe(strip.Color(0, 50, 0), 25); // Green
			colorWipe(strip.Color(100, 100, 100), 25);
			
			for(int a=0;a<serverretry;a++)
			{
				if (!client.connect(host, port))
				{
					Serial.println("connection failed, try again...");
					continue;
				}
				Serial.println("Server connection successful!");
				client.println(WiFi.macAddress());
				delay(TCPprocessTime);

				if (client.available() > 0)
				{
					String line = client.readStringUntil('\r');
					dpline=line;
					if(line=="")
						break;
					Serial.print("Server:");
					Serial.println(line);
					client.stop();
					break;
				}
				break;
			}
			break;
		}

		delay(WiFiChecktime);
		for(a=0; a< strip.numPixels(); a++) 
		{
			strip.setPixelColor(a, Wheel((( a* 256 / strip.numPixels()) + j%256) & 255));
		}
		strip.show();
		j++;
		if(i==WiFitimeout*(1000/WiFiChecktime)-1)
		{
			Serial.println("WIFI ERROR!");
			colorWipe(strip.Color(100, 0, 0), 25); // Red
		}
	}
	attachInterrupt(CLK_PIN, rotaryEncoderChanged, FALLING);
	attachInterrupt(PB, PB_Push, FALLING);
	isInit=true;
}


void loop()
{
	int remainingTimeBudget = ui.update();
	RSSI=WiFi.RSSI();
	colorWipe(strip.Color(map(lightIntensity, 0, 100, 0, 255), map(lightIntensity, 0, 100, 0, 255), map(lightIntensity, 0, 100, 0, 255)), 0);
	if (remainingTimeBudget > 0)
	{
		//PrevPBState = digitalRead(PB);
		delay(remainingTimeBudget);
		isrCounter = isrCounter + remainingTimeBudget;		
		if(isrCounter >= ISRTime)
		{
			timeISR();
			isrCounter = 0;
		}		
	}
	if(Serial.available()>0)
		{
			SerialISR();
		}
	/*
	if((digitalRead(PB)==0)&&(PrevPBState==1))
		{
			PB_Push();
		}
		*/
}

/****************ISR code****************/
void timeISR()
{
	Serial.println("timer ISR");
	overlayString = "Updating...";
	ui.update();
	readDHT11();
	for(int a=0;a<serverretry;a++)
	{
		client.connect(host, port);
		if (!client.connect(host, port))
		{
			Serial.println("connection failed, try again...");
			overlayString = "Server Error";
			return;
		}
		Serial.println("Server connection successful!");
		client.println(WiFi.macAddress());
		delay(TCPprocessTime);
		if (client.available() > 0)
		{
			String line = client.readStringUntil('\r');
			dpline=line;
			if(line=="")
			{
				Serial.println("Error, Unable to read string!");
				break;
			}
			Serial.print("Server:");
			Serial.println(line);
			client.stop();
			break;
		}
		break;
	}
	overlayString = "";
}

void SerialISR()
{
	int a = Serial.read();		
	Serial.println(a);
	if(a==49)
	{
		Serial.println("Next Page");
		ui.nextFrame();
	}
	if(a==50)
	{
		Serial.println("Previous Page");
		ui.previousFrame();
	}
	if(a==51)
	{
		Serial.println("swithc to Frame");
	//ui.switchToFrame(2);
		dpline = "hello";
	}
}

void rotaryEncoderChanged()// when CLK_PIN is FALLING
{
	
	
	if(!Light)
	{
		if(digitalRead(DT_PIN) == HIGH)
		{
			ui.nextFrame();
		}
		else
		{
			ui.previousFrame();
		}
	}
	if(Light)
	{
		unsigned long temp = millis();
		if(temp - t < 30)
		return;
		t = temp;
		if(digitalRead(DT_PIN) == HIGH)
		{
			if(lightIntensity>0)
			{
				lightIntensity=lightIntensity-5;
			}
		}
		else
		{
			if(lightIntensity<100)
			{
				lightIntensity=lightIntensity+5;
			}
		}
		Serial.println(lightIntensity);
	}
}

void PB_Push()
{
	unsigned long temp = millis();
	if(temp - t < 30)
		return;
	t = temp;
	if(!Light)
	{
		Light = true;
		Serial.println("Light=True");
		ui.switchToFrame(5);
	}
	else
	{
		Light = false;
		Serial.println("Light=False");
	}
}

void readDHT11()
{
  humidity = dht.readHumidity();
  // Read temperature as Celsius (the default)
  temperature = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  fahrenheit = dht.readTemperature(true);

  // Check if any reads failed and exit early (to try again).
  if (isnan(humidity) || isnan(temperature) || isnan(fahrenheit)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  // Compute heat index in Fahrenheit (the default)
  hif = dht.computeHeatIndex(fahrenheit, humidity);
  // Compute heat index in Celsius (isFahreheit = false)
  hic = dht.computeHeatIndex(temperature, humidity, false);

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print(" %\t");
  Serial.print("Heat index: ");
  Serial.print(hic);
  Serial.println(" *C ");
}


/****************subroutine UI setup****************/
void displaySetup()
{
	ui.setTargetFPS(100);
	ui.setTimePerTransition(80);
	ui.disableAutoTransition();
	ui.setActiveSymbol(activeSymbol);
	ui.setInactiveSymbol(inactiveSymbol);
	ui.setIndicatorDirection(LEFT_RIGHT);
	ui.setFrameAnimation(SLIDE_LEFT);
	ui.setOverlays(overlays, overlaysCount);
	ui.setFrames(frames,frameCount);
	ui.init();
	display.flipScreenVertically();
}

/****************subroutine UI Update****************/

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // draw an xbm image.
  // Please note that everything that should be transitioned
  // needs to be drawn relative to x and y
	if(WiFi.status() != WL_CONNECTED)
	{
		display->drawXbm(x + 0, y + 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits); //x+34
		display->drawXbm(x + 50, y + 4, 80, 50, blackman);
	}
	else
	{
		display->drawXbm(x + 0, y + 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits); //x+34
		
		if(isInit)
		{
			if(RSSI>=-67)
			{
				display->drawXbm(x + 75, y + 10, 40, 40, wifi_4);
			}
			if(RSSI<-67 && RSSI >=-75)
			{
				display->drawXbm(x + 75, y + 10, 40, 40, wifi_3);
			}
			if(RSSI<-75 && RSSI >=-85)
			{
				display->drawXbm(x + 75, y + 10, 40, 40, wifi_2);
			}
			if(RSSI<-85)
			{
				display->drawXbm(x + 75, y + 10, 40, 40, wifi_1);
			}
		}
		else
		{
			display->drawXbm(x + 50, y + 4, 80, 50, okICON);
		}
	}
	
  
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) 
{
  // Demonstrates the 3 included default sizes. The fonts come from SSD1306Fonts.h file
  // Besides the default fonts there will be a program to convert TrueType fonts into this format
	/*
	display->setTextAlignment(TEXT_ALIGN_LEFT);
	display->setFont(ArialMT_Plain_10);
	display->drawString(0 + x, 10 + y, "Arial 10");
	display->setFont(ArialMT_Plain_16);
	display->drawString(0 + x, 20 + y, "Arial 16");
	display->setFont(ArialMT_Plain_24);
	display->drawString(0 + x, 34 + y, "Arial 24");
	*/
	display->setFont(ArialMT_Plain_24);
	display->setTextAlignment(TEXT_ALIGN_CENTER);
	display->drawString(64 + x, 5 + y, dpline.substring(0,8));
	display->setFont(ArialMT_Plain_16);
	display->setTextAlignment(TEXT_ALIGN_CENTER);
	display->drawString(64 + x, 32 + y, dpline.substring(8,23));

}

void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) 
{
	display->drawXbm(x -5, y + 3, thermometer_width, thermometer_height, thermometer_bits);
	display->setFont(ArialMT_Plain_24);
	display->drawString(120 + x, 15 + y, String(String(hic,1)+"*C"));
	//display->drawXbm(x+90, y + 10, cel_width, cel_height, cel_bits);
}

void drawFrame4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) 
{
	display->drawXbm(x + 5, y + 10, hygrometer_width, hygrometer_height, hygrometer_bits);
	display->setFont(ArialMT_Plain_24);
	display->drawString(120 + x, 15 + y, String(String(humidity,1)+" %"));
}

void drawFrame5(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
	if(!Light)
	{
		display->drawXbm(x + 5, y + 10, lightball_width, lightball_height, lightball_bits);
		display->setFont(ArialMT_Plain_24);
		display->drawString(120 + x, 15 + y, String(lightIntensity)+" %");
	}
	else
	{
		display->setTextAlignment(TEXT_ALIGN_CENTER);
		display->setFont(ArialMT_Plain_24);
		display->drawString(64 + x, 15 + y, String(lightIntensity)+" %");
	}
}

void msOverlay(OLEDDisplay *display, OLEDDisplayUiState* state) 
{
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(128, 0, overlayString);
}

/****************NeoPixel subroutine************/
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void colorWipe(uint32_t c, uint8_t wait) 
{
	for(uint16_t i=0; i<strip.numPixels(); i++) 
	{
		strip.setPixelColor(i, c);
		strip.show();
		delay(wait);
	}
}