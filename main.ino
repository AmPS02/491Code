#include <TMCStepper.h>
using namespace std;

// Serial commands
#define CMD_UNKNOWN 0
#define CMD_PING    1
#define CMD_HOME    2
#define CMD_END     3
#define CMD_EJECT   4
#define CMD_LED_ON  5
#define CMD_LED_OFF 6

// Motor 1
#define ENA 5
#define DIR 6
#define PUL 7
#define EXTEND 11 
#define BED_CLEAR 12
#define HOME 10 //13

// Other pin
#define PIN_LED LED_BUILTIN

void setup() {
  Serial.begin(115200);

  // Fan & leds relays
  pinMode(PIN_LED, OUTPUT);
  pinMode (PUL, OUTPUT);
  pinMode (DIR, OUTPUT);
  pinMode (ENA, OUTPUT);
  pinMode(EXTEND, INPUT_PULLUP);// define pin as Input  sensor
  pinMode(BED_CLEAR, INPUT_PULLUP);
  pinMode(HOME, INPUT_PULLUP);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }
  // Test relays
  //digitalWrite(PIN_LED, HIGH); 
  //delay(1000);
  //digitalWrite(PIN_LED, LOW);
  // Enable SPI
  //SPI.begin();
  //pinMode(MISO, INPUT_PULLUP);
  
}

int extend_pos_sensor(){ //pin 11
  int L = digitalRead(EXTEND);
  if(L == 0){ //obstacle detected
    return 0;
    }
  else{
    return 1;
  }
}
int bed_clear_sensor(){ //pin 12
  // if needed add a delay here to not detect the printer itself 
  int L = digitalRead(BED_CLEAR);
  if(L == 0){ //obstacle detected
    return 0;
    }
  else{
    return 1;
  }
}
int home_pos_sensor(){ //pin 13 now 10
  int L = digitalRead(HOME);
  if(L == 0){ //obstacle detected
    return 0;
    }
  else{
    return 1;
  }
}

int parseStringCommand(String stringCommand){
    if (stringCommand.equals("PING")){
      return CMD_PING;
    } else if (stringCommand.equals("HOME")){
      return CMD_HOME;
    } else if (stringCommand.equals("END")){
      return CMD_END;
    } else if (stringCommand.equals("EJECT")){
      return CMD_EJECT;
    } else if (stringCommand.equals("LED ON")) {
      return CMD_LED_ON;
    } else if (stringCommand.equals("LED OFF")) {
      return CMD_LED_OFF;
    } else {
      return CMD_UNKNOWN;
    }
}

void eject(){

  int there_was_an_error = 0;
  
  digitalWrite(PIN_LED,HIGH);
  delay(3000);

  digitalWrite(PIN_LED, LOW);   
  delay(1000);  
  
  while((extend_pos_sensor() != 0) && (bed_clear_sensor() != 0)){ //while extend and bed clear do not detect anything
      digitalWrite(DIR,HIGH);
      digitalWrite(ENA,HIGH);
      digitalWrite(PUL,HIGH);
      delayMicroseconds(50);
      digitalWrite(PUL,LOW);
      delayMicroseconds(50);
  }
  
  if (bed_clear_sensor() == 0){
    Serial.println("ERROR");
    there_was_an_error++;
    
  }
   while(home_pos_sensor() != 0){ //while home sensor doesn't detect anything
      digitalWrite(DIR,LOW);
      digitalWrite(ENA,HIGH);
      digitalWrite(PUL,HIGH);
      delayMicroseconds(50);
      digitalWrite(PUL,LOW);
      delayMicroseconds(50); 
   }
    
    if(there_was_an_error == 0){ 
      Serial.println("END"); 
    }
    there_was_an_error = 0;
}
     //Hard code to move forward then back
//  for(int j=0; j<10; j++){
//    for(int i=0; i<30000; i++)     //Backwards
//    {
//    digitalWrite(PIN_LED,HIGH);
//    
//    digitalWrite(DIR,HIGH);
//    digitalWrite(ENA,HIGH);
//    digitalWrite(PUL,HIGH);
//    delayMicroseconds(50);
//    digitalWrite(PUL,LOW);
//    delayMicroseconds(50);
//    }
//  }
//  for(int j=0; j<10; j++){  
//    for (int i=0; i<30000; i++)   //Forward
//    {
//    digitalWrite(PIN_LED, LOW);
//    
//    digitalWrite(DIR,LOW);
//    digitalWrite(ENA,HIGH);
//    digitalWrite(PUL,HIGH);
//    delayMicroseconds(50);
//    digitalWrite(PUL,LOW);
//    delayMicroseconds(50);
//   }
//  }

void loop() {
  if (Serial.available() > 0) {
    String stringCommand = Serial.readStringUntil('\n');
     int command = parseStringCommand(stringCommand);
    switch (command) {
      case CMD_PING:
        Serial.println("PONG");
        break;
      case CMD_END:
        Serial.println("START");
        Serial.println("END");
        break;
      case CMD_EJECT:
        Serial.println("START");
        eject();
        //Serial.println("END");
        break;
      case CMD_LED_ON:
        //digitalWrite(PIN_LED, HIGH); 
        Serial.println("DONE");
        break;
      case CMD_LED_OFF:
        //digitalWrite(PIN_LED, LOW); 
        Serial.println("DONE");
        break;
      default:
      case CMD_UNKNOWN:
        Serial.println("UNKNOWN COMMAND");
    }
  }
}
