// Llamar librería para uso de servomotor
#include <Servo.h>

// Variables para servomotor y sensor PIR
Servo servoPIR1;
int pos = 50; // Posición inicial del servomotor
const int pinPIR = 2;   // Pin donde está conectado el sensor PIR

// Variable para servomotor "cerrojo"
Servo servoCerrojo;
int pos2 = 180; // Posición inicial del cerrojo (Osea apuntando hacia arriba, impidiendo el movimiento libre de la puerta batible)

// Variables de tiempo
unsigned long tiempoInicioMovimiento = 0; // Guarda el tiempo cuando empieza la detección
unsigned long tiempoInicioNoMovimiento = 0; // Guarda el tiempo cuando termina la detección

bool movimiento = false;  // Indica si el sensor está detectando movimiento
bool primerMovimiento = false; // Indica si ya se ha realizado el primer movimiento del servo
bool regresoServo = false; // Indica si el servo ya regresó a la posición inicial

void setup() {
  pinMode(pinPIR, INPUT);  // Configura el pin PIR como entrada
  Serial.begin(9600);      // Inicia la comunicación serial

  // El servoPIR1 estará conectado al pin 9
  servoPIR1.attach(9, 500, 2500);
  // Establecer el servomotor en la posición de 50 grados al inicio
  servoPIR1.write(50);
  pos = 50;  // Actualizar la variable de posición

  // ===================================
  // El servoCerrojo estará conectado al pin 11
  servoCerrojo.attach(11, 500, 2500);
  // Establecer el servo en la posición de 180 grados al inicio hacia arriba
  servoCerrojo.write(180);
  pos2 = 180;  // Actualizar la variable de posición

  // Mensaje inicial para confirmar que el sistema está listo
  Serial.println("Sistema inicializado. Esperando detección de movimiento...");
}

void loop() {
  // Lee el estado del sensor PIR
  int estadoPIR = digitalRead(pinPIR);

  // Si el sensor detecta algo y la variable `movimiento` está en falso
  if (estadoPIR == HIGH && !movimiento) {
     
    // Si es el primer movimiento, mueve el servomotor hacia arriba
    if (!primerMovimiento) {
      movimiento1(); // Mueve de 50 a 130 grados
      primerMovimiento = true; // Marca que ya se hizo el primer movimiento
      regresoServo = false; // Resetea la bandera para regreso del servo
      Serial.println("Movimiento detectado!");

    // Movimiento especial cuando detecta movimiento de nuevo sin haber regresado
    } else if (primerMovimiento && !regresoServo) {

      movEspecialAbajo(); // Mueve de 130 a 10 grados
      Serial.println("Movimiento detectado (Posiblemente una persona)");
      Serial.println("Servo en enfriamiento por 10 segundos.");
      delay(10000); // Periodo de enfriamiento de 10 segundos
      movEspecialArriba(); // Regresa a la posición original
      delay(2000); // Enfriamiento de 2 segundos para evitar detección errónea

      // Restablece las banderas para esperar un nuevo ciclo de detección
      primerMovimiento = false; 
      // Resetea la bandera para regreso del servo
      regresoServo = false;
      movimiento = false;
      limpiador();
      Serial.println("Esperando detección de movimiento...");

    }

    // Registra que hay movimiento
    movimiento = true;
    tiempoInicioMovimiento = millis(); // Empieza a contar el tiempo de detección

  // Si la señal del sensor baja, pero la variable `movimiento` es true
  } else if (estadoPIR == LOW && movimiento) {
    movimiento = false;
    tiempoInicioNoMovimiento = millis(); // Guarda el tiempo cuando deja de detectar
  } 

  // Si no hay movimiento y han pasado menos de 10 segundos
  if (!movimiento && primerMovimiento) {
    unsigned long tiempoSinMovimiento = millis() - tiempoInicioNoMovimiento;
    
    // Imprime el tiempo sin detección continuamente
    Serial.print("Sin detección de movimiento durante: ");
    Serial.print(tiempoSinMovimiento / 1000.0); // Imprime la duración en segundos
    Serial.println(" segundos");

    // Si no hay movimiento durante 10 segundos y el servo aún no ha regresado
    if (tiempoSinMovimiento >= 10000 && !regresoServo) {
      movimiento2(); // Mueve de 130 a 50 grados
      Serial.println("Servo regresó a la posición inicial.");
      delay(2000); // Enfriamiento de 2 segundos
      regresoServo = true; // Marca que el servo ya regresó

      // Abre el cerrojo por 15 segundos
      abrirCerrojo();
      Serial.println("Se ha abierto el cerrojo por 15 segundos.");
      delay(15000); // Espera 15 segundos antes de cerrar
      cerrarCerrojo();
      Serial.println("Se ha cerrado el cerrojo.");

      // Restablece las banderas para esperar un nuevo ciclo de detección
      primerMovimiento = false;
      limpiador();

      Serial.println("Esperando detección de movimiento...");
    }
  }

  delay(100); // Pequeño retraso para evitar lecturas erráticas
}

// Función: movimiento de 50 a 130 grados
void movimiento1() {
  for (pos = 50; pos <= 130; pos += 1) {
    servoPIR1.write(pos);
    delay(15); // Espera 15 milisegundos
  }
}


// Función: movimiento de 130 a 50 grados
void movimiento2() {
  for (pos = 130; pos >= 50; pos -= 1) {
    servoPIR1.write(pos);
    delay(15); // Espera 15 milisegundos
  }
}

// Función especial: movimiento de 130 a 10 grados
void movEspecialAbajo() {
  for (pos = 130; pos >= 10; pos -= 1) {
    servoPIR1.write(pos);
    delay(15); // Espera 15 milisegundos
  }
}

// Función especial: movimiento de 10 a 50 grados
// Para su posicion original
void movEspecialArriba() {
  for (pos = 10; pos <= 50; pos += 1) {
    servoPIR1.write(pos);
    delay(15); // Espera 15 milisegundos
  }
}


// Funcion: movimiento de 180 a 100 grados para "abrir" cerrojo
// Osea "bajar" el servomotor
void abrirCerrojo(){
    for (pos2 = 180; pos2 >= 100; pos2 -= 1) {
    servoCerrojo.write(pos2);
    delay(15); // Espera 15 milisegundos
  }
}

// Funcion: movimiento de 100 a 180 grados para "cerrar" cerrojo
// Osea "subir" el servomotor
void cerrarCerrojo(){
    for (pos2 = 100; pos2 <= 180; pos2 += 1) {
    servoCerrojo.write(pos2);
    delay(15); // Espera 15 milisegundos
  }
}

// Funcion extra: limpiador de consola
// Como tal no limpia la consola, solo imprime campos en blanco
// Pero al menos simula limpiar la consola
void limpiador(){
  for (int var = 0; var < 15; var++) {
    Serial.println("");
  }
}
