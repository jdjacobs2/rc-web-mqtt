// https://github.com/juarezefren/mecatronica/blob/master/Ultrasonico_Node.ino
// https://www.youtube.com/watch?v=1BllzhJKo9o
// https://microcontrollerslab.com/esp8266-nodemcu-web-server-using-littlefs-flash-file-system/#Creating_HTML_file
// https://microcontrollerslab.com/esp32-esp8266-web-server-input-data-html-forms/#Example_1_Arduino_Sketch_for_Input_data_to_HTML_form_web_server

#include <ESP8266WiFi.h>  // Enables the ESP8266 to connect to the local network (via WiFi)
#include <PubSubClient.h> // Allows us to connect to, and publish to the MQTT broker
#include <ESP8266mDNS.h>
#include <RCSwitch.h>
// #include <ESP8266WebServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// Create AsyncWebServer object
AsyncWebServer server(80);

RCSwitch mySwitch = RCSwitch(); // Initialize RCSwitch (radio freq) library

void handleRoot(); // function prototypes for HTTP handlers
void handleNotFound();

const char *location = "rc";
const char *ssid = "windsong";
const char *wifi_password = "fubsey00";
long now;
long lastTime;
char deviceBuf[50]; // Initilize buffer to hold device name
char functionBuf[50];

// Don't change the function below. This functions connects your ESP8266 to your router
void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, wifi_password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

// MQTT
// Make sure to update this for your own MQTT Broker!
const char *mqtt_server = "homeauto.local";
// const char *mqtt_topic = "rc/read";
// The client id identifies the ESP8266 device. Think of it a bit like a hostname (Or just a name, like Greg).
const char *clientID = "ESP8266RC";

// Initialise the WiFi and MQTT Client objects at 26 in derived
WiFiClient espClient;
// PubSubClient client(mqtt_server, 1883, wifiClient); // 1883 is the listener port for the Broker
PubSubClient client(espClient);

// This functions is executed when some device publishes a message to a topic
// that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device
// publishes a message to a topic that your ESP8266 is subscribed you can
// actually do something

void callback(String topic, byte *message, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  // Serial.println();
}

// Feel free to add more if statements to control more GPIOs with MQTT

// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266
void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
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
    if (client.connect("ESP8266RC"))
    {
      Serial.println("connected");
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      client.subscribe("rc/write");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// Replaces placeholder with LED state value
String processor(const String &var)
{
  Serial.println(var);
  // if(var == "GPIO_STATE"){
  //   if(digitalRead(ledPin)){
  //     ledState = "OFF";
  //   }
  //   else{
  //     ledState = "ON";
  //   }
  //   Serial.print(ledState);
  //   return ledState;
  // }
  return String();
}

void setup()
{
  // Setup wifi
  Serial.begin(115200);
  setup_wifi();
  Serial.print("\n\n");
  Serial.print("Location: ");
  Serial.println(location);

  // Initialze LittleFS
  if (!LittleFS.begin())
  {
    Serial.println("An error has occurred while mounting LittleFS");
    return;
  }

  // Setup MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Serial.println("");

  // Setup HTTP server root route
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    // processor is used for template substitution
    request->send(LittleFS, "/index.html", String(), false, processor); });

  // Route to send instructions to homeauto.local
  server.on("/report", HTTP_GET,
            [](AsyncWebServerRequest *request)
            {
              const String device = "device";
              const String function = "function";

              // TODO:  handle error if no parameter (use request->hasParam)
              if (request->hasParam(device))
              {
                const String &deviceValue = request->getParam(device)->value();
                Serial.println(deviceValue);
                deviceValue.toCharArray(deviceBuf, 50);
                Serial.print("device in deviceBuf:  ");
                Serial.println(deviceBuf);
                client.publish("rc/read", deviceValue.c_str());
              }
              if (request->hasParam(function))
              {
                const String &functionValue = request->getParam(function)->value();
                Serial.println(functionValue);
                functionValue.toCharArray(functionBuf, 50);
                Serial.print("function in functionBuf:  ");
                Serial.println(functionBuf);
                client.publish("rc/read", functionValue.c_str());
              }

              request->send(LittleFS, "/report.html", String(), false, processor);
            });

  server.begin();
  Serial.println("HTTP server started");

  mySwitch.enableReceive(4); // Receiver on interrupt 0 => that is pin #2

  // Setup mDNS
  if (!MDNS.begin("remotecontrol"))
  { // Start the mDNS responder for esp8266.local
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");
  MDNS.addService("http", "tcp", 80);
}

void loop()
{
  if (!client.connected()) // Establish MQTT listner
  {
    reconnect();
  }

  // Following necessary for subscribing and maintain connection
  if (!client.loop())
    client.connect("ESP8266RC");

  if (mySwitch.available())
  {
    Serial.println("my switch is available");
    output(mySwitch.getReceivedValue(), mySwitch.getReceivedBitlength(), mySwitch.getReceivedDelay(), mySwitch.getReceivedRawdata(), mySwitch.getReceivedProtocol());

    // TODO:  mqtt device, function and code
    char recvValueBuf[100];
    // Serial.print("received value:  ");
    // Serial.println(mySwitch.getReceivedValue());
    sprintf(recvValueBuf, "%u", mySwitch.getReceivedValue());
    // char msgBuf[150];
    Serial.print("In mySwitch value of deviceBuf:  ");
    // Serial.println(typeid(deviceBuf).name());
    Serial.println(deviceBuf);
    // sprintf(msgBuf, "%s  %s  %u", deviceBuf, functionBuf, mySwitch.getReceivedValue());
    // sprintf(msgBuf, '{"device":"%c", "function":"%c", "code":"%u"}',
    // (char *)deviceBuf, (char *)functionBuf, mySwitch.getReceivedValue());
    const int capacity = JSON_OBJECT_SIZE(5);
    StaticJsonDocument<capacity> msg;
    msg["device"] = deviceBuf;
    msg["function"] = functionBuf;
    msg["code"] = mySwitch.getReceivedValue();

    char msgBuf[150];
    serializeJson(msg, msgBuf);
    Serial.print("msg:  ");
    // serializeJson(msg, Serial);
    Serial.println(msgBuf);
    // Serial.println(msg);
    client.publish("rc/read", msgBuf);

    mySwitch.resetAvailable();
  }

  //   if (millis() - lastTime >= 1000)
  //   {
  //     Serial.println("in millis for mySwitch");
  //     lastTime = millis();

  //   }

  // delay(1000);
}
