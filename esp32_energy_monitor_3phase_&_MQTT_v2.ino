/*
********
Programming Stamatis Iliadakis
based https://learn.openenergymonitor.org/electricity-monitoring/ct-sensors/how-to-build-an-arduino-energy-monitor-measuring-current-only
and on Rui Santos projects repository at https://randomnerdtutorials.com  
********
*/

#include <WiFi.h>
#include <PubSubClient.h>

// Replace the next variables with your SSID/Password combination
const char* ssid = "xxxxx";
const char* password = "xxxxxxxxx";

// Add your MQTT Broker IP address, example:
const char* mqtt_server = "192.168.1.xxxxx";

WiFiClient espClient2;
PubSubClient client(espClient2);
long lastMsg = 0;
char msg[50];
int value = 0;
float Irms1 = 0;  
float Irms2 = 0;  
float Irms3 = 0;  

char Irms1Mes[8];
char Irms2Mes[8];
char Irms3Mes[8];

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;

// LED Pin
const int lamp = 4;

// EmonLibrary examples openenergymonitor.org, Licence GNU GPL V3

#include "EmonLib.h"                   // Include Emon Library
EnergyMonitor emon1;                   // Create an instance
EnergyMonitor emon2;                   // Create an instance
EnergyMonitor emon3;                   // Create an instance

/* I2C LCD with Arduino example code. More info: https://www.makerguides.com */
// Include the libraries:
// LiquidCrystal_I2C.h: https://github.com/johnrickman/LiquidCrystal_I2C
//#include <Wire.h> // Library for I2C communication
#include <LiquidCrystal_I2C.h> // Library for LCD
// Wiring: SDA pin is connected to A4 and SCL pin to A5.
// Connect to LCD via I2C, default address 0x27 (A0-A2 not jumpered)
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);    // Change to (0x27,16,2) for 16x2 LCD.
//#include <math.h>

void setup() {
  analogReadResolution(10);
  Serial.begin(115200);
  emon1.current(32, 90.9);             // Current: input pin, calibration.
  emon2.current(33, 90.9);             // Current: input pin, calibration.
  emon3.current(34, 90.9);             // Current: input pin, calibration.

  // Initiate the LCD:
  //lcd.init();
  lcd.begin();
  lcd.backlight();
  // clear the screen
  lcd.clear();
  
  pinMode(lamp, OUTPUT);
  
  Serial.begin(115200);
    
  setup_wifi();
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  lcd.setCursor(1, 0); // Set the cursor on the first column and first row.
  lcd.print("IP :"); // Print the string "Hello World!"
  lcd.setCursor(1, 1); // Set the cursor on the first column and first row.
  lcd.print(WiFi.localIP());
  lcd.print(" ");
  delay(1000);

  
}


// This functions is executed when some device publishes a message to a topic that your ESP8266/ESP32 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that 
// your ESP8266/ESP32  is subscribed you can actually do something
// Η συνάρτηση callback εκτελείται όταν κάποια συσκευή δημοσιεύει ένα μήνυμα σε ένα θέμα στο οποίο έχει εγγραφεί το ESP8266/ESP32 
// Αλλάξτε τη λειτουργία παρακάτω για να προσθέσετε λογική στο πρόγραμμα σας, έτσι όταν μια συσκευή δημοσιεύει ένα μήνυμα σε ένα θέμα που
// το ESP8266/ESP32  σας έχει εγγραφεί, μπορείτε πραγματικά να κάνετε κάτι

void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT
  // If a message is received on the topic room/lamp, you check if the message is either on or off. Turns the lamp GPIO according to the message
  //Προσθέσετε ελεύθερα περισσότερες προτάσεις εάν θέλετε να ελέγξετε περισσότερα GPIO με MQTT
  // Αν ληφθεί ένα μήνυμα σχετικά με το topic room/lamp, ελένξετε εάν το μήνυμα είναι είτε ενεργοποιημένο είτε απενεργοποιημένο. 
  // Ενεργοποιεί τον λαμπτήρα GPIO σύμφωνα με το μήνυμα.
    
  if(topic=="room/lamp"){
      Serial.print("Changing Room lamp to ");
      if(messageTemp == "on"){
        digitalWrite(lamp, HIGH);
        Serial.print("On");
      }
      else if(messageTemp == "off"){
        digitalWrite(lamp, LOW);
        Serial.print("Off");
      }
  }
  Serial.println();
}


// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266 
// Αυτή η λειτουργία επανασυνδέει το ESP8266 με τον MQTT broker
// Άλλαξε τη λειτουργία παρακάτω, εάν θέλεις να εγγραφείτε σε περισσότερα θέματα με το ESP8266 σας
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    /*
     YOU MIGHT NEED TO CHANGE THIS LINE, IF YOU'RE HAVING PROBLEMS WITH MQTT MULTIPLE CONNECTIONS
     To change the ESP device ID, you will have to give a new name to the ESP8266.
     Here's how it looks:
       if (client.connect("ESP8266Client")) {
     You can do it like this:
       if (client.connect("ESP1_Office")) {
     Then, for the other ESP:
       if (client.connect("ESP2_Garage")) {
      That should solve your MQTT multiple connections problem
    */
    /*
    // Προσπάθεια σύνδεσης
    //  ΜΠΟΡΕΙΤΕ ΝΑ ΧΡΗΣΙΜΟΠΟΙΗΣΕΤΕ ΤΗΝ ΑΚΟΛΟΥΘΗ ΓΡΑΜΜΗ, ΕΑΝ ΕΧΕΤΕ ΠΡΟΒΛΗΜΑΤΑ ΜΕ ΠΟΛΛΑΠΛΕΣ ΣΥΝΔΕΣΕΙΣ MQTT
    //  Για να αλλάξετε το αναγνωριστικό συσκευής ESP, θα πρέπει να δώσετε ένα νέο όνομα στο ESP8266.
    //  Δείτε πώς φαίνεται:
    //  εάν (client.connect ("ESP8266Client")) {
    //  Μπορείτε να το κάνετε όπως παρακάτω:
    //  αν (client.connect ("ESP1_Office")) {
    //  Στη συνέχεια, για το άλλο ESP:
    //  εάν (client.connect ("ESP2_Garage")) {
    //  Αυτό θα λύσει το πρόβλημα πολλαπλών συνδέσεων MQTT
    */
    if (client.connect("Line X Client")) {
      Serial.println("connected");  
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      // Εγγραφείτε ή επαναλάβετε την εγγραφή σε ένα θέμα 
      // Μπορείτε να εγγραφείτε σε περισσότερα θέματα (για να ελέγξετε περισσότερες LED σε αυτό το παράδειγμα)
      client.subscribe("room/Line1");
      client.subscribe("room/Line2");
      client.subscribe("room/Line3");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 3 seconds before retrying  - Περίμενε πέντε δευτερόλεπτα πριν ξαναπροσπαθήσεις 
      delay(3000);
    }
  }
}

void loop() {
  
  if (!client.connected()) {
    reconnect();
  }

 if(!client.loop())
    client.connect("Electrical Line Client");

  now = millis();
  // Δημοσιεύει νέα μέτρηση, κάθε 20 δευτερόλεπτα
  if (now - lastMeasure > 10000) {
    lastMeasure = now;
  
     Irms1 = emon1.calcIrms(1480);  // Calculate Irms only
    //static char Irms1Mes[8];
    dtostrf(Irms1,4,1,Irms1Mes); 
    client.publish("room/Line1", Irms1Mes);
    Irms2 = emon2.calcIrms(1480);  // Calculate Irms only
    //static char Irms2Mes[7];
    dtostrf(Irms2, 4, 1, Irms2Mes);
    client.publish("room/Line2", Irms2Mes);
    Irms3 = emon3.calcIrms(1480);  // Calculate Irms only
    //static char Irms3Mes[7];
    dtostrf(Irms3, 4, 1, Irms3Mes);
    client.publish("room/Line3", Irms3Mes);
    }
    monitor_function();
}

void monitor_function(){
    
  float P1 = Irms1 * 240;
  float P2 = Irms2 * 240;
  float P3 = Irms3 * 240;
  int x = round(P1);
  int y = round(P2);
  int z = round(P3);


  Serial.print("Irms1=");
  Serial.print(Irms1, 1);             // Irms
  Serial.print("A, P1=");
  Serial.print(round(P1), 1);        // Apparent power
  Serial.print("W, ");

  Serial.print("Irms2=");
  Serial.print(Irms2, 1);             // Irms
  Serial.print("A, P2=");
  Serial.print(round(P2), 1);        // Apparent power
  Serial.print("W, ");

  Serial.print("Irms3=");
  Serial.print(Irms3, 1);             // Irms
  Serial.print("A, P3=");
  Serial.print(round(P3), 1);        // Apparent power
  Serial.println("W ");



  // Print 'Hello World!' on the first line of the LCD:
  lcd.setCursor(0, 0); // Set the cursor on the first column and first row.
  lcd.print("I "); // Print the string "Hello World!"
  lcd.setCursor(2, 0); // Set the cursor on the first column and first row.
  lcd.print(Irms1, 1);
  lcd.print(" ");
  lcd.setCursor(7, 0); // Set the cursor on the first column and first row.
  lcd.print(Irms2, 1);
  lcd.print(" ");
  lcd.setCursor(12, 0); // Set the cursor on the first column and first row.
  lcd.print(Irms3, 1);
  lcd.print(" ");
  lcd.setCursor(0, 1); //Set the cursor on the third column and the second row (counting starts at 0!).
  lcd.print("P ");
  lcd.setCursor(2, 1); //Set the cursor on the third column and the second row (counting starts at 0!).
  lcd.print(x);
  lcd.print(" ");
  lcd.setCursor(7, 1); //Set the cursor on the third column and the second row (counting starts at 0!).
  lcd.print(y);
  lcd.print(" ");
  lcd.setCursor(12, 1); //Set the cursor on the third column and the second row (counting starts at 0!).
  lcd.print(z);
  lcd.print(" ");
  
  }
