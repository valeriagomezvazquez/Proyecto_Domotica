#include <ESP32Servo.h>

// ----------------- PINES ESP32 -----------------
// Sensores ultrasonicos
const int trigEntrada = 5;
const int echoEntrada = 18;

const int trigSalida = 19;
const int echoSalida = 21;

// Servo
const int pinServo = 13;

// Focos (relevadores)
const int foco1 = 14;
const int foco2 = 27;
const int foco3 = 26;

// Botón focos
const int botonFocos = 25;

// ----------------- SENSOR DE GAS -----------------
const int sensorGas = 34;    // ADC del ESP32
const int botonGas = 33;
const int buzzerGas = 12;

const int UMBRAL_GAS = 560;

bool alarmaGasActiva = false;
int valorGasAnterior = 0;
unsigned long silencioGasHasta = 0;
unsigned long tiempoBotonGas = 0;

const long ANTIRREBOTE = 250;
const long TIEMPO_SILENCIO = 5000;

// ----------------- VARIABLES -----------------
Servo puerta;

bool focosEnModoAuto = true;

int personasDentro = 0;

unsigned long tiempoUltimoBotonFocos = 0;

unsigned long tiempoDeteccion = 0;

unsigned long tiempoPuerta = 0;
bool puertaAbierta = false;

const int distanciaUmbral = 35;
const int debounce = 300;
const int tiempoPuertaAbierta = 1000;
const int tiempoBloqueoLectura = 600;

// ----------------- FUNCIONES -----------------
long medirDistancia(int trig, int echo) {

  digitalWrite(trig, LOW);
  delayMicroseconds(2);
  digitalWrite(trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(trig, LOW);

  long duracion = pulseIn(echo, HIGH, 30000);
  long distancia = duracion * 0.034 / 2;

  if (distancia == 0) return 999;
  return distancia;
}

void encenderFocos() {
  digitalWrite(foco1, HIGH);
  digitalWrite(foco2, HIGH);
  digitalWrite(foco3, HIGH);
}

void apagarFocos() {
  digitalWrite(foco1, LOW);
  digitalWrite(foco2, LOW);
  digitalWrite(foco3, LOW);
}

// ----------------- CONTROL PUERTA -----------------
void abrirPuerta() {
  puerta.write(90);
  puertaAbierta = true;
  tiempoPuerta = millis();
}

void cerrarPuertaAuto() {
  if (puertaAbierta && millis() - tiempoPuerta >= tiempoPuertaAbierta) {
    puerta.write(0);
    puertaAbierta = false;
  }
}

// ----------------- SETUP -----------------
void setup() {
  Serial.begin(115200);

  // Ultrasonicos
  pinMode(trigEntrada, OUTPUT);
  pinMode(echoEntrada, INPUT);

  pinMode(trigSalida, OUTPUT);
  pinMode(echoSalida, INPUT);

  // Servo
  puerta.attach(pinServo);
  puerta.write(0);

  // Focos
  pinMode(foco1, OUTPUT);
  pinMode(foco2, OUTPUT);
  pinMode(foco3, OUTPUT);

  // Botones
  pinMode(botonFocos, INPUT_PULLUP);
  pinMode(botonGas, INPUT_PULLUP);

  // Gas
  pinMode(sensorGas, INPUT);
  pinMode(buzzerGas, OUTPUT);

  apagarFocos();
}

// ----------------- LOOP -----------------
void loop() {
  unsigned long ahora = millis();

  // ----------- SENSOR DE GAS -----------
  int valorGas = analogRead(sensorGas);
  bool gasAlto = valorGas >= UMBRAL_GAS;

  if (valorGas != valorGasAnterior) {
    Serial.print("Nivel de gas: ");
    Serial.println(valorGas);
    valorGasAnterior = valorGas;
  }

  if (gasAlto && !alarmaGasActiva) {
    alarmaGasActiva = true;
    Serial.println("¡¡ ALARMA DE GAS ACTIVADA !!");
  }

  if (!gasAlto && alarmaGasActiva) {
    alarmaGasActiva = false;
    noTone(buzzerGas);
    Serial.println("Alarma de gas desactivada");
  }

  if (digitalRead(botonGas) == LOW && ahora - tiempoBotonGas > ANTIRREBOTE) {
    if (alarmaGasActiva) {
      silencioGasHasta = ahora + TIEMPO_SILENCIO;
      noTone(buzzerGas);
      Serial.println("Buzzer silenciado 5s");
    }
    tiempoBotonGas = ahora;
  }

  if (alarmaGasActiva && ahora > silencioGasHasta) {
    tone(buzzerGas, 800);
  } else {
    noTone(buzzerGas);
  }

  // ----------- BOTÓN FOCOS -----------
  if (digitalRead(botonFocos) == LOW && ahora - tiempoUltimoBotonFocos > debounce) {
    focosEnModoAuto = !focosEnModoAuto;

    if (!focosEnModoAuto) apagarFocos();
    else if (personasDentro > 0) encenderFocos();

    tiempoUltimoBotonFocos = ahora;
  }

  // ----------- DETECCIÓN PERSONAS -----------
  if (ahora - tiempoDeteccion > tiempoBloqueoLectura) {

    long distEntrada = medirDistancia(trigEntrada, echoEntrada);
    long distSalida  = medirDistancia(trigSalida, echoSalida);

    // ---- ENTRADA ----
    if (distEntrada < distanciaUmbral && distSalida > (distanciaUmbral + 10)) {

      if (!puertaAbierta) abrirPuerta();

      personasDentro++;
      Serial.print("Personas dentro: ");
      Serial.println(personasDentro);

      if (focosEnModoAuto && personasDentro > 0)
        encenderFocos();

      tiempoDeteccion = ahora;
    }

    // ---- SALIDA ----
    if (distSalida < distanciaUmbral && distEntrada > (distanciaUmbral + 10)) {

      if (!puertaAbierta) abrirPuerta();

      if (personasDentro > 0) personasDentro--;

      Serial.print("Personas dentro: ");
      Serial.println(personasDentro);

      if (personasDentro == 0 && focosEnModoAuto)
        apagarFocos();

      tiempoDeteccion = ahora;
    }
  }

  // ----------- CERRAR PUERTA AUTO -----------
  cerrarPuertaAuto();
}
