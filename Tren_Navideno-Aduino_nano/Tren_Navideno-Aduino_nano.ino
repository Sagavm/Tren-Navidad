  #include <SoftwareSerial.h>
  #include <DFRobotDFPlayerMini.h>
  const byte chimney = 2; 
  const byte engine = 3; //To applty PWM signal to control the speed.
  const byte led1 = 9, led2 = 10, led3 = 11, led4 = 13;
  const byte sensor = A0;
  const int playerBusy = 513;
  const int playerReady = 512;
  byte songsQ;
  int sensorVal;
  int ledStepTime = 250, fumeTime = 100;
  unsigned long fewerFumeTime, fewerLedTime, actualTime;
  byte selectedSong;
  SoftwareSerial serialConnection(6, 5); // RX, TX
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
  bool dfPlayerStarted = false;
  serialConnection.begin(9600);
  Serial.begin(9600); //Start communication between pc and board

  if (!player.begin(serialConnection)) {
    Serial.println("No se pudo encontrar el módulo DFPlayer Mini, asegúrate de la conexión!");
    while (true);
  }
  songsQ = 3;player.readFileCounts();
  Serial.println("Módulo DFPlayer Mini encontrado.");
  
}

void loop() {
  //Verify people presence.
  sensorVal = analogRead(sensor);
  Serial.println("Sensor = ");
  Serial.println(sensorVal);
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
    
    while((player.readState() == playerReady)){
      digitalWrite(led1,HIGH);
      digitalWrite(led2,HIGH);
      digitalWrite(led3,HIGH);
      digitalWrite(led4,HIGH);
      Serial.println("Otro estado");
      Serial.println(player.readState());
      //delay(300);
    }
    while (player.readState() == playerBusy){
      //Fumes Cicle./////////////////////////////////////
      actualTime = millis();
      if (digitalRead(chimney) == LOW && actualTime-fewerFumeTime <= 20){
        digitalWrite(chimney, HIGH);}
      if (digitalRead(chimney) == HIGH && actualTime-fewerFumeTime >= 20){
        digitalWrite(chimney, LOW);
      }
      //Off Cicle.
      if (digitalRead(chimney) == LOW && actualTime-fewerFumeTime >= fumeTime && actualTime-fewerFumeTime <= fumeTime +20){
        if (digitalRead(chimney) == LOW && actualTime-fewerFumeTime <= fumeTime + 20){
          digitalWrite(chimney, HIGH);}
        if (digitalRead(chimney) == HIGH && actualTime-fewerFumeTime >= fumeTime + 20){
          digitalWrite(chimney, LOW);
        }
      }
      if (digitalRead(chimney)== LOW && actualTime-fewerFumeTime >= fumeTime +25 && actualTime-fewerFumeTime <= fumeTime +47){
        if (digitalRead(chimney)== LOW && actualTime-fewerFumeTime <= fumeTime + 45){
          digitalWrite(chimney, HIGH);}
        if (digitalRead(chimney)== HIGH && actualTime-fewerFumeTime >= fumeTime + 45){
          digitalWrite(chimney, LOW);
        }
      }
      if (actualTime-fewerFumeTime >= fumeTime*2){
        fewerFumeTime = millis();
        Serial.println("tIEMPO RESETEADO");
      }

      /*
      if (actualTime-fewerFumeTime >= fumeTime*6){
        digitalWrite(chimney, HIGH); delay(5); digitalWrite(chimney, LOW);
        fewerFumeTime = millis();
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
      if (fumeCicle == 1 || fumeCicle == 3){digitalWrite(chimney, HIGH); delay(5); digitalWrite(chimney, LOW);}
      if (fumeCicle == 2  || fumeCicle == 4){digitalWrite(chimney, HIGH); delay(5); digitalWrite(chimney, LOW);
        digitalWrite(chimney, HIGH); delay(5); digitalWrite(chimney, LOW);}*/
      //Lights Cicle.//////////////////////////////////////////
      if (actualTime-fewerLedTime <= ledStepTime){digitalWrite(led4,LOW);digitalWrite(led1,HIGH);}
        else{if (actualTime-fewerLedTime <= ledStepTime*2){digitalWrite(led1,LOW);digitalWrite(led2,HIGH);}
          else {if (actualTime-fewerLedTime <= ledStepTime*3){digitalWrite(led2,LOW);digitalWrite(led3,HIGH);}
            else{if (actualTime-fewerLedTime <= ledStepTime*4){digitalWrite(led3,LOW);digitalWrite(led4,HIGH);fewerLedTime = millis();}
              else{if (actualTime-fewerLedTime > ledStepTime*4){fewerLedTime = millis();}    
        }}}}
        Serial.println("En el While Busy");
        Serial.println(player.readState());
    }
    //Turn Off everithing.
    digitalWrite(led1,LOW);
    digitalWrite(led2,LOW);
    digitalWrite(led3,LOW);
    digitalWrite(led4,LOW); 
    Serial.println("Fuera del While");

    //Stop engine when find stop signal.
    /*while(1){
      if (analogRead(sensor)>600){
        delay(500);//Wait half second and stop.
        digitalWrite(engine, LOW);
        break;
      } 
    }*/
    delay(30000); //Wait 30s till next reading.                                                                         
  }
  player.sleep();
}
