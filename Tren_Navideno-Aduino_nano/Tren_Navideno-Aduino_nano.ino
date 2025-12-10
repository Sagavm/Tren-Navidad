  #include <SoftwareSerial.h>
  #include <DFRobotDFPlayerMini.h>
  const byte chimney = 2; 
  const byte engine = 3; //To applty PWM signal to control the speed.
  const byte led1 = 9, led2 = 10, led3 = 11, led4 = 12;
  const byte sensor = A0, trigger = A1, stationSound = 1, breakSound = 2, startingSong = 3;
  const int playerBusy = 513, playerReady = 512;
  byte songsQ;
  long sensorVal;
  int ledStepTime = 250, fumeTime = 100;
  unsigned long fewerFumeTime, fumeElapseTime,fewerLedTime, actualTime;
  byte selectedSong;
  SoftwareSerial serialConnection(6, 5); // RX, TX 5
  DFRobotDFPlayerMini player;

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
  selectedSobg = 2;////////////////////////////////////////////////////////
  bool dfPlayerStarted = false;
  serialConnection.begin(9600);
  Serial.begin(9600); //Start communication between pc and board

  if (!player.begin(serialConnection)) {
    Serial.println("No se pudo encontrar el módulo DFPlayer Mini, asegúrate de la conexión!");
    while (true);
  }
  songsQ = player.readFileCounts();
  Serial.println(songsQ);
  Serial.println("Módulo DFPlayer Mini encontrado.");
}

void loop() {
  //Verify people presence.
  Serial.println(getDistance());
  delay(2000);
  if (getDistance()<=60){
    //Start engine.
    delay(20);//Wait till player wakes up.
    digitalWrite(engine, HIGH);
    //Play first Song
    player.play(selectedSong);
    if (selectedSong < songsQ){
      selectedSong += 1;
    }else{
      selectedSong = breakSound; ///////////////////////////////////////////////////// startingSong
    }
    delay(30); //Wait for asking again to player.
    fewerFumeTime = millis();//Start counting time.
    fewerLedTime = fewerFumeTime;
    //While busy, show lights and fumes.
    
    while((player.readState() == playerReady)){
      digitalWrite(led1,HIGH);
      digitalWrite(led2,HIGH);
      digitalWrite(led3,HIGH);
      digitalWrite(led4,HIGH);
      delay(300);
    }
    while (player.readState() == playerBusy){
      //Fumes Cicle./////////////////////////////////////
      actualTime = millis();
      fumeElapseTime = actualTime-fewerFumeTime;
      startFumes();
      stopFumes();
      resetFumeTime();
      //Lights Cicle.//////////////////////////////////////////
      if (actualTime-fewerLedTime <= ledStepTime){digitalWrite(led4,LOW);digitalWrite(led1,HIGH);}
        else{if (actualTime-fewerLedTime <= ledStepTime*2){digitalWrite(led1,LOW);digitalWrite(led2,HIGH);}
          else {if (actualTime-fewerLedTime <= ledStepTime*3){digitalWrite(led2,LOW);digitalWrite(led3,HIGH);}
            else{if (actualTime-fewerLedTime <= ledStepTime*4){digitalWrite(led3,LOW);digitalWrite(led4,HIGH);fewerLedTime = millis();}
              else{if (actualTime-fewerLedTime > ledStepTime*4){fewerLedTime = millis();}    
        }}}}
        //Serial.println("En el While Busy");
        //Serial.println(player.readState());
    }
    //Turn Off everything.
    digitalWrite(led1,LOW);
    digitalWrite(led2,LOW);
    digitalWrite(led3,LOW);
    digitalWrite(led4,LOW); 
    delay(20);
    player.play(stationSound); //Train running till station.
    //Stop engine when find stop signal.
    while(1){
      actualTime = millis();
      fumeElapseTime = actualTime-fewerFumeTime;
      startFumes();
      stopFumes();
      resetFumeTime();
      if (getDistance()<=6){
        player.stop();
        delay(500);
        player.play(breakSound); //Break.
        delay(500);//Wait half second and stop.
        digitalWrite(engine, LOW);
        player.stop();
        break;
      } 
    }
    delay(30000); //Wait 30s till next reading.                                                                         
  }
}

void startFumes(){
  if (digitalRead(chimney) == LOW && fumeElapseTime <= 40){
    digitalWrite(chimney, HIGH);}
  if (digitalRead(chimney) == HIGH && fumeElapseTime >= 40){
    digitalWrite(chimney, LOW);
  }
}
void stopFumes(){
  //Off Cicle.
  if (digitalRead(chimney) == LOW && fumeElapseTime >= fumeTime && fumeElapseTime <= fumeTime +40){
    if (digitalRead(chimney) == LOW && fumeElapseTime <= fumeTime + 40){
      digitalWrite(chimney, HIGH);}
    if (digitalRead(chimney) == HIGH && fumeElapseTime >= fumeTime + 40){
      digitalWrite(chimney, LOW);
    }
  }
  if (digitalRead(chimney)== LOW && fumeElapseTime >= fumeTime +50 && fumeElapseTime <= fumeTime + 90){
    if (digitalRead(chimney)== LOW && fumeElapseTime <= fumeTime + 90){
      digitalWrite(chimney, HIGH);}
    if (digitalRead(chimney)== HIGH && fumeElapseTime >= fumeTime + 85){
      digitalWrite(chimney, LOW);
    }
  }
}
void resetFumeTime(){
  if (fumeElapseTime >= fumeTime*2){
    fewerFumeTime = millis();
  }
}
int getDistance(){
  digitalWrite(trigger, LOW);
  delayMicroseconds(4);
  digitalWrite(trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger, LOW);
  sensorVal = pulseIn(sensor, HIGH);
  return sensorVal/58;
}