// ------------- Librerias -----------------

// Libreria para manejar servomotores
#include <ESP32Servo.h>

// Libreria para usar conexion a wifi
#include <WiFi.h>
#include <PubSubClient.h>

// ------------ Conexion a Red y datos necesarios para MQTT ---------------

// Nombre de la red wifi y la contraseña
const char* ssid = "";
const char* password = "";

// broker por donde vamos a enviar datos por 
const char* mqtt_server = "mqtt.eclipseprojects.io";

// Act: vamos a crear topics para el envio de datos
const char* topicMascotas = "st/grupo04/compuerta/PIR_Mascota";
const char* topicPersonas = "st/grupo04/compuerta/PIR_Persona";
const char* topicPuerta = "st/grupo04/compuerta/Estado_Puerta";

// Variables necesarias para Wifi y MQTT
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE	(50)
char msg[MSG_BUFFER_SIZE];
int value = 0;

// Metodo para configurar wifi
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Conectandose a ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());
}

// Metodo de callback para recibir mensajes desde MQTT
// Se ha descartado debido a que solo van a enviarse datos desde la puerta, no recibirse
/*
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
*/

// Metodo para reconectar
void reconnect() {
  
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("Conectado");
      // Cuando se conecte, mostrara un mensaje
      Serial.println("Suscrito al topic definido");

      // Resuscribirse a los tópicos utilizados
      client.subscribe(topicMascotas);
      client.subscribe(topicPersonas);
      client.subscribe(topicPuerta);
    } else {
      // En caso contrario, mostrara un mensaje
      Serial.print("Fallo en la conexion, reintentando=");
      Serial.print(client.state());
      Serial.println(" Intentando de nuevo en 5 segundos");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// ------------------------------ Variables -------------------------------
// Servomotor
Servo servoCerrojo;   // GPIO 2 = D2
int posCerrojo = 180; // Posición inicial del cerrojo (180 grados impide el paso)

/* 
==================================================================================================
// Estas eran las variables a usar si se trabajaba con el ESP32 que nos entregó nuestro docente

// Sensores PIR para un lado de la puerta
const int pinPirMascota1 = 5;   // GPIO 5 = D5
const int pinPirPersona1 = 18;  // GPIO 18 = D18

// Sensores PIR para el otro lado de la puerta
const int pinPirMascota2 = 22;  // GPIO 22 = D22
const int pinPirPersona2 = 23;  // GPIO 23 = D23

==================================================================================================
*/ 

// ---------------- Variables para usar ESP-WROOM-32 ---------------------------

// Sensores PIR para un lado de la puerta
const int pinPirMascota1 = 27;   // GPIO 27
const int pinPirPersona1 = 26;  // GPIO 26

// Sensores PIR para el otro lado de la puerta
// ACT: Los sensores del lado del servo estaban en posicion invertida
const int pinPirMascota2 = 33;  // GPIO 25
const int pinPirPersona2 = 25;  // GPIO 33

// ---------------- Variables de tiempo y estado -------------------------------

// Para contar segundos desde la primera deteccion de mascota
unsigned long tiempoInicioMovimientoMascota = 0;
// Indica si se ha detectado una mascota
bool movimientoMascota = false;
// Indica el estado del servomotor cerrojo
bool cerrojoAbierto = false;
// Indica si hay una persona frente a la puerta
bool personaPresente = false;

// Variables para envio de señales a sus topics correspondientes
// Bandera para evitar múltiples envíos del mensaje "Persona detectada"
// Act: No funcional
bool mensajePersonaEnviado = false; 

// ======================== Codigo ===========================
void setup() {

  // Iniciar comunicacion serial (Sirve para ver los mensajes en el monitor serial)
  Serial.begin(115200);
  // Llamar metodos para wifi
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  //client.setCallback(callback);

  // Declarar sensores PIR a utilizar
  pinMode(pinPirMascota1, INPUT);
  pinMode(pinPirMascota2, INPUT);
  pinMode(pinPirPersona1, INPUT);
  pinMode(pinPirPersona2, INPUT);

  // Configura el servomotor del cerrojo
  servoCerrojo.attach(32, 500, 2500);  // GPIO 32 = D32
  servoCerrojo.write(posCerrojo);

  Serial.println("Sistema inicializado. Esperando detección de movimiento...");
}

void loop() {

  // Metodo para reconexion a red
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  
  // Lee el estado de los sensores PIR
  int estadoPIRMascota1 = digitalRead(pinPirMascota1);
  int estadoPIRMascota2 = digitalRead(pinPirMascota2);
  int estadoPIRPersona1 = digitalRead(pinPirPersona1);
  int estadoPIRPersona2 = digitalRead(pinPirPersona2);

  // Detecta movimiento de persona si alguno de los sensores de persona lo registra, y el cerrojo está cerrado
  if ((estadoPIRPersona1 == HIGH || estadoPIRPersona2 == HIGH) && !cerrojoAbierto) {
    Serial.println("Movimiento de persona detectado. Desactivando detección de mascotas temporalmente.");
    personaPresente = true;  // Marca que hay una persona presente

    // Enviar señal "Persona detectada" al topic correspondiente
    client.publish(topicPersonas, "Persona detectada");
  }

  // Verifica si ya no hay movimiento de persona (es decir, están LOW ambos sensores de persona)
  if ((estadoPIRPersona1 == LOW && estadoPIRPersona2 == LOW) && personaPresente) {
    personaPresente = false;
    Serial.println("La persona ya no está presente. Se reactiva la detección de mascotas.");

    // Enviar señal de "Persona no detectada" al topic correspondiente
    client.publish(topicPersonas, "Persona no detectada");
  }

  // Lógica para detección de mascotas solo cuando no hay deteccion de persona en ninguno de los 2 sensores PIR Persona
  // y el cerrojo se encuentre cerrado
  if (!personaPresente && (estadoPIRMascota1 == HIGH || estadoPIRMascota2 == HIGH) && !movimientoMascota && !cerrojoAbierto) {
    Serial.println("Movimiento de mascota detectado. Abriendo cerrojo...");
    abrirCerrojo();
    movimientoMascota = true;
    tiempoInicioMovimientoMascota = millis();

    // Enviar señal "Mascota detectada" al topic correspondiente
    client.publish(topicMascotas, "Mascota detectada");
  }
  
  // Detecta movimiento de persona mientras el cerrojo está abierto y muestra un mensaje
  if ((estadoPIRPersona1 == HIGH || estadoPIRPersona2 == HIGH) && cerrojoAbierto) {
    Serial.println("Movimiento de persona detectado, ignorado mientras el cerrojo está abierto.");
  }

  // Verifica si ya pasó el tiempo de espera tras la detección de mascota para cerrar el cerrojo
  // Lo ideal es que sea 1 minuto de apertura en el servo
  // Por temas de tiempo y para hacer ejemplos visibles, se redujo a 15 segundos
  if (movimientoMascota && millis() - tiempoInicioMovimientoMascota >= 15000) {
    cerrarCerrojo();
    movimientoMascota = false;
    Serial.println("Se cerró el cerrojo después de detección de mascota.");

    // Enviar info por MQTT
    client.publish(topicMascotas, "Mascota no detectada");
  }

  delay(100); // Evita lecturas excesivamente rápidas
}

// Función: abre el cerrojo
// Tambien envia la señal "ABIERTO" al topicPuerta
void abrirCerrojo() {
  for (posCerrojo = 180; posCerrojo >= 100; posCerrojo -= 1) {
    servoCerrojo.write(posCerrojo);
    delay(15); // Controla la velocidad de movimiento
  }
  cerrojoAbierto = true;

  // Enviar info por MQTT
  client.publish(topicPuerta, "ABIERTA");
}

// Función: cierra el cerrojo
// Tambien envia la señal "CERRADO" al topicPuerta
void cerrarCerrojo() {
  for (posCerrojo = 100; posCerrojo <= 180; posCerrojo += 1) {
    servoCerrojo.write(posCerrojo);
    delay(15); // Controla la velocidad de movimiento
  }
  cerrojoAbierto = false;

  // Enviar info por MQTT
  client.publish(topicPuerta, "CERRADA");
}