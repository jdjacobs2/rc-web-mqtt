// https://github.com/juarezefren/mecatronica/blob/master/Ultrasonico_Node.ino
// https://www.youtube.com/watch?v=1BllzhJKo9o
// https://microcontrollerslab.com/esp8266-nodemcu-web-server-using-littlefs-flash-file-system/#Creating_HTML_file
// https://microcontrollerslab.com/esp32-esp8266-web-server-input-data-html-forms/#Example_1_Arduino_Sketch_for_Input_data_to_HTML_form_web_server
// https://randomnerdtutorials.com/esp32-websocket-server-arduino/

#include <ESP8266WiFi.h>  // Enables the ESP8266 to connect to the local network (via WiFi)
#include <PubSubClient.h> // Allows us to connect to, and publish to the MQTT broker
#include <ESP8266mDNS.h>
#include <RCSwitch.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <LittleFS.h>
#include <ArduinoJson.h>

// Create AsyncWebServer object
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

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
char msgBuf[100];

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
// The client id identifies the ESP8266 device
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
}

// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics
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
      // You can subscribe to more topics
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

// receiving WebSocket message
void onWsEvent(AsyncWebSocket *ws, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  Serial.println('in onWsEvent');
  if (type == WS_EVT_CONNECT)
  {
    Serial.printf("ws[%s][%u] connect\n", ws->url(), client->id());
    client->printf("Hello Client %u :)", client->id());
    client->ping();
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    Serial.printf("ws[%s][%u] disconnect\n", ws->url(), client->id());
  }
  else if (type == WS_EVT_ERROR)
  {
    Serial.printf("ws[%s][%u] error(%u): %s\n", ws->url(), client->id(), *((uint16_t *)arg), (char *)data);
  }
  else if (type == WS_EVT_PONG)
  {
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", ws->url(), client->id(), len, (len) ? (char *)data : "");
  }
  else if (type == WS_EVT_DATA)
  // TODO:  Must include code to repaint index.html for new device and function
  // Assumes all data comes in one frame.
  {
    Serial.println("in onWsEvent WS_TEXT");
    // String msg = "";
    char msg[100];
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->opcode == WS_TEXT)
    {
      for (size_t i = 0; i < info->len; i++)
      {
        msg[i] = (char)data[i];
      }
      msg[info->len] = '\0';
    }
    // Serial.printf("msg = %s\n", msg);
    char *token;
    token = strtok(msg, ",");
    strcpy(deviceBuf, token);
    token = strtok(NULL, ",");
    strcpy(functionBuf, token);

    if (info->opcode == WS_TEXT)
    {
      sprintf(msg, "device is %s and function is %s",
              deviceBuf, functionBuf);
      client->text(msg);
    }
  }
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

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/index.html", String(), false); });

  server.on("/app.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(LittleFS, "/app.js", String(), false); });

  // setup websockets server
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.begin();
  Serial.println("HTTP server started");

  mySwitch.enableReceive(4); // Receiver on interrupt 0 => that is pin #2

  // Setup mDNS
  if (!MDNS.begin("remotecontrol"))
  { // Start the mDNS responder for esp8266.local
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");
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
    // output(mySwitch.getReceivedValue(), mySwitch.getReceivedBitlength(), mySwitch.getReceivedDelay(), mySwitch.getReceivedRawdata(), mySwitch.getReceivedProtocol());

    // TODO:  mqtt device, function and code
    char recvValueBuf[100];
    sprintf(recvValueBuf, "%u", mySwitch.getReceivedValue()); // decimal value
    Serial.print("In mySwitch value of deviceBuf:  ");
    Serial.println(deviceBuf);
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

    // TODO: must find how to send to client
    // request->send(LittleFS, "/index.html", String(), false, processor);

    mySwitch.resetAvailable();
  }

  //   if (millis() - lastTime >= 1000)
  //   {
  //     Serial.println("in millis for mySwitch");
  //     lastTime = millis();

  //   }

  // delay(1000);
}
