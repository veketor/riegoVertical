/*
 Name:		riegoVertical.ino
 Created:	18/05/2022 8:47:38
 Author:	veketor
*/

//WIfi and NTP (Network Time Protocol) includes
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
// #include <WiFiServerSecureBearSSL.h>
// #include <WiFiServerSecure.h>
// #include <WiFiServer.h>
// #include <WiFiClientSecureBearSSL.h>
// #include <WiFiClientSecure.h>
// #include <WiFiClient.h>
// #include <ESP8266WiFiType.h>
// #include <ESP8266WiFiSTA.h>
// #include <ESP8266WiFiScan.h>
// #include <ESP8266WiFiMulti.h>
// #include <ESP8266WiFiGratuitous.h>
// #include <ESP8266WiFiGeneric.h>
// #include <ESP8266WiFiAP.h>
// #include <CertStoreBearSSL.h>
// #include <BearSSLHelpers.h>
// #include <ArduinoWiFiServer.h>
/////  END NTP INCLUDE ///////////////


// Declaración de variables locales////////////////////
//Wifi configuration data
const char* ssid = "CENTRO";
const char* password = "";
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

//Constant
const int secondsPerMinute = 60;
const int secondsPerHour = 60 * secondsPerMinute;

//Time adjustment
int ntpUpdateGapInMilliseconds = 1000 * 30 * secondsPerMinute; //30 minutos
int summerTimeOffset = 2 * secondsPerHour;
int relayOpenTimeMinutes = 1 * secondsPerMinute; //Tiempo que está abierto en minutos
const int relayTimeBetweenInMinutes = 1 * secondsPerMinute; //Tiempo entre aperturas

//Time control variables
unsigned long lastTimeOpenRelay; //Ultima vez que se abrio el relay
unsigned long epochTime;
unsigned long epochWateringTimeMorning; //sobre las 12
unsigned long epochWateringTimeNight;// sobre las 9

//Relay variables
int relayPin = 5; //pinout in https://www.electronicwings.com/nodemcu/nodemcu-gpio-with-arduino-ide
bool isOpenRelay = false;
//////////////////////////////////////////////////////

void updateEpochTime()
{
	epochTime = timeClient.getEpochTime();
}

unsigned long getCurrentEpochTime()
{
	if (timeClient.isTimeSet())
	{
		return timeClient.getEpochTime();
	}
	else
	{
		return 0;
	}
}

bool iniNTP()
{
	Serial.println("Setting Up NTP Client");
	//timeClient.setPoolServerName("pool.ntp.org");
	timeClient.setUpdateInterval(ntpUpdateGapInMilliseconds);//Milliseconds; 180000/1000 = 180 seconds -> 3 min.
	timeClient.begin();
	timeClient.update();//force start.
	timeClient.setTimeOffset(summerTimeOffset);//Spain is +1 time offset
	timeClient.forceUpdate();
	if (timeClient.isTimeSet())
	{
		Serial.print("iniNTP OK time: ");
		Serial.println(timeClient.getFormattedTime());
	}
	else
	{
		Serial.println("iniNTP wrong.");
	}
	Serial.print("Hora: ");
	Serial.println(timeClient.getHours());
	return timeClient.isTimeSet();
}

bool iniNetworkConection()
{
	Serial.print("Connecting to ");
	Serial.println(ssid);
	WiFi.begin(ssid, password);
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}
	return true;//Always return true if find network.
}

// the setup function runs once when you press reset or power the board
void setup() 
{
	Serial.begin(115200);
	Serial.println("Setting Up RIEGO VERTICAL");
	pinMode(relayPin, OUTPUT);
	digitalWrite(relayPin, LOW);
	delay(1000);
	digitalWrite(relayPin, HIGH);
	digitalWrite(LED_BUILTIN, LOW);
	iniNetworkConection();
	iniNTP();
	lastTimeOpenRelay = timeClient.getEpochTime() - 30; //inciamos pasados 30 segundos en el arranque.
	pinMode(LED_BUILTIN, OUTPUT);
}
void setRelayOpen(bool relayStatus)
{
	Serial.print("Relay status: ");
	Serial.println(relayStatus);
	if (relayStatus == isOpenRelay)
	{
		Serial.print("Relay same status");
	}
	else
	{
		if (relayStatus == true)
		{
			Serial.println("Abrimos relay");
			digitalWrite(relayPin, LOW);//OPEN
		}
		else
		{
			Serial.println("Cerramos relay");
			digitalWrite(relayPin, HIGH);//CLOSE
		}
	}
	isOpenRelay = relayStatus;
}

void makeBlink()
{
	int epochTwoLastDigit = timeClient.getEpochTime() % 10;
	Serial.print("----------------------------------------Epoch blink counter: ");
	Serial.println(epochTwoLastDigit);
	if (epochTwoLastDigit == 0)
	{
		digitalWrite(LED_BUILTIN, HIGH);
		delay(200);
		digitalWrite(LED_BUILTIN, LOW);
		delay(200);
		digitalWrite(LED_BUILTIN, HIGH);
		delay(200);
		digitalWrite(LED_BUILTIN, LOW);
	}
}

void makeBlinkFast()
{
	digitalWrite(LED_BUILTIN, HIGH);
	delay(100);
	digitalWrite(LED_BUILTIN, LOW);
	delay(100);
}

// the loop function runs over and over again until power down or reset
void loop() 
{
	timeClient.update(); //Only called every ntpUpdateGapInMilliseconds
	updateEpochTime();
	if (epochTime > 0)
	{
		unsigned long nextOpenRelayTime = lastTimeOpenRelay + relayTimeBetweenInMinutes;

		if (epochTime >= nextOpenRelayTime )
		{
			unsigned long closeRelayAt = nextOpenRelayTime + relayOpenTimeMinutes;
			unsigned long timeToCloseInSeconds = closeRelayAt - epochTime;
			Serial.print("Relay control segment, close at: ");
			Serial.println(timeToCloseInSeconds);

			if (isOpenRelay == false)
			{
				setRelayOpen(true);
			}
			if (epochTime >= closeRelayAt)
			{
				lastTimeOpenRelay = timeClient.getEpochTime();
				setRelayOpen(false);
			}

			//DEbug info only
			if (isOpenRelay)
			{
				Serial.println(">>>>>> Regando");
				makeBlinkFast();
			}
			else
			{
				Serial.println(">>>>>> Esperando");
			}
			//
		}
		else
		{
			//Debug Info only
			unsigned long timeToOpen = nextOpenRelayTime - epochTime;
			Serial.print("Relay will open in: ");
			Serial.print(timeToOpen);
			Serial.println(" seconds.");
			makeBlink();
		}
		
	}
	delay(1000);
}
