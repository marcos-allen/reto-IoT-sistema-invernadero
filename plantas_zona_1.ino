#include <DHT.h>                     // Library required for DHT11 sensor
#include <PubSubClient.h>            // Library required for MQTT

// Set up the particular type of sensor used (DHT11, DHT21, DHT22, ...) attached to esp32 board.
#define DHTPIN 14             // Digital esp32 pin to receive data from DHT.
#define DHTTYPE    DHT11      // DHT 11 is used.
DHT dht (DHTPIN, DHTTYPE);    // DHT sensor set up.

// Variables to hold temperature (t) and humidity (h) readings
float t;
float h;

// Credentials to connect to your WiFi Network.
// Strongly related to your working envirnonment

const char* ssid = ""; 
const char* password = "";

// MQTT broker server. Note that the specific port is defined depending on the type of the
// used connection.
const char* server = "mqtt3.thingspeak.com";

// A macro is used to select between secure and nonsecure connection as it is hardware-dependent
// Certain IoT hardware platforms do not work with the WiFiClientSecure library.

//#define USESECUREMQTT             // Comment this line if nonsecure connection is used

#ifdef USESECUREMQTT
  #include <WiFiClientSecure.h>
  #define mqttPort 8883
  WiFiClientSecure client; 
#else
  #include <WiFi.h>
  #define mqttPort 1883
  WiFiClient client;
#endif

// Credentials that allow to publish and subscribe to the ThingSpeak channel. It depends on your
// ThinkSpeak account and the defined channels
const char mqttUserName[]   = ""; 
const char clientID[]       = "";
const char mqttPass[]       = "";

// Channel ID that is defined in the ThinkSpeak account tp hold out streaming data. Recall that
// up to eigth fields are allowed per channel
#define channelID 2727382                     // This channel holds two fields: temperature and humidity


// Since the target esp32-based board support secure connections, a thingspeak certificate is used.

  const char * PROGMEM thingspeak_ca_cert = \
  "-----BEGIN CERTIFICATE-----\n" \
  "MIIDxTCCAq2gAwIBAgIQAqxcJmoLQJuPC3nyrkYldzANBgkqhkiG9w0BAQUFADBs\n" \
  "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n" \
  "d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\n" \
  "ZSBFViBSb290IENBMB4XDTA2MTExMDAwMDAwMFoXDTMxMTExMDAwMDAwMFowbDEL\n" \
  "MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\n" \
  "LmRpZ2ljZXJ0LmNvbTErMCkGA1UEAxMiRGlnaUNlcnQgSGlnaCBBc3N1cmFuY2Ug\n" \
  "RVYgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMbM5XPm\n" \
  "+9S75S0tMqbf5YE/yc0lSbZxKsPVlDRnogocsF9ppkCxxLeyj9CYpKlBWTrT3JTW\n" \
  "PNt0OKRKzE0lgvdKpVMSOO7zSW1xkX5jtqumX8OkhPhPYlG++MXs2ziS4wblCJEM\n" \
  "xChBVfvLWokVfnHoNb9Ncgk9vjo4UFt3MRuNs8ckRZqnrG0AFFoEt7oT61EKmEFB\n" \
  "Ik5lYYeBQVCmeVyJ3hlKV9Uu5l0cUyx+mM0aBhakaHPQNAQTXKFx01p8VdteZOE3\n" \
  "hzBWBOURtCmAEvF5OYiiAhF8J2a3iLd48soKqDirCmTCv2ZdlYTBoSUeh10aUAsg\n" \
  "EsxBu24LUTi4S8sCAwEAAaNjMGEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF\n" \
  "MAMBAf8wHQYDVR0OBBYEFLE+w2kD+L9HAdSYJhoIAu9jZCvDMB8GA1UdIwQYMBaA\n" \
  "FLE+w2kD+L9HAdSYJhoIAu9jZCvDMA0GCSqGSIb3DQEBBQUAA4IBAQAcGgaX3Nec\n" \
  "nzyIZgYIVyHbIUf4KmeqvxgydkAQV8GK83rZEWWONfqe/EW1ntlMMUu4kehDLI6z\n" \
  "eM7b41N5cdblIZQB2lWHmiRk9opmzN6cN82oNLFpmyPInngiK3BD41VHMWEZ71jF\n" \
  "hS9OMPagMRYjyOfiZRYzy78aG6A9+MpeizGLYAiJLQwGXFK3xPkKmNEVX58Svnw2\n" \
  "Yzi9RKR/5CYrCsSXaQ3pjOLAEFe4yHYSkVXySGnYvCoCWw9E1CAx2/S6cCZdkGCe\n" \
  "vEsXCS+0yx5DaMkHJ8HSXPfqIbloEpw8nL+e/IBcm2PN7EeqJSdnoDfzAIJ9VNep\n" \
  "+OkuE6N36B9K\n" \
  "-----END CERTIFICATE-----\n";


// Initial state of the wifi connection
int status = WL_IDLE_STATUS; 

// The MQTT client is liked to the wifi connection
PubSubClient mqttClient( client );

// Variables are defined to control the timing of connections and to define the
// update rate of sensor readings (in milliseconds)

int connectionDelay    = 4;    // Delay (s) between trials to connect to WiFi
long lastPublishMillis = 0;    // To hold the value of last call of the millis() function
int updateInterval     = 15;   // Sensor readings are published every 15 seconds o so.


/***
* Function prototypes mainly grouped by funcionality and dependency
**/

// Function to connect to WiFi.
void connectWifi();

// Function to connect to MQTT server, i.e., mqtt3.thingspeak.com
void mqttConnect();

// Function to subscribe to ThingSpeak channel for updates.
void mqttSubscribe( long subChannelID );

// Function to handle messages from MQTT subscription to the ThingSpeak broker.
void mqttSubscriptionCallback( char* topic, byte* payload, unsigned int length );

// Function to publish messages to a ThingSpeak channel.
void mqttPublish(long pubChannelID, String message);


void setup() {
  // Recall that this code is executed only once.
  // Establish the serial transmission rate and set up the communication
  Serial.begin( 115200 );
  // Some delay to allow serial set up.
  delay(3000);

  dht.begin();                   // Initialize DHT11 sensor          

  // Connect to the specified Wi-Fi network.
  connectWifi();
  
  // Configure the MQTT client to connect with ThinkSpeak broker
  mqttClient.setServer( server, mqttPort ); 
  
  // Set the MQTT message handler function.
  mqttClient.setCallback( mqttSubscriptionCallback );
  // Set the buffer to handle the returned JSON. 
  // NOTE: A buffer overflow of the message buffer will result in your callback not being invoked.
  mqttClient.setBufferSize( 2048 );
  
  // Use secure MQTT connections if defined.
  #ifdef USESECUREMQTT
    // Handle functionality differences of WiFiClientSecure library for different boards.
    // Herein we are dealing with esp32-based IoT boards.
      client.setCACert(thingspeak_ca_cert);
  #endif
}

void loop() {
  // After everythins is set up, go the perception-action loop
  // Reconnect to WiFi if it gets disconnected.
  if (WiFi.status() != WL_CONNECTED) {
      connectWifi();
  }
  
  // Connect if MQTT client is not connected and resubscribe to channel updates.
  // ThinkSpeak broaker server : suscribe to the specified channel
  if (!mqttClient.connected()) {
     mqttConnect(); 
     mqttSubscribe( channelID );
  }
  
  // Call the loop to maintain connection to the server.
  mqttClient.loop(); 
  
  // Update ThingSpeak channel periodically according to the specified rate. 
  // The update results in the message to the subscriber.
  if ( (millis() - lastPublishMillis) > updateInterval*1000) {
    // Sensors readings: temperature and humidity
    t = dht.readTemperature();
    h = dht.readHumidity();
    float ht = 4095 - analogRead(35);

    if ( isnan(t) || isnan(h)) {
        Serial.println("Failed to read from DHT sensor!");
      }
      Serial.print(F("Local temperature: "));
      Serial.print(t);
      Serial.print(" ºC ");
      Serial.print(F("  Local humidity: "));
      Serial.print(h); 
      Serial.println(" %");
      Serial.print("Humedad en tierra es: ");
      Serial.println(ht);

      /*0 ~300 : dry soil 
      300~700 : humid soil 
      700~950: in water */ // base 1024
      if (ht < 1200) {
        Serial.println("El suelo está seco (o el sensor presenta fallos)");
      } else if (ht < 2800 && ht >= 1200) { 
        Serial.println("El suelo está humedo");
      } else if (ht < 3800 && ht >= 2800) {
        Serial.println("El suelo esta practicamente en agua");
      } else {
        Serial.println("Fallo");
      }
      
    //mqttPublish( channelID, (String("field1=")+String(WiFi.RSSI())) );
    mqttPublish( channelID, (String("field2=")+String(t) + String("&field3=")+String(h) + String("&field4=")+String(ht) ) );
    lastPublishMillis = millis();
  }
}

// Function to connect to WiFi.
void connectWifi() {
  Serial.println( "Connecting to Wi-Fi..." );
  // Loop until WiFi connection is successful
    while ( WiFi.status() != WL_CONNECTED ) {
    //WiFi.begin( ssid, pass );
    WiFi.begin(ssid, password);
    delay( connectionDelay*1000 );
    Serial.println( WiFi.status() ); 
  }
  Serial.println( "Connected to Wi-Fi." );
}

// Function to connect to MQTT server.
void mqttConnect() {
  // Loop until the client is connected to the server.
  while ( !mqttClient.connected() ) {
    // Connect to the MQTT broker.
    if ( mqttClient.connect( clientID, mqttUserName, mqttPass ) ) {
      Serial.print( "MQTT to " );
      Serial.print( server );
      Serial.print (" at port ");
      Serial.print( mqttPort );
      Serial.println( " successful." );
    } else {
      Serial.print( "MQTT connection failed, rc = " );
      // See https://pubsubclient.knolleary.net/api.html#state for the failure code explanation.
      Serial.print( mqttClient.state() );
      Serial.println( " Will try the connection again in a few seconds" );
      delay( connectionDelay*1000 );
    }
  }
}

// Function to subscribe to ThingSpeak channel for updates.
void mqttSubscribe( long subChannelID ){
  String myTopic = "channels/"+String( subChannelID )+"/subscribe";
  mqttClient.subscribe(myTopic.c_str());
}

// Function to handle messages from MQTT subscription to the ThingSpeak broker.
void mqttSubscriptionCallback( char* topic, byte* payload, unsigned int length ) {
  // Print the message details that was received to the serial monitor.
  Serial.print("Message arrived from ThinksSpeak broker [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// Function to publish messages to a ThingSpeak channel.
void mqttPublish(long pubChannelID, String message) {
  String topicString ="channels/" + String( pubChannelID ) + "/publish";
  mqttClient.publish( topicString.c_str(), message.c_str() );
}
