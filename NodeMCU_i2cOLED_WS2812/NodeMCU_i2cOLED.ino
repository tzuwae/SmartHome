#include <ESP8266WiFi.h>
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306.h" // alias for `#include "SSD1306Wire.h"
#include "OLEDDisplayUi.h"// Include the UI lib
#include "images.h"
#include <Adafruit_NeoPixel.h>

/****************System setup****************/
#define SERIAL_BAUDRATE 115200
#define CLK_PIN 14 // 定義連接腳位
#define DT_PIN 12
/****************WiFi setup****************/
#define ssid  "TZUWAE-X1C"
#define password "12345678"
#define WiFitimeout 20
/****************server setup****************/
#define port  12345			// target server TCP socket port
#define host  "192.168.137.1"	// target server ip or dns
#define serverretry 5
/****************UI setup****************/
#define frameCount 5
/****************VAR****************/
#define ISRTime 5000
bool isAthorized = false;
bool isInit = false; 
int isrCounter = 0;
String dpline;
unsigned long t = 0;

// Initialize the OLED display using Wire library
Adafruit_NeoPixel strip = Adafruit_NeoPixel(14, 2, NEO_GRB + NEO_KHZ800);
SSD1306  display(0x3c, 5, 4);
OLEDDisplayUi ui     ( &display );
WiFiClient client;
FrameCallback frames[] = { drawFrame1, drawFrame2, drawFrame3, drawFrame4, drawFrame5 };

void setup() {
	Serial.begin(SERIAL_BAUDRATE);
	Serial.println();
	displaySetup();
	display.displayOn();
	/****************starting WiFi connection****************/
  	Serial.println();
	Serial.print("Connecting to WiFi:");
	Serial.println(ssid);
	WiFi.begin(ssid, password);

	attachInterrupt(CLK_PIN, rotaryEncoderChanged, FALLING);
	pinMode(CLK_PIN, INPUT_PULLUP);
	pinMode(DT_PIN, INPUT_PULLUP);

	for(int i=0;i<WiFitimeout;i++)
	{		
		Serial.print(".");
		if(WiFi.status() == WL_CONNECTED)
		{
			Serial.println("");
			Serial.println("WiFi connected");  
			Serial.print("Local IP address:");
			Serial.println(WiFi.localIP());
			Serial.print("connecting to Server:");
			Serial.println(host);
			strip.begin();
			colorWipe(strip.Color(0, 0, 100), 40); // Red
			colorWipe(strip.Color(0, 100, 0), 40); // Green
			colorWipe(strip.Color(100, 0, 0), 40); // Blue
			colorWipe(strip.Color(10, 10, 10), 40);
			
			for(int a=0;a<serverretry;a++)
			{
				if (!client.connect(host, port))
				{
					Serial.println("connection failed, try again...");
					continue;
				}
				Serial.println("Server connection successful!");
				client.println(WiFi.macAddress());
				delay(200);

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

		delay(1000);
		if(i==WiFitimeout-1)
		{
			Serial.println("WIFI ERROR!");
		}
	}
}


void loop()
{
	int remainingTimeBudget = ui.update();

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
	Serial.println("timer ISR");
	for(int a=0;a<serverretry;a++)
			{
				if (!client.connect(host, port))
				{
					Serial.println("connection failed, try again...");
					continue;
				}
				Serial.println("Server connection successful!");
				client.println(WiFi.macAddress());
				delay(50);

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
  if(temp - t < 30) // 去彈跳
    return;
  t = temp;  
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
	ui.setFrames(frames,frameCount);
	ui.init();
	display.flipScreenVertically();
}

/****************subroutine UI Update****************/

void drawFrame1(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // draw an xbm image.
  // Please note that everything that should be transitioned
  // needs to be drawn relative to x and y

  display->drawXbm(x + 34, y + 14, WiFi_Logo_width, WiFi_Logo_height, WiFi_Logo_bits);
}

void drawFrame2(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y) {
  // Demonstrates the 3 included default sizes. The fonts come from SSD1306Fonts.h file
  // Besides the default fonts there will be a program to convert TrueType fonts into this format
  display->setTextAlignment(TEXT_ALIGN_LEFT);
  display->setFont(ArialMT_Plain_10);
  display->drawString(0 + x, 10 + y, "Arial 10");

  display->setFont(ArialMT_Plain_16);
  display->drawString(0 + x, 20 + y, "Arial 16");

  display->setFont(ArialMT_Plain_24);
  display->drawString(0 + x, 34 + y, "Arial 24");
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
  display->drawStringMaxWidth(0 + x, 20 + y, 128, "Lorem ipsum\n dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore.");
}

void drawFrame5(OLEDDisplay *display, OLEDDisplayUiState* state, int16_t x, int16_t y)
{
	display->setFont(ArialMT_Plain_10);
	display->setTextAlignment(TEXT_ALIGN_CENTER);
	display->drawString(64 + x, 22 + y, dpline);
}


/****************NeoPixel subroutine************/
