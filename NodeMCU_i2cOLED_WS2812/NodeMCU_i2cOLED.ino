#include <ESP8266WiFi.h>
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"// Include the UI lib
#include "images.h"
#include <Adafruit_NeoPixel.h>

/****************System setup****************/
#define SERIAL_BAUDRATE 115200
#define CLK_PIN 14
#define DT_PIN 12
/****************WiFi setup****************/
#define ssid  "TZUWAE-X1C"
#define password "12345678"
#define WiFitimeout 20
#define TCPprocessTime 100
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
/****************VAR****************/
#define ISRTime 10000
int isrCounter = 0;
String dpline;
String overlayString = "";
unsigned long t = 0;
int RSSI;
/****************Flag****************/
bool isInit = false; 

// Initialize the OLED display using Wire library
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NEO_PIXELS, NEO_PIN, NEO_GRB + NEO_KHZ800);
SSD1306  display(0x3c, 5, 4);
OLEDDisplayUi ui     ( &display );
WiFiClient client;
FrameCallback frames[] = { drawFrame1, drawFrame2, drawFrame3, drawFrame4, drawFrame5 };
OverlayCallback overlays[] = { msOverlay };
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
	/****************starting WiFi connection****************/
  	Serial.println();
	Serial.print("Connecting to WiFi:");
	Serial.println(ssid);
	WiFi.begin(ssid, password);	
	pinMode(CLK_PIN, INPUT_PULLUP);
	pinMode(DT_PIN, INPUT_PULLUP);
	pinMode(13,INPUT);
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
	isInit=true;
}


void loop()
{
	int remainingTimeBudget = ui.update();
	RSSI=WiFi.RSSI();
	Serial.println(digitalRead(13));
	if (remainingTimeBudget > 0)
	{
		delay(remainingTimeBudget);
		isrCounter = isrCounter + remainingTimeBudget;		
		if(isrCounter >= ISRTime)
		{
			timeISR();
			isrCounter = 0;
		}

		if(Serial.available()>0)
		{
			SerialISR();
		}
	}
}

/****************ISR code****************/
void timeISR()
{
	colorWipe(strip.Color(10, 10, 10), 1);
	Serial.println("timer ISR");
	overlayString = "Updating...";
	ui.update();
	client.connect(host, port);
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
			Serial.print("Server:");
			Serial.println(line);
			client.stop();
			break;
		}
		break;
	}
	overlayString = "";
	colorWipe(strip.Color(100, 100, 100), 1);
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

void rotaryEncoderChanged(){ // when CLK_PIN is FALLING
  unsigned long temp = millis();
  /*
  if(temp - t < 30)
    return;
  t = temp;
  */
  if(digitalRead(DT_PIN) == HIGH)
	  ui.nextFrame();
  else
	  ui.previousFrame();
}


void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

/****************subroutine UI setup****************/
void displaySetup()
{
	ui.setTargetFPS(60);
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

void drawFrame3(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // Text alignment demo
  display->setFont(ArialMT_Plain_10);

  // The coordinates define the left starting point of the text
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->drawString(0 + x, 11 + y, "Left aligned (0,10)");

  // The coordinates define the center of the text
  display->setTextAlignment(TEXT_ALIGN_CENTER);
  display->drawString(64 + x, 22 + y, "Center aligned (64,22)");

  // The coordinates define the right end of the text
  display->setTextAlignment(TEXT_ALIGN_RIGHT);
  display->drawString(128 + x, 33 + y, "Right aligned (128,33)");
}

void drawFrame4(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // Demo for drawStringMaxWidth:
  // with the third parameter you can define the width after which words will be wrapped.
  // Currently only spaces and "-" are allowed for wrapping
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawStringMaxWidth(0 + x, 0 + y, 128, "Lorem ipsum\n dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore.");
}

void drawFrame5(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
	display->setFont(ArialMT_Plain_10);
	display->setTextAlignment(TEXT_ALIGN_CENTER);
	display->drawString(64 + x, 22 + y, dpline);
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