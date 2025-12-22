#include <NeoSWSerial.h>
#include <DFRobotDFPlayerMini.h>

const byte chimney = A5;//2; 
const byte chimneyState = A6;
const byte engine = 3; //To applty PWM signal to control the speed.
const byte led1 = 9, led2 = 10, led3 = 11, led4 = 12;//12; ///////////////////////////////////////////////////Cambiar al final
const byte sensor = A0, trigger = A1, stationSound = 1, breakSound = 2, startingSong = 3;
const int playerBusy = 513, playerReady = 512;
const int minDist = 40, stopDist = 5;
const int accelerationTime = 2000, stoppingTime = 2000, detectPersonTime = 1000, stopSensor = 2000, sleepCycleDuration = 1000;

byte actualSong = 3, lastLapSong = 1, stoppingSong = 2, songsQ, fumeState; //Used to known in what state is the pulse of the fume.

long sensorVal;

unsigned long fewerPersonTime, fewerSensorTime, fewerFumeTime, fumeElapseTime, fewerLedTime, actualTime, fewerEngineTime, sleepDurationTime, fewerSleepingTime, fewerSleepingStepTime;

// NeoSWSerial instead of SoftwareSerial
NeoSWSerial serialConnection(6, 5); // RX, TX 5

DFRobotDFPlayerMini player;

bool person, stopSignal, playM, songStarted, stopM, engineWorking, onLeds, onFumes, stoppingEngine, goToSleep, sleeping, playingActualSong, playStopSong, setUpPowerVal, songStopped, isSleeping, sentShutdownPulse;

//To control starting engine (Time in ms)
int startEngineTime = 4000, stopEngineTime = 15000, engineSteps = 200, startStepsTime, stopStepsTime, power, distance;
int ledStepTime = 250, fumeOnPulseTime = 100, fumeOffPulseTime = 20, offFumeMultiplier = 11, sleepingTime = 30000;
const float pwmStart = 76.5, pwmEnd = 250, pwmSteps = 250.0 / 76.5;


void setup() {
  // put your setup code here, to run once:
  pinMode(chimney, OUTPUT);
  pinMode(chimneyState, INPUT);
  pinMode(engine, OUTPUT);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);
  pinMode(sensor, INPUT);
  pinMode(trigger,OUTPUT);
   ////////////////////////////////////////////////////////Put the pwm at 31.25Khz
  TCCR2B = (TCCR2B & 0b11111000) | 0x01;
  bool dfPlayerStarted = false;
  serialConnection.begin(9600);
  Serial.begin(9600); //Start communication between pc and board

  if (!player.begin(serialConnection)) {
    Serial.println("No se pudo encontrar el módulo DFPlayer Mini, asegúrate de la conexión!");
    while (true);
  }
  Serial.println("Módulo DFPlayer Mini encontrado.");
  songsQ = player.readFileCounts();
  delay(500);
  Serial.println(String("Se encontraron ") + songsQ + " pistas");
  //Inicio de variables
  person = true;
  fewerPersonTime = millis();
  distance = 200;
  goToSleep = false;
  startStepsTime = (int)startEngineTime/engineSteps;
  stopStepsTime = (int)stopEngineTime/engineSteps;
}

void loop() {
  // put your main code here, to run repeatedly:
  actualTime = millis();
  //Serial.println((String)"Estado de fumes: " + analogRead(chimneyState));
  if (person){
    // Lógica para alternar entre Detección (1000ms) y Descanso (1000ms)
    if (isSleeping){
        // Rest for 1000ms
        if (actualTime - fewerPersonTime >= sleepCycleDuration) {
            // Start detecting
            isSleeping = false;
            fewerPersonTime = actualTime; // Reinicia el tiempo para el ciclo de detección
        }
    } else {
        // ESTADO DE DETECCIÓN ACTIVA (1000ms)
        if (DetectPerson()){
            // Si detecta, comienza la secuencia del tren
            playM = true;
            engineWorking = true; 
            onLeds = true;
            onFumes = true;
            person = false; // Sale del estado de espera
            isSleeping = false; // Asegura que no está en estado de descanso
            
            fewerLedTime = actualTime;
            fewerFumeTime = actualTime;
            fewerEngineTime = actualTime;
            
        } else {
            // Si no detecta en el ciclo de detección de 1000ms
            if (actualTime - fewerPersonTime >= sleepCycleDuration) {
                // Termina el ciclo de detección, pasa a descansar
                isSleeping = true;
                fewerPersonTime = actualTime; // Reinicia el tiempo para el ciclo de descanso
            }
        }
    }
  }
  if (playM){
    if (actualSong > songsQ){
      actualSong = 3;
    }
    PlayMusic(actualSong);
    actualSong += 1;
    playM = false;
    playingActualSong = false; //Still haven't started the song.
    songStarted = false;
  }
  if (!songStarted){
    if (player.readState() == playerBusy){
      //Serial.println("Canción iniciada; listo para verificar que termine");
      playingActualSong =  true;
      songStarted = true;
    }
  }
  if (engineWorking){
    StartEngine(startStepsTime);
  }
  if (onLeds){
    if (actualTime - fewerLedTime <= ledStepTime){
      OnLeds();
    }else{
      onLeds = false;
      fewerLedTime = millis();
    }
  }
  if (!onLeds){
    if (actualTime - fewerLedTime <= ledStepTime){
      OffLeds();
    }else{
      onLeds = true;
      fewerLedTime = millis();
    }
  }
  if (onFumes){
    StartFumes();
  }
  //Verify playing actual song status, if it is ready, it means that ended last song; so stop everything
  if (player.readState() == playerReady && playingActualSong){
    //Play last lap song; number 1.
    PlayMusic(lastLapSong);
    Serial.println("Reproduciendo pista de última vuelta");
    stopSignal = true;
    playingActualSong = false;
  }
  if (stopSignal){
    distance = GetDistance();
    //Serial.println((String)"Midiendo distancia final: " + distance);
    if(distance <= 5){
        StopMusic();
        Serial.println("Pista de última vuelta detenida");
        playStopSong = true;
        stopSignal = false;
      }
  }
  //This stops everything.
  if (player.readState() == playerReady && playStopSong){
    Serial.println("Reproduciendo frenado");
    PlayMusic(stoppingSong);
    stoppingEngine = true;
    fumeOnPulseTime = (int)fumeOnPulseTime*2;
    //Prepare for measure engine time.
    fewerEngineTime = millis();
    playStopSong = false;
  }
  if (stoppingEngine){
    StopEngine(stopStepsTime); 
  }
  if(goToSleep){
    onFumes = false;
    stopSignal = false;
    goToSleep = false;
    songStarted = false;
    setUpPowerVal = false; //For stopping engine in next song.
    Serial.println("Fue a dormir");
    sleeping = true;
    //delay(30000);
    fewerPersonTime = actualTime;
    fewerSleepingTime = actualTime;
    fewerSleepingStepTime = actualTime;
    //FinalStopFumes();
  }
  if (sleeping){
    SleepingTasks();
  }
}

void SleepingTasks(){ 
  static unsigned long timeSteps;
  static int chimneySensorValue;
  chimneySensorValue = analogRead(chimneyState);
  timeSteps = actualTime - fewerSleepingStepTime;
  //Serial.println(chimneySensorValue);
  if (timeSteps <= 100){
    digitalWrite(13, HIGH);
    //Serial.println((String)"Haciendo Tareas");
    if (chimneySensorValue <= 700 && !sentShutdownPulse){
      //Serial.println((String)"Estado de fumes: " + digitalRead(chimneyState));
      digitalWrite(chimney, HIGH);
      sentShutdownPulse = true;
    }
  }
  if (timeSteps > 100 && timeSteps <= 200){
      digitalWrite(chimney, LOW);
      sentShutdownPulse = false;
      digitalWrite(13, LOW);
  }
  if (timeSteps > 200){
    fewerSleepingStepTime = actualTime;
  }
  if (actualTime - fewerSleepingTime > sleepingTime){
    person = true;
    sleeping = false;
    sentShutdownPulse = false;
  }
}

bool DetectPerson(){
  distance = GetDistance();
  if (distance <= minDist){
    return true;
  } 
  return false;
}
bool DetectStop(){
  distance = GetDistance();
  if (distance <= stopDist){
    return true;
  } 
  return false;
}
void PlayMusic(int song){
  player.play(song);
  Serial.println(String("Reproduciendo pista ") + song);
}
void StopMusic(){
  player.stop();
  Serial.println("Deteniendo Módulo DFPlayer");
}
int StartEngine(int time){
  if (actualTime-fewerEngineTime >= time){
    power += (int)pwmSteps;
    analogWrite(engine, power);
    //digitalWrite(engine, HIGH);
    fewerEngineTime = millis();
    //Serial.println((String)"Aumentando Power a: " + power);
  }
  if (power >= pwmEnd){
    engineWorking = false;
  }
}
void StopEngine(int time){
  //Serial.println((String)"Entrando a stop");
  if (!setUpPowerVal){
    power = pwmEnd;
    setUpPowerVal = true;
  }
  if (actualTime-fewerEngineTime >= time){
    //Serial.println((String)"Tiempo medido: " + (actualTime-fewerEngineTime));
    power -= (int)pwmSteps;
    if (power <= pwmStart){
      power = 0;
      //Serial.println((String)"Power en cero: " + power);
      if (player.readState() == playerReady){
        goToSleep = true;
        stoppingEngine = false;
        fumeOnPulseTime = fumeOnPulseTime/2;
      }
    } 
    
    analogWrite(engine, power);
    //Serial.println((String)"Reduciendo Power a: " + power);
    //digitalWrite(engine, LOW);
    fewerEngineTime = actualTime; //millis();
  }

}
void OnLeds(){
  //Serial.println((String)"Estado de fumes: " + analogRead(chimneyState));
  digitalWrite(led1,HIGH);
  digitalWrite(led2,HIGH);
  digitalWrite(led3,HIGH);
  digitalWrite(led4,HIGH);
}
void OffLeds(){
  digitalWrite(led1,LOW);
  digitalWrite(led2,LOW);
  digitalWrite(led3,LOW);
  digitalWrite(led4,LOW); 
}
void StartFumes(){
  int tempOffFumeTime;

  fumeElapseTime = actualTime-fewerFumeTime;
  
  //Activate fumes.
  if (fumeElapseTime <= fumeOnPulseTime){
    digitalWrite(chimney, HIGH);
    //Serial.println((String)"Start High" + fumeElapseTime);
    fumeState = 1;  
  }
  if (fumeElapseTime > fumeOnPulseTime && fumeElapseTime <= fumeOnPulseTime*2){
    digitalWrite(chimney, LOW); 
    //Serial.println((String)"Start LOW" + fumeElapseTime) ;
    fumeState = 2;
  }
 //Now deactivate it.
 tempOffFumeTime = fumeOnPulseTime*2;
  if (fumeElapseTime > tempOffFumeTime && fumeElapseTime <= tempOffFumeTime + fumeOffPulseTime){
    digitalWrite(chimney, HIGH);   
    fumeState = 3;  
  }
  if (fumeElapseTime > tempOffFumeTime + fumeOffPulseTime && fumeElapseTime <= tempOffFumeTime + fumeOffPulseTime*2){
    digitalWrite(chimney, LOW); 
    fumeState = 4;
  }
  if (fumeElapseTime > tempOffFumeTime + fumeOffPulseTime*2 && fumeElapseTime <= tempOffFumeTime + fumeOffPulseTime*3){
    digitalWrite(chimney, HIGH); 
    fumeState = 5;
  }
  if (fumeElapseTime > tempOffFumeTime + fumeOffPulseTime*3 && fumeElapseTime <= tempOffFumeTime + fumeOffPulseTime*4){
    digitalWrite(chimney, LOW); 
    fumeState = 6;
  }
  if (fumeElapseTime > tempOffFumeTime + fumeOffPulseTime*4 && fumeElapseTime <= tempOffFumeTime + fumeOffPulseTime*8){
    //onFumes = true;
    fumeState = 7;
  }
  //Wait in off mode
  if (fumeElapseTime > tempOffFumeTime + fumeOffPulseTime*offFumeMultiplier){
    fewerFumeTime = actualTime;
  }
}
int GetDistance(){
  //delay(50);
  digitalWrite(trigger, LOW);
  delayMicroseconds(4);
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger, LOW);
  sensorVal = pulseIn(sensor, HIGH, 30000); //If echo is lost, it get 30ms as answer more or less 5m.
  //Serial.println((String)"Distancia detectada en: " + sensorVal/58);
  if (sensorVal == 0 || sensorVal == 30000){
    return 200;
  }
  return (int)sensorVal/58;
}
