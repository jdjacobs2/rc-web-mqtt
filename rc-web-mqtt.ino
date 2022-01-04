// https://github.com/juarezefren/mecatronica/blob/master/Ultrasonico_Node.ino
// https://www.youtube.com/watch?v=1BllzhJKo9o

#include <ESP8266WiFi.h>  // Enables the ESP8266 to connect to the local network (via WiFi)
#include <PubSubClient.h> // Allows us to connect to, and publish to the MQTT broker
#include <ESP8266mDNS.h>
#include <RCSwitch.h>
// #include <ESP8266WebServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
// Create a webserver object that listens for HTTP request on port 80

RCSwitch mySwitch = RCSwitch(); // Initialize RCSwitch (radio freq) library

void handleRoot(); // function prototypes for HTTP handlers
void handleNotFound();

const char *location = "rc";
const char *ssid = "windsong";
const char *wifi_password = "fubsey00";
long now;
long lastTime;

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

AsyncWebServer server(80);
void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "404: Not Found");
};

void setup()
{
  // Setup wifi
  Serial.begin(115200);
  setup_wifi();
  Serial.print("\n\n");
  Serial.print("Location: ");
  Serial.println(location);

  // Setup MQTT
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  Serial.println("");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Hello World AsyncWebServer"); });

  // Setup HTTP server
  server.onNotFound(notFound);
  server.begin();
  Serial.println("HTTP server started");

  SPIFFS.begin();

  mySwitch.enableReceive(4); // Receiver on interrupt 0 => that is pin #2
}

void loop()
{

  // server.handleClient(); // Listen for HTTP requests

  // if (!client.connected())   // Establish MQTT listner
  // {
  //    reconnect();
  // }
  // if (!client.loop())
  //   client.connect("ESP8266RC");

  // Serial.println('In main loop before client.publish');
  // client.publish("rc/read", "From ESP8622");

  if (mySwitch.available())
  {
    Serial.println("my switch is available");
    output(mySwitch.getReceivedValue(), mySwitch.getReceivedBitlength(), mySwitch.getReceivedDelay(), mySwitch.getReceivedRawdata(), mySwitch.getReceivedProtocol());
    mySwitch.resetAvailable();
  }

  //   if (millis() - lastTime >= 1000)
  //   {
  //     Serial.println("in millis for mySwitch");
  //     lastTime = millis();

  //   }

  // delay(1000);
}

// bool handleFileRead(String path)
// { // send the right file to the client (if it exists)
//   Serial.println("handleFileRead: " + path);
//   if (path.endsWith("/"))
//     path += "index.html"; // If a folder is requested, send the index file
//   Serial.print("path is:  ");
//   Serial.println(path);
//   String contentType = getContentType(path); // Get the MIME type
//   String pathWithGz = path + ".gz";
//   if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path))
//   {                                                     // If the file exists, either as a compressed archive, or normal
//     if (SPIFFS.exists(pathWithGz))                      // If there's a compressed version available
//       path += ".gz";                                    // Use the compressed verion
//     File file = SPIFFS.open(path, "r");                 // Open the file
//     size_t sent = server.streamFile(file, contentType); // Send it to the client
//     file.close();                                       // Close the file again
//     Serial.println(String("\tSent file: ") + path);
//     return true;
//   }
//   Serial.println(String("\tFile Not Found: ") + path); // If the file doesn't exist, return false
//   return false;
// }

// String getContentType(String filename)
// { // convert the file extension to the MIME type
//   if (filename.endsWith(".html"))
//     return "text/html";
//   else if (filename.endsWith(".css"))
//     return "text/css";
//   else if (filename.endsWith(".js"))
//     return "application/javascript";
//   else if (filename.endsWith(".ico"))
//     return "image/x-icon";
//   else if (filename.endsWith(".gz"))
//     return "application/x-gzip";
//   return "text/plain";
// }

// void handleRoot() {
//   server.send(200, "text/plain", "Hello world!");   // Send HTTP status 200 (Ok) and send some text to the browser/client
// }

// void handleNotFound(){
//   server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
// }