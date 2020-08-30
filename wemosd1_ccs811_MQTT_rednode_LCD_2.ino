/*
 Ο κώδικας είναι βασισμένος στο 
 βασικό παράδειγμα λειτουργίας του ccs811 της εταιρίας Sparkfun.
 Ενώ οι πηγές για το ESP, MQTT, redNode περιγράφονται στο : http://randomnerdtutorials.com/

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include "DHT.h"
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>

#include "SparkFunCCS811.h" //Click here to get the library: http://librarymanager/All#SparkFun_CCS811
//#define CCS811_ADDR 0x5B //Default I2C Address
#define CCS811_ADDR 0x5A //Alternate I2C Address
CCS811 cc811Sensor(CCS811_ADDR);

#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h> // Library for LCD
// Καλωδίωση: Ο ακροδέκτης SDA συνδέεται με τον ακροδέκτη A4 και τον ακροδέκτη SCL στο A5 (Wiring: SDA pin is connected to A4 and SCL pin to A5) 
// Σύνδεση στο LCD μέσω I2C, προεπιλεγμένη διεύθυνση 0x27 (A0-A2 δεν έχουν βραχυκυκλωθεί) (Connect to LCD via I2C, default address 0x27 (A0-A2 not jumpered))

LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27,16,2);    // Άλλαξε σε (0x27,16,2) για 16x2 LCD.(Change to (0x27,16,2) for 16x2 LCD.)

#define WIFI_SSID "XXXXXX"
#define WIFI_PASSWORD "XXXXXXXXXX"

// Raspberri Pi Mosquitto MQTT Broker
#define MQTT_HOST IPAddress(192, 168, 1, XXXXXX )
// For a cloud MQTT broker, type the domain name
//#define MQTT_HOST "example.com"
#define MQTT_PORT 1883

// MQTT Topics
#define MQTT_PUB_TEMP "room/temperature"
#define MQTT_PUB_HUM "room/humidity"
#define MQTT_PUB_CO2 "room/CO2"
#define MQTT_PUB_tVOC "room/tVOC"

// Digital pin connected to the DHT sensor
#define DHTPIN 0

// Uncomment whatever DHT sensor type you're using
//#define DHTTYPE DHT11   // DHT 11
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
//#define DHTTYPE DHT21   // DHT 21 (AM2301)   

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Variables to hold sensor readings
float temp;
float hum;

//Initialize MQTT client
AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

unsigned long previousMillis = 0;   // Stores last time temperature was published
const long interval = 10000;        // Interval at which to publish sensor readings

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void onWifiConnect(const WiFiEventStationModeGotIP& event) {
  Serial.println("Connected to Wi-Fi.");
  connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected& event) {
  Serial.println("Disconnected from Wi-Fi.");
  mqttReconnectTimer.detach(); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
  wifiReconnectTimer.once(2, connectToWifi);
}

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  mqttClient.connect();
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(2, connectToMqtt);
  }
}

/*void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}*/

void onMqttPublish(uint16_t packetId) {
  Serial.print("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  lcd.begin();
  lcd.clear();
  lcd.backlight();
    
  dht.begin();
  
  wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
  wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  //mqttClient.onSubscribe(onMqttSubscribe);
  //mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onPublish(onMqttPublish);
  mqttClient.setServer(MQTT_HOST, MQTT_PORT);
  // If your broker requires authentication (username and password), set them below
  //mqttClient.setCredentials("REPlACE_WITH_YOUR_USER", "REPLACE_WITH_YOUR_PASSWORD");
  
  connectToWifi();

   //Initialize I2C Hardware
   Wire.begin(); 
  
  //Initialize - Check CCS811 Sensor
  if (cc811Sensor.begin() == false)
  {
    Serial.print("CCS811 error. Please check wiring. Freezing...");
    while (1);
  }
  }

  void loop() {
  unsigned long currentMillis = millis();
  // Every X number of seconds (interval = 10 seconds) 
  // it publishes a new MQTT message
  if (currentMillis - previousMillis >= interval) {
    // Save the last time a new reading was published
    previousMillis = currentMillis;
    // New DHT sensor readings
    hum = dht.readHumidity();
    // Read temperature as Celsius (the default)
    temp = dht.readTemperature();
    // Read temperature as Fahrenheit (isFahrenheit = true)
    //temp = dht.readTemperature(true);

    static char temperatureTemp[7];
    dtostrf(temp, 6, 2, temperatureTemp);

    static char humidityTemp[7];
    dtostrf(hum, 6, 2, humidityTemp);
  
    // Publish an MQTT message on topic room/temperature
    uint16_t packetIdPub1 = mqttClient.publish(MQTT_PUB_TEMP, 1, true, String(temperatureTemp).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_TEMP, packetIdPub1);
    Serial.printf("Message: %.2f \n", temp);

    // Publish an MQTT message on topic room/humidity
    uint16_t packetIdPub2 = mqttClient.publish(MQTT_PUB_HUM, 1, true, String(humidityTemp).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId %i: ", MQTT_PUB_HUM, packetIdPub2);
    Serial.printf("Message: %.2f \n", hum);
   if (cc811Sensor.dataAvailable())
  {
    //If so, have the sensor read and calculate the results.
    //Get them later
    cc811Sensor.readAlgorithmResults();
    
    float co2 = cc811Sensor.getCO2();
    static char co2Mes[7];
    dtostrf(co2, 6, 2, co2Mes); 
    float tVOC = cc811Sensor.getTVOC();
    static char TVOCMes[7];
    dtostrf(tVOC, 6, 2, TVOCMes);
   
   // Publish an MQTT message on topic room/CO2
    uint16_t packetIdPub3 = mqttClient.publish(MQTT_PUB_CO2, 1, true, String(co2Mes).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_CO2, packetIdPub3);
    Serial.printf("Message: %.2f \n", co2);

     // Publish an MQTT message on topic room/tvoc
    uint16_t packetIdPub4 = mqttClient.publish(MQTT_PUB_tVOC, 1, true, String(TVOCMes).c_str());                            
    Serial.printf("Publishing on topic %s at QoS 1, packetId: %i ", MQTT_PUB_tVOC, packetIdPub4);
    Serial.printf("Message: %.2f \n", tVOC);
    
    //monitor total publish
    Serial.println();
    Serial.print("CO2[");
    //Returns calculated CO2 reading
    Serial.print(cc811Sensor.getCO2());
    Serial.print("] tVOC[");
    //Returns calculated TVOC reading
    Serial.print(cc811Sensor.getTVOC());
    Serial.print("]");
    //print on serial monitor 
    Serial.print(" %\t Temperature: ");
    Serial.print(temp);
    Serial.print("   Humidity: ");
    Serial.print(hum);
    Serial.print(" *C ");
    Serial.println();


  
   // Print on LCD :
  lcd.backlight();
  lcd.setCursor(0, 0); // Set the cursor to a specific position.
  lcd.print("T:"); // Print string. 
  lcd.setCursor(2, 0); // Set the cursor to a specific position.
  lcd.print(temp,1);
  lcd.print(" H:");
  lcd.setCursor(9, 0); // Set the cursor to a specific position.
  lcd.print(hum,1);
  lcd.print("%  ");
  lcd.setCursor(0, 1); // Set the cursor to a specific position.
  lcd.print("CO2:");
  lcd.setCursor(4, 1); // Set the cursor to a specific position.
  lcd.print(co2, 0);
  lcd.print("ppm");
  lcd.setCursor(11, 1); // Set the cursor to a specific position.
  lcd.print(tVOC, 0);
  lcd.print("voc");

    
  }
    
  
  
  }
}
