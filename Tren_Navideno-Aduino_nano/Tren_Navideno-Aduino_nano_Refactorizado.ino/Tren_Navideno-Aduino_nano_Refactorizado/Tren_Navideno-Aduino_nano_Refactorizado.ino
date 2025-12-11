#include <SoftwareSerial.h>
  #include <DFRobotDFPlayerMini.h>
  const byte chimney = 2; 
  const byte engine = 3; //To applty PWM signal to control the speed.
  const byte led1 = 9, led2 = 10, led3 = 11, led4 = 13; ///////////////////////////////////////////////////Cambiar al final
  const byte sensor = A0, trigger = A1, stationSound = 1, breakSound = 2, startingSong = 3;
  const int playerBusy = 513, playerReady = 512;
  const int minDist = 40, stopDist = 5;
  const int accelerationTime = 2000, stoppingTime = 2000, detectPersonTime = 1000, stopSensor = 2000;
  byte actualSong = 2, lastLapSong = 1, stoppingSong = 2, songsQ;
  long sensorVal;
  const int ledStepTime = 250, fumePulseTime = 40;
  unsigned long fewerPersonTime, fewerSensorTime, fewerFumeTime, fumeElapseTime,fewerLedTime, actualTime, fewerEngineTime;
  int counter;
  SoftwareSerial serialConnection(6, 5); // RX, TX 5
  DFRobotDFPlayerMini player;
  bool person, stopSignal, playM, stopM, engineWorking, onLeds, onFumes, stoppingEngine, goToSleep, playingActualSong, setUpPowerVal, songStopped; 
  //To control starting engine (Time in ms)
  int startEngineTime = 4000, stopEngineTime = 15000, engineSteps = 200, startStepsTime, stopStepsTime, power;
  const float pwmStart = 76.5, pwmEnd = 250, pwmSteps = 250/76.5;


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
  startStepsTime = (int)startEngineTime/engineSteps;
  stopStepsTime = (int)stopEngineTime/engineSteps;
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
    playingActualSong =  true;
    fewerEngineTime = millis();
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
  if (!onFumes){
    StopFumes();
  }
  //Verify playing actual song status, if it is ready, it means that ended last song; so stop everything
  if (player.readState() == playerReady && playingActualSong){
    //Play last lap song; number 1.
    PlayMusic(lastLapSong);
    stopSignal = true;
    playingActualSong = false;
  }
  if (stopSignal && player.readState() == playerBusy){
    //If found stop signal, stop song.
    if(GetDistance() <= 5 || songStopped){
      if (!songStopped){
        StopMusic();
        songStopped = true;
      }
    //This stops everything even if stop signal is not found.//////////////////////////////////////////////
      if (player.readState() == playerReady){
        PlayMusic(stoppingSong);
        //Verify state of fumes.
        stoppingEngine = true;
        //Avoid distance detection and state  of the player again.
        songStopped = false;
        stopSignal = false;
        fewerEngineTime = millis();
      }
    }
  }
  if (stoppingEngine){
    StopEngine(stopStepsTime); 
  }
  if(goToSleep && onFumes){
        FinalStopFumes();
        onFumes = false;
        stopSignal = false;
        person = true;
        goToSleep = false;
        setUpPowerVal = false; //For stopping engine in next song.
        delay(30000);
        fewerPersonTime = millis();
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
int StartEngine(int time){
  if (actualTime-fewerEngineTime >= time){
    power += (int)pwmSteps;
    analogWrite(engine, power);
    fewerEngineTime = millis();
  }
  if (power >= pwmEnd){
    engineWorking = false;
  }
}
void StopEngine(int time){
  if (!setUpPowerVal){
    power = pwmEnd;
    setUpPowerVal = true;
  }
  if (actualTime-fewerEngineTime >= time){
    power -= (int)pwmSteps;
    if (power <= pwmStart){
      power = 0;
      stoppingEngine = false;
      goToSleep = true;
    } 
    analogWrite(engine, power);
    fewerEngineTime = millis();
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
  fumeElapseTime = actualTime-fewerFumeTime;
  //25ms on
  if (fumeElapseTime <= fumePulseTime){
    digitalWrite(chimney, HIGH);     
  }else{
    //75ms off (25 to 100)
    if (fumeElapseTime <= fumePulseTime*4){
      digitalWrite(chimney, LOW); 
    }else{
      onFumes = false;
      fewerFumeTime = millis();
    }
  }
}
void StopFumes(){
  fumeElapseTime = actualTime-fewerFumeTime;
  //First Pulse
  //25ms on
  if (fumeElapseTime <= fumePulseTime){
    digitalWrite(chimney, HIGH);     
  }
  //25ms off (25 to 50)
  if (fumeElapseTime > fumePulseTime && fumeElapseTime <= fumePulseTime*2 ){
    digitalWrite(chimney, LOW); 
  }
  //Second Pulse
  //50 to 75
  if (fumeElapseTime > fumePulseTime*2 && fumeElapseTime <= fumePulseTime*3 ){
    digitalWrite(chimney, HIGH); 
  }
  //75 to 100
  if (fumeElapseTime > fumePulseTime*3 && fumeElapseTime <= fumePulseTime*4 ){
    digitalWrite(chimney, LOW); 
  }else{
    onFumes = true;
    fewerFumeTime = millis();
  }
}
void FinalStopFumes(){
  //Final turn off, no matter synchronization.
  if (digitalRead(chimney)==HIGH){
    digitalWrite(chimney, LOW);
  }
  delay(25);
  digitalWrite(chimney, HIGH);
  delay(25);
  digitalWrite(chimney, LOW);
  delay(25);
  digitalWrite(chimney, HIGH);
  delay(25);
  digitalWrite(chimney, LOW);
}

int GetDistance(){
  digitalWrite(trigger, LOW);
  delayMicroseconds(4);
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger, LOW);
  sensorVal = pulseIn(sensor, HIGH);
  Serial.println((String)"Distancia detectada en: " + sensorVal/58);
  return sensorVal/58;
}
