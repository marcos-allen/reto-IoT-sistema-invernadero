// Reto: Sistema de IoT para gestion, monitoreo y control de un invernadero 

/* Codigo fuente del sistema de control de agua (riego automatizado)
Elaborado por:
Marcos Allen Martínez Cortés
Sebastián Hernández Guevara
Karol Anette Lozano González
Edwin Emmanuel Salazar Meza */


#include <PubSubClient.h> // Libreria para gestionar la conexion MQTT
#include <ArduinoJson.h> // Libreria para manejar mensajes JSON de manera sencilla
#include "GravityTDS.h" // Libreria para leer los datos del sensor TDS (Total Dissolved Solids) de agua
#include <EEPROM.h> // Libreria para leer y escribir en la memoria EEPROM

// Definicion de variables a utilizar
#define TdsSensorPin A0
GravityTDS gravityTds;      // Objeto para interactuar con el sensor TDS
float calidad;              // Variable para almacenar la calidad del agua
const int bombaAguaZona1 = 7;  // Pin para la bomba de agua en la Zona 1
const int bombaAguaZona2 = 8;  // Pin para la bomba de agua en la Zona 2
int field4_value = 0;      // Variable para almacenar la humedad del suelo de la zona 1
int field7_value = 0;      // Variable para almacenar la humedad del suelo de la zona 2
float field2_value = 0;    // Variable para almacenar la temperatura de la zona 1
float field5_value = 0;    // Variable para almacenar la temperatura de la zona 2

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
  // Este codigo se ejecuta solo una vez al inicio del programa
  
  // Configuracion inicial del puerto serial y memoria EEPROM
  Serial.begin(115200);  // Establece la tasa de transmision serial
  EEPROM.begin();        // Inicializa la memoria EEPROM para configuraciones persistentes
  delay(3000);           // Retraso para permitir que la configuracion serial se complete

  // Configuracion del sensor TDS (Total Dissolved Solids) de calidad de agua
  gravityTds.setPin(TdsSensorPin);    // Establece el pin del sensor TDS
  gravityTds.setAref(3.3);            // Establece el voltaje de referencia a 3.3V
  gravityTds.setAdcRange(1024);       // Establece el rango del ADC a 1024
  gravityTds.begin();                 // Inicializa el sensor TDS

  // Configuracion de las bombas de agua
  pinMode(bombaAguaZona1, OUTPUT);    // Configura el pin de la bomba de la zona 1 como salida
  pinMode(bombaAguaZona2, OUTPUT);    // Configura el pin de la bomba de la zona 2 como salida
  digitalWrite(bombaAguaZona1, LOW); // Apaga la bomba de la zona 1
  digitalWrite(bombaAguaZona2, LOW); // Apaga la bomba de la zona 2
  
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

    // Si los valores de field2 y field5 no son 0, se calcula el promedio de estos y se establece como temperatura
    if (field2_value != 0 && field5_value != 0) {
      float promedio = (field2_value + field5_value) / 2;
      gravityTds.setTemperature(promedio); // Establecer la temperatura para el sensor TDS
    } else {
      gravityTds.setTemperature(20); // Si los valores son incorrectos, usar un valor por defecto de 20°C
    }
    gravityTds.update(); // Actualizar el sensor de TDS
    calidad = gravityTds.getTdsValue(); // Obtener el valor de calidad del agua en PPM
    Serial.print(F("Quality: "));
    Serial.print(calidad, 2);
    Serial.println("ppm");

    // Evaluar la calidad del agua segun el valor de TDS
    if (calidad < 500) {
      Serial.println("La calidad del agua es excelente.");
    } else if (calidad >= 500) {
      Serial.println("El agua presenta indicios de contaminación (no recomendable).");
    }

    // Publicar el valor de calidad del agua en el canal de ThingSpeak (field1)
    mqttPublish( channelID, (String("field1=")+String(calidad)));
    lastPublishMillis = millis(); // Actualizar el tiempo del ultimo mensaje publicado

    // Si la calidad del agua no es aceptable (TDS >= 500), desactivar ambas bombas
    if (calidad >= 500) {
      digitalWrite(bombaAguaZona1, LOW); // Apagar la bomba de Zona 1
      digitalWrite(bombaAguaZona2, LOW); // Apagar la bomba de Zona 2
      Serial.println("Ambas bombas desactivadas debido a mala calidad del agua.");
    } else {
      // Si la calidad del agua es aceptable, controlar las bombas basándose en los valores de humedad del suelo (field4 y field7)
      if (field4_value < 1200 && field7_value < 1200 && field4_value != 0 && field7_value != 0 ) {
        digitalWrite(bombaAguaZona1, HIGH); // Activar ambas bombas si los valores de humedad son bajos
        digitalWrite(bombaAguaZona2, HIGH);
        Serial.println("Ambas bombas activadas porque field4 y field7 < 1200.");
      } else if (field4_value < 1200 && field4_value != 0  ) {
        digitalWrite(bombaAguaZona1, HIGH); // Activar solo la bomba de Zona 1
        digitalWrite(bombaAguaZona2, LOW);
        Serial.println("Bomba de Zona 1 activada porque field4 < 1200.");
      } else if (field7_value < 1200 && field7_value != 0 ) {
        digitalWrite(bombaAguaZona1, LOW);        
        digitalWrite(bombaAguaZona2, HIGH); // Activar solo la bomba de Zona 2
        Serial.println("Bomba de Zona 2 activada porque field7 < 1200.");
      } else {
        // Si los valores de humedad son altos o los datos son erroneos (0), desactivar ambas bombas
        digitalWrite(bombaAguaZona1, LOW);
        digitalWrite(bombaAguaZona2, LOW); 
        Serial.println("Ambas bombas desactivadas porque field4 y field7 >= 1200 o datos erroneos (0)");
      } 
    }
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

// Funcion para procesar los mensajes JSON recibidos desde el servidor MQTT
void processJsonMessage(const char* jsonMessage) {
  // Crear un objeto DynamicJsonDocument para almacenar el mensaje JSON
  DynamicJsonDocument doc(2048);

  // Intentar deserializar el mensaje JSON
  DeserializationError error = deserializeJson(doc, jsonMessage);

  // Si hay un error al deserializar, mostrar el mensaje de error
  if (error) {
    Serial.print("Error al analizar JSON: ");
    Serial.println(error.c_str());
    return;
  }

  // Extraer los valores de los campos field2, field4, field5, y field7 del JSON
  if (doc.containsKey("field2") && !doc["field2"].isNull()) {
    field2_value = doc["field2"].as<float>();
  } 
  if (doc.containsKey("field4") && !doc["field4"].isNull()) {
    field4_value = doc["field4"].as<int>();
  } 
  if (doc.containsKey("field5") && !doc["field5"].isNull()) {
    field5_value = doc["field5"].as<float>();
  }
  if (doc.containsKey("field7") && !doc["field7"].isNull()) {
    field7_value = doc["field7"].as<int>();
  }

  // Imprimir los valores de los campos recibidos en el monitor serial
  Serial.print("Valor de field2: ");
  Serial.println(field2_value);
  Serial.print("Valor de field4: ");
  Serial.println(field4_value);
  Serial.print("Valor de field5: ");
  Serial.println(field5_value);
  Serial.print("Valor de field7: ");
  Serial.println(field7_value);
}

// Funcion callback de suscripcion MQTT que se llama cuando llega un nuevo mensaje
void mqttSubscriptionCallback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i]; // Convertir el payload de byte a cadena
  }
  Serial.println("Mensaje recibido: " + message); // Mostrar el mensaje recibido en el monitor serial
  // Llamar a la funcion para procesar el mensaje JSON
  processJsonMessage(message.c_str());
}

// Funcion para publicar un mensaje en el canal de ThingSpeak
void mqttPublish(long pubChannelID, String message) {
  String topicString ="channels/" + String( pubChannelID ) + "/publish"; // Crear el topico para la publicacion
  mqttClient.publish( topicString.c_str(), message.c_str() ); // Publicar el mensaje en el topico
}
