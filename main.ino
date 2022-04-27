#include <TMCStepper.h> //probably need to download this library
using namespace std;

// serial commands that the raspberry pi might send to the arduino
#define CMD_UNKNOWN 0 
#define CMD_PING    1
#define CMD_EJECT   2

// Motor
#define ENA 5 //OUTPUT: enable on pin D5
#define DIR 6 //OUTPUT: direction on pin D5
#define PUL 7 //OUTPUT: pulse on pin D5

// Sensors
#define EXTEND 11 //INPUT: pin 11
#define BED_CLEAR 12 //INPUT: pin 12
#define HOME 10 //INPUT: pin 10 (used to be 13, but 13 is used by bulitin LED)

// Other pin
#define PIN_LED LED_BUILTIN

void setup() {
  Serial.begin(115200); //baud rate of printer

  // I/O pins
  pinMode(PIN_LED, OUTPUT); // set all pins to either input or output
  pinMode(PUL, OUTPUT);
  pinMode(DIR, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(EXTEND, INPUT_PULLUP);// define pin as Input  sensor
  pinMode(BED_CLEAR, INPUT_PULLUP);
  pinMode(HOME, INPUT_PULLUP);

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB
  }
}

// function to return if sensor detects anything
int extend_pos_sensor(){ //pin 11
  int L = digitalRead(EXTEND); 
  if(L == 0){ //obstacle detected
    return 0;
    }
  else{
    return 1;
  }
}

// function to return if sensor detects anything
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

// function to return if sensor detects anything
int home_pos_sensor(){ //was pin 13 now 10
  int L = digitalRead(HOME);
  if(L == 0){ //obstacle detected
    return 0;
    }
  else{
    return 1;
  }
}

// function returns the command that was sent by the PI (defined above)
int parseStringCommand(String stringCommand){ 
    if (stringCommand.equals("PING")){
      return CMD_PING;
    } else if (stringCommand.equals("EJECT")){
      return CMD_EJECT;
    } else {
      return CMD_UNKNOWN;
    }
}

// function that controls motor movement 
void eject(){

  int there_was_an_error = 0; //keep track of if there was an issue with removal
  
  digitalWrite(PIN_LED,HIGH); // random delay used for testing, delete if needed
  delay(1000);

  digitalWrite(PIN_LED, LOW);   
  delay(1000);  
  
  while((extend_pos_sensor() != 0) && (bed_clear_sensor() != 0)){ //while extend and bed clear do not detect anything
      digitalWrite(DIR,HIGH); //technically moves backwards (away from motor driver)
      digitalWrite(ENA,HIGH);
      digitalWrite(PUL,HIGH);
      delayMicroseconds(50);
      digitalWrite(PUL,LOW);
      delayMicroseconds(50);
  }
  
  if (bed_clear_sensor() == 0){ // if anything was detected on the print bed
    Serial.println("ERROR"); // send the PI that there was an error (cause the revomal to happen up to 2 more times)
    there_was_an_error++; //add to counter that an error was detected
  }
  
   while(home_pos_sensor() != 0){ //while home sensor doesn't detect anything
      digitalWrite(DIR,LOW); //technically moves forward (towards motor driver)
      digitalWrite(ENA,HIGH);
      digitalWrite(PUL,HIGH);
      delayMicroseconds(50);
      digitalWrite(PUL,LOW);
      delayMicroseconds(50); 
   }
    
    if(there_was_an_error == 0){ // if no error was detected
      Serial.println("END"); // send the PI that print was successfully removed
    }
    there_was_an_error = 0; //reset counter
}
     //Hard code to move forward then back 
     //this was for testing purposes, but this could be used if sensors are not needed
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
  if (Serial.available() > 0) { //constantly check if PI is sending any command
    String stringCommand = Serial.readStringUntil('\n'); //read the string until \n
     int command = parseStringCommand(stringCommand);// call function to decode which command was sent
    switch (command) { //switch statment used for command sent
      case CMD_PING: //used to check proper communication with PI
        Serial.println("PONG");
        break;
      case CMD_EJECT:
        Serial.println("START"); //tell PI that ejection is starting
        eject(); //function call
        break;
      default:
      case CMD_UNKNOWN:
        Serial.println("UNKNOWN COMMAND");
    }
  }
}
