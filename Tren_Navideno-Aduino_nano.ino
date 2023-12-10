  #include <SoftwareSerial.h>
  #include <DFRobotDFPlayerMini.h>
  const byte chimney = 3; //To applty PWM signal to control the fumes.
  const byte engine = 2;
  const byte led1 = 6, led2 = 9, led3 = 10, led4 = 11;
  const byte sensor = A0;
  byte songsQ;
  int sensorVal;
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
    digitalWrite(engine, HIGH);
    //Play first Song
    player.play(selectedSong);
    if (selectedSong < songsQ){
      selectedSong += 1;
    }else{
      selectedSong = 1;
    }
    //While busy, show lights and fumes.
    while (player.readState() == Busy){
      //Start fumes.
      digitaWrite(chimney, HIGH);

    }                                                                                   

    

    
    
  }
  player.sleep();
}
