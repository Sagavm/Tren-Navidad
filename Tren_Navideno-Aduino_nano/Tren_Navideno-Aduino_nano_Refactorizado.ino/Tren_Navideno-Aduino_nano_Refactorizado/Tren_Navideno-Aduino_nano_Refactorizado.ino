#include <SoftwareSerial.h>
  #include <DFRobotDFPlayerMini.h>
  const byte chimney = 2; 
  const byte engine = 3; //To applty PWM signal to control the speed.
  const byte led1 = 9, led2 = 10, led3 = 11, led4 = 12;
  const byte sensor = A0, trigger = A1, stationSound = 1, breakSound = 2, startingSong = 3;
  const int playerBusy = 513, playerReady = 512;
  const int minDist = 40, stopDist = 5;
  const int accelerationTime = 2000, stoppingTime = 2000, detectPersonTime = 1000, stopSensor = 2000;
  byte actualSong = 3, lastLapSong = 1, stoppingSong = 2, songsQ;
  long sensorVal;
  const int ledStepTime = 250, fumeTime = 100;
  unsigned long fewerPersonTime, fewerSensorTime, fewerFumeTime, fumeElapseTime,fewerLedTime, actualTime, fewerEngineTime;
  int counter;
  SoftwareSerial serialConnection(6, 5); // RX, TX 5
  DFRobotDFPlayerMini player;
  bool person, stopSignal, playM, stopM, engineWorking, onLeds, onFumes, stoppingEngine, goToSleep, fumeStopped, playingSong; 
  //To control starting engine
  const int steps = 200;         // softness resolution
  const float dt = (float)accelerationTime / steps;
  const float pwmStart = 0.30 * 255.0;  // 30% = 76.5
  const float pwmEnd    = 0.98 * 255.0;  // 98% = 249.9
  //To control stopping engine
  const float startPWM = 0.98 * 255;   // 98%
  const float endPWM   = 0;            // 0%
  const unsigned long duration = 15000; // 15 segundos


void setup() {
  // put your setup code here, to run once:
  pinMode(chimney, OUTPUT);
  pinMode(engine, OUTPUT);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);
  pinMode(sensor, INPUT);
  pinMode(trigger,OUTPUT);
   ////////////////////////////////////////////////////////
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
}

void loop() {
  // put your main code here, to run repeatedly:
  actualTime = millis();
  if (person){
    while (actualTime - fewerPersonTime <= detectPersonTime){
      if (DetectPerson()){
        playM = true;         //Play music
        engineWorking = true; //Start engine etc.
        onLeds = true;
        onFumes = true;
        person = false;
        fewerLedTime = millis();
        fewerFumeTime = millis();
        return;
      }
    }
    delay(1000);
    fewerPersonTime = millis();
  }
  if (playM){
    if (actualSong > songsQ){
      actualSong = 3;
    }
    PlayMusic(actualSong);
    actualSong += 1;
    playM = false;
  }
  if (engineWorking){
    StartEngine();
    playingSong =  true;
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
      onLeds = true;
      fewerLedTime = millis();
    }
  }
  if (onFumes){
    if (actualTime - fewerFumeTime <= fumeTime){
      StartFumes();
    }else{
      onFumes = false;
      fumeStopped = false;
      fewerFumeTime = millis();
    }
  }
  if (!onFumes){
    if (!fumeStopped){
      StopFumes();
      fumeStopped = true;
    }
    if (actualTime - fewerFumeTime <= fumeTime){
      delay(1);
    }else{
      onFumes = true;
      fewerFumeTime = millis();
      }
  }
  //Verify playing status, if it is ready, it means that ended last song; so stop everithing
  if (player.readState() == playerReady && playingSong){
    //Play last lap song; number 1.
    PlayMusic(lastLapSong);
    stopSignal = true;
    stoppingEngine = true;
    playingSong = false;
  }
  if (stopSignal){
    if(GetDistance()<=5 && stoppingEngine){
      StopMusic();
      delay(10);
      PlayMusic(stoppingSong);
      fewerEngineTime = millis();
    }
    if (stoppingEngine){
      StopEngine(fewerEngineTime);
      if(goToSleep){
        OffLeds();
        onLeds = false;
        StopFumes();
        onFumes = false;
        stopSignal = false;
        person = true;
        goToSleep = false;
        delay(30000);
        fewerPersonTime = millis();
      }
    }
  }
}

bool DetectPerson(){
  if (GetDistance()<= minDist){
    return true;
  }
  return false;
}
bool DetectStop(){
    if (GetDistance()<= stopDist){
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
void StartEngine(){
  if (counter <= steps){
    // esponential acceleration
    float t = (float)counter / steps;      // 0 → 1
    float curve = pow(t, 2.8);       // EXPONENTIAL (2.8 = soft)
    int pwm = pwmStart + curve * (pwmEnd - pwmStart);
    analogWrite(engine, pwm);
    delay(dt);
    counter +=1;
  }
  if (counter > steps){
    counter = 0;
    engineWorking = false;
  }
}
void StopEngine(unsigned long startTime){
    Serial.println("Deteniendo Motor");
    unsigned long elapsed = millis() - startTime;
    // Progreso 0.0 → 1.0
    float t = (float)elapsed / duration;
    // Curva exponencial inversa: desaceleración suave
    // Fórmula: y = (1 - e^( -α * t )) 
    // Ajuste α = 4 para una curva bonita
    float alpha = 4.0;
    float curve = exp(-alpha * t);
    // Valor PWM interpolado
    float pwmValue = endPWM + (startPWM - endPWM) * curve;
    analogWrite(engine, (int)pwmValue);
    delay(20);  // 50 actualizaciones por segundo
    if (elapsed >= duration) {
        analogWrite(engine, 0);   // detener completamente
        goToSleep = true;// salir de la función
    }
}
void OnLeds(){
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
  digitalWrite(chimney, HIGH);
  delay(20);
  digitalWrite(chimney, LOW);
}
void StopFumes(){
  digitalWrite(chimney, HIGH);
  delay(20);
  digitalWrite(chimney, LOW);
  delay(20);
  digitalWrite(chimney, HIGH);
  delay(20);
  digitalWrite(chimney, LOW);
}
int GetDistance(){
  digitalWrite(trigger, LOW);
  delayMicroseconds(4);
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger, LOW);
  sensorVal = pulseIn(sensor, HIGH);
  return sensorVal/58;
}
