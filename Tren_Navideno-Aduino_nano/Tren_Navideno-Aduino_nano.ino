  #include <SoftwareSerial.h>
  #include <DFRobotDFPlayerMini.h>
  const byte chimney = 3; //To applty PWM signal to control the fumes.
  const byte engine = 2;
  const byte led1 = 6, led2 = 9, led3 = 10, led4 = 11;
  const byte sensor = A0;
  byte songsQ, fumeCicle;
  int sensorVal;
  int ledStepTime = 250, fumeTime = 700;
  unsigned long fewerFumeTime, fewerLedTime, actualTime;
  byte selectedSong;
  SoftwareSerial serialConnection(1, 0); // RX, TX
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
  selectedSong = 1;
  fumeCicle = 1;
  
  serialConnection.begin(9600);
  Serial.begin(115200); //Start communication between pc and board

  if (!player.begin(serialConnection)) {
    Serial.println("No se pudo encontrar el módulo DFPlayer Mini, asegúrate de la conexión!");
    while (true);
  }
  songsQ = player.readFileCounts();
  Serial.println("Módulo DFPlayer Mini encontrado.");
  
}

void loop() {
  //Verify people presence.
  sensorVal = analogRead(sensor);

  if (sensorVal >= 250){
    player.start();
    //Start engine.
    delay(20);//Wait till player wakes up.
    digitalWrite(engine, HIGH);
    //Play first Song
    player.play(selectedSong);
    if (selectedSong < songsQ){
      selectedSong += 1;
    }else{
      selectedSong = 1;
    }
    delay(20); //Wait for asking again to player.
    fewerFumeTime = millis();//Start counting time.
    fewerLedTime = fewerFumeTime;
    //While busy, show lights and fumes.
    while((player.readState() != Busy)){
      digitalWrite(led1,HIGH);
      digitalWrite(led2,HIGH);
      digitalWrite(led3,HIGH);
      digitalWrite(led4,HIGH);
    }
    while (player.readState() == Busy){
      //Fumes Cicle./////////////////////////////////////
      actualTime = millis();
      if (actualTime-fewerFumeTime >= fumeTime*6){
        fumeCicle = 1;fewerFumeTime = millis();
      } else{ 
        if (actualTime-fewerFumeTime >= fumeTime*3){
            fumeCicle = 4;
        } else{
          if (actualTime-fewerFumeTime >= fumeTime*2){
            fumeCicle = 3;
          } else{
              if (actualTime-fewerFumeTime >= fumeTime){
                fumeCicle = 2;
              }
        }
          }
            }

      //Now Turn on or off the chimney.
      if (fumeCicle == 1 || fumeCicle == 3){digitalWrite(chimney, HIGH); delay(5);digitalWrite(chimney, LOW);}
      if (fumeCicle == 2  || fumeCicle == 4){
        digitalWrite(chimney, HIGH); delay(5);digitalWrite(chimney, LOW);
        digitalWrite(chimney, HIGH); delay(5);digitalWrite(chimney, LOW);}
      //Lights Cicle.//////////////////////////////////////////
      if (actualTime-fewerLedTime <= ledStepTime){digitalWrite(led4,LOW);digitalWrite(led1,HIGH);}
        else{if (actualTime-fewerLedTime <= ledStepTime*2){digitalWrite(led1,LOW);digitalWrite(led2,HIGH);}
          else {if (actualTime-fewerLedTime <= ledStepTime*3){digitalWrite(led2,LOW);digitalWrite(led3,HIGH);}
            else{if (actualTime-fewerLedTime <= ledStepTime*4){digitalWrite(led3,LOW);digitalWrite(led4,HIGH);fewerLedTime = millis();}
              else{if (actualTime-fewerLedTime > ledStepTime*4){fewerLedTime = millis();}    
        }}}}
    }
    //Turn Off everithing.
    digitalWrite(led1,LOW);
    digitalWrite(led2,LOW);
    digitalWrite(led3,LOW);
    digitalWrite(led4,LOW); 
    if (fumeCicle == 1 || fumeCicle == 3){
        digitalWrite(chimney, HIGH); delay(5);digitalWrite(chimney, LOW);
        digitalWrite(chimney, HIGH); delay(5);digitalWrite(chimney, LOW);} 
    //Stop engine when find stop signal.
    while(1){
      if (analogRead(sensor)>600){
        delay(500);//Wait half second and stop.
        digitalWrite(engine, LOW);
        break;
      } 
    }
    delay(30000); //Wait 30s till next reading.                                                                         
  }
  player.sleep();
}
