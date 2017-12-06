
#include <ESP8266WiFi.h>
/****************WiFi setup****************/
#define ssid  "TZUWAE-X1C"
#define password "12345678"
#define WiFitimeout 10
/****************server setup****************/
#define port  12345			// target server TCP socket port
#define host  "192.168.137.1"	// target server ip or dns
#define serverretry 5

WiFiClient client;

void setup() {

	pinMode(LED_BUILTIN, OUTPUT); 
	digitalWrite(LED_BUILTIN, HIGH); 

	Serial.begin(115200);
	delay(10);

	/****************starting WiFi connection****************/
	Serial.println();
	Serial.print("Connecting to WiFi:");
	Serial.println(ssid);
	WiFi.begin(ssid, password);

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
			Serial.println("");
			Serial.println("WiFi connection failed");
		}
	}
}


 void loop() 
 {	
	 Serial.println("Main loop routine");
	 delay(1000);
 }
