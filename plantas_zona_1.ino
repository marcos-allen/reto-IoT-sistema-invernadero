// Reto: Sistema de IoT para gestion, monitoreo y control de un invernadero 

/* Codigo fuente del sistema de sensado de la zona 1
Elaborado por:
Marcos Allen Martínez Cortés
Sebastián Hernández Guevara
Karol Anette Lozano González
Edwin Emmanuel Salazar Meza */

#include <DHT.h>                     // Libreria para el sensor DHT11 (humedad y temperatura)
#include <PubSubClient.h>            // Libreria para gestionar la conexion MQTT

// Set up the particular type of sensor used (DHT11, DHT21, DHT22, ...) attached to esp32 board.
#define DHTPIN 14             // Digital esp32 pin to receive data from DHT.
#define DHTTYPE    DHT11      // DHT 11 is used.
DHT dht (DHTPIN, DHTTYPE);    // DHT sensor set up.

// Variables to hold temperature (t) and humidity (h) readings
float t;
float h;

// Credentials to connect to your WiFi Network.
// Credenciales para conectarse a la red WiFi
const char* ssid = ""; 
const char* password = "";

// Servidor MQTT de ThingSpeak
const char* server = "mqtt3.thingspeak.com";

// Definicion de la conexion segura o no segura
// En algunos casos, ciertos dispositivos IoT no soportan la conexion segura
//#define USESECUREMQTT             // Descomentar esta linea si se utiliza conexion segura
#ifdef USESECUREMQTT
  #include <WiFiClientSecure.h>   // Si se usa conexión segura, se incluye la librería de WiFi seguro
  #define mqttPort 8883           // Puerto para conexión segura
  WiFiClientSecure client;       // Cliente para la conexión segura
#else
  #include <WiFiS3.h>            // Si no se usa conexión segura, se usa esta librería
  #define mqttPort 1883           // Puerto para conexión no segura
  WiFiClient client;            // Cliente para la conexión no segura
#endif

// Credenciales para publicar y suscribirse en el canal de ThingSpeak
const char mqttUserName[]   = ""; 
const char clientID[]       = "";
const char mqttPass[]       = "";

// ID del canal en ThingSpeak
#define channelID 2727382

// Certificado para conexion segura (para dispositivos como el ESP32)
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

// Estado inicial de la conexion WiFi
int status = WL_IDLE_STATUS; 

// El cliente MQTT esta vinculado a la conexion WiFi
PubSubClient mqttClient( client );

// Variables para controlar la sincronizacion de las conexiones y la frecuencia de lectura de sensores (en milisegundos)
int connectionDelay    = 4;    // Retraso (s) entre intentos de conexión WiFi
long lastPublishMillis = 0;    // Para almacenar el valor de la ultima llamada de la funcion millis()
int updateInterval     = 15;   // Los valores del sensor se publican cada 15 segundos

/* Prototipos de funciones agrupadas por funcionalidad y dependencias */

// Funcion para conectar al Wi-Fi
void connectWifi();

// Funcion para conectar al servidor MQTT (mqtt3.thingspeak.com)
void mqttConnect();

// Funcion para suscribirse al canal de ThingSpeak para recibir actualizaciones
void mqttSubscribe( long subChannelID );

// Funcion para manejar los mensajes de la suscripcion MQTT al servidor de ThingSpeak
void mqttSubscriptionCallback( char* topic, byte* payload, unsigned int length );

// Funcion para publicar mensajes en el canal de ThingSpeak
void mqttPublish(long pubChannelID, String message);


void setup() {
  // Este codigo solo se ejecuta una vez al inicio.
  // Establece la tasa de transmision serial y configura la comunicacion
  Serial.begin( 115200 );

  // Un pequeño retraso para permitir que se establezca la comunicacion serial.
  delay(3000);

  // Inicializa el sensor DHT11
  dht.begin();          

  // Conexion a la red Wi-Fi
  connectWifi();   // Conecta el dispositivo a la red Wi-Fi

  // Configuracion del cliente MQTT para conectarse con el broker ThingSpeak
  mqttClient.setServer( server, mqttPort );  // Configura el servidor MQTT y el puerto
  mqttClient.setCallback( mqttSubscriptionCallback );  // Establece la funcion de callback para mensajes MQTT
  mqttClient.setBufferSize( 2048 );  // Establece el tamaño del buffer para mensajes MQTT
  
  // Conexion segura MQTT (si esta habilitada)
  #ifdef USESECUREMQTT
    client.setCACert(thingspeak_ca_cert);  // Establece el certificado CA de ThinkSpeak para conexion segura
  #endif
}

void loop() {
  // Despues de que todo esta configurado, se entra al ciclo de percepcion-accion
  // Reintentar la conexion a WiFi si se ha perdido la conexion
  if (WiFi.status() != WL_CONNECTED) {
      connectWifi();  // Reconectar WiFi si la conexion no esta activa
  }
  
  // Si el cliente MQTT no esta conectado, intentamos conectar de nuevo
  if (!mqttClient.connected()) {
     mqttConnect(); // Reconectar MQTT
     mqttSubscribe( channelID ); // Suscribirse al canal de ThingSpeak
  }
  
  // Mantener la conexion con el servidor MQTT mediante la llamada al loop()
  mqttClient.loop();
  
  // Actualizar el canal de ThingSpeak cada vez que pase el intervalo de actualizacion
  // El resultado de esta actualizacion sera el mensaje enviado al suscriptor
  if ( (millis() - lastPublishMillis) > updateInterval*1000) {
    // Lecturas de los sensores: temperatura y humedad
    t = dht.readTemperature();  // Lee la temperatura
    h = dht.readHumidity();     // Lee la humedad
    float ht = 4095 - analogRead(35);  // Lee la humedad del suelo
    
    // Verifica si la lectura de temperatura o humedad es invalida
    if ( isnan(t) || isnan(h)) {
        Serial.println("¡Fallos al leer del sensor DHT!");  // Muestra mensaje de error si falla la lectura
    }
    
    // Muestra la temperatura local en el monitor serial
    Serial.print(F("Temperatura local: "));
    Serial.print(t);
    Serial.print(" ºC ");
    // Muestra la humedad local en el monitor serial
    Serial.print(F("  Humedad local: "));
    Serial.print(h);
    Serial.println(" %");
    // Muestra la humedad del suelo en el monitor serial
    Serial.print("Humedad del suelo es: ");
    Serial.println(ht);
    
    // Condiciones para determinar el estado del suelo
    if (ht > 0 && ht < 1200) {
        Serial.println("El suelo esta seco.");
    } else if (ht < 2500 && ht >= 1200) { 
        Serial.println("El suelo esta humedo.");
    } else if (ht < 3800 && ht >= 2500) {
        Serial.println("El suelo esta muy humedo (inundado).");
    } else {
        Serial.println("Fallo.");  // Si no cumple ninguna condicion, muestra un fallo
    }

    // Publicar los valores en el canal de ThingSpeak (field2, field3, field4)
    mqttPublish( channelID, (String("field2=")+String(t) + String("&field3=")+String(h) + String("&field4=")+String(ht) ) );
    lastPublishMillis = millis(); // Actualizar el tiempo del ultimo mensaje publicado
  }
}

// Funcion para conectar a la red WiFi
void connectWifi() {
  Serial.println( "Connecting to Wi-Fi..." ); // Mensaje inicial
  // Reintentar hasta obtener conexion exitosa
    while ( WiFi.status() != WL_CONNECTED ) {
    WiFi.begin(ssid, password); // Intentar conectar usando las credenciales definidas
    delay( connectionDelay*1000 ); // Esperar el tiempo de reintento (delay entre intentos)
    Serial.println( WiFi.status() ); // Mostrar el estado de la conexion
  }
  Serial.println( "Connected to Wi-Fi." ); // Mensaje cuando la conexion WiFi es exitosa
}

// Funcion para conectar al servidor MQTT
void mqttConnect() {
  // Reintentar la conexion al servidor MQTT hasta que sea exitosa
  while ( !mqttClient.connected() ) {
    // Intentar conectar con el broker MQTT usando las credenciales definidas
    if ( mqttClient.connect( clientID, mqttUserName, mqttPass ) ) {
      Serial.print( "MQTT to " );
      Serial.print( server );
      Serial.print (" at port ");
      Serial.print( mqttPort );
      Serial.println( " successful." ); // Mostrar exito de la conexion
    } else {
      Serial.print( "MQTT connection failed, rc = " );
      // Mostrar codigo de error en caso de fallo
      Serial.print( mqttClient.state() );
      Serial.println( " Will try the connection again in a few seconds" );
      delay( connectionDelay*1000 ); // Reintentar la conexion despues de un tiempo de espera
    }
  }
}

// Funcion para suscribirse a un canal de ThingSpeak para recibir actualizaciones
void mqttSubscribe( long subChannelID ) {
  String myTopic = "channels/"+String( subChannelID )+"/subscribe"; // Crear el topico de suscripcion basado en el canal
  mqttClient.subscribe(myTopic.c_str()); // Suscribirse al topico
}

// Funcion para manejar los mensajes de la suscripcion MQTT al broker de ThingSpeak
void mqttSubscriptionCallback( char* topic, byte* payload, unsigned int length ) {
  // Imprime los detalles del mensaje recibido en el monitor serie
  Serial.print("Mensaje recibido del broker de ThinksSpeak [");
  Serial.print(topic);  // Imprime el tema del mensaje
  Serial.print("] ");
  // Imprime el contenido del mensaje recibido byte por byte
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);  // Convierte cada byte a caracter e imprime
  }
  // Imprime una nueva linea al final del mensaje
  Serial.println();
}

// Funcion para publicar un mensaje en el canal de ThingSpeak
void mqttPublish(long pubChannelID, String message) {
  String topicString ="channels/" + String( pubChannelID ) + "/publish"; // Crear el topico para la publicacion
  mqttClient.publish( topicString.c_str(), message.c_str() ); // Publicar el mensaje en el topico
}
