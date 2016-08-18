//Shooting Gallery Controller
//Written by Christian Donica
//Being used by Cub Scouts Pack 3001, Medford, OR
//August 16, 2016

/*
Made for the Arduino Nano
Pinout:
Analog:
  A0 = Piezo sensor 1 to detect hits on targets.  The other wire on the Piezo goes to GND (because this pin is high)
  A1 = Piezo sensor 2 to detect hits on targets.  The other wire on the Piezo goes to GND (because this pin is high)
  A2 = Piezo sensor 3 to detect hits on targets.  The other wire on the Piezo goes to GND (because this pin is high)
  A3 = Piezo sensor 4 to detect hits on targets.  The other wire on the Piezo goes to GND (because this pin is high)
  A4 = I2C SDA (data) Pin, connected to A4 on other Nano (also need to connect GNDs on both Nanos)
  A5 = I2C SCL (clock) Pin, connected to A5 on other Nano (also need to connect GNDs on both Nanos)
  A6 = Piezo sensor 5 to detect hits on targets.  The other wire on the Piezo goes to GND (because this pin is high)
  A7 = Do not use!  Left unconnected for Random Seed function

 ?? = Mode change button.  The other wire on the button goes to GND (because this pin is high)

Digital:
  D1 =  Tx, which is used for sending Serial data to computer  (this is the order they are on the Nano!)
  D0 =  Mode change button.  The other wire on the button goes to GND (because this pin is high).  Also Rx, which is used for receiving Serial data from computer.
  D2 - D6 = Servo control signals for Servos 1 through 5
  D7 =  Light(s) pin.  The other wire on the Lights goes to GND (because pin is high)
  D8 =  Start/reset Button pin.  The other wire on the Button goes to GND (because this pin is high)
  D9 =  Speaker pin - the other speaker wire goes to Vcc.  Must be a PWM pin
  D10 = SD Card adapter Chip Select (CS) pin (SS)
  D11 = SD Card adapter MOSI pin (and also to other Arduino for vs. mode)
  D12 = SD Card adapter MISO pin (and also to other Arduino for vs. mode)
  D13 = SD Card adapter SCK pin (and also to other Arduino for vs. mode)
  
Note: The SD Card adapter also needs to be connected to the Nano's GND and Vcc (5v) pins
Note: A6 and A7 do NOT work good as button inputs (digital read) because they float between 0 and 1024 quite a bit
*/

#include <Servo.h>
#include <SPI.h>      //Used to connect to the SD adapter card
#include <SD.h>       //SD Card library
#include <TMRpcm.h>   //Sound Library
#include <Wire.h>     //I2C library to communicate with other Nano

//Define some constants
#define RND_SEED_PIN     A7         //Left unconnected for the Random Seed Function
#define MODE_PIN         0          //The Mode Change button pin.  The other wire of the button is connected to GND.
#define LIGHT_PIN        7          //The LED light is connected to this pin.  The other wire of the light goes to GND
#define BUTTON_PIN       8          //The start/reset button pin.  The other wire of the button is connected to GND.
#define SPEAKER_PIN      9          //The speaker is connected to this pin (the other end of the speaker goes to Vcc 5v).  Must be a PWNM pin
#define SD_CS_PIN        10         //For SD Adapter card CS (SS) pin
#define TOTAL_SERVOS     5          //Change this to match your set up
#define CLOCKWISE        1000       //The Pulsewidth to move a servo all the way Clockwise (right) (out of 180 degrees).  Modify this up or down to adjust range of movement.  Default is 1000, but 1100 works better on my Parallax servo.
#define CENTER           1500       //The Pulsewidth to move a servo to the center (out of 180 degrees).  Default is 1500. (This is not really used in this program, but here for future needs.)
#define COUNTERCLOCKWISE 2000       //The Pulsewidth to move a servo all the way Counterclockwise (out of 180 degrees).  Default is 2000.
#define GAME_CYCLES      4          //This is the number of times the game will cycle through the targets until it shuts down and waits for the start button to be pressed again
#define PIEZOTHRESHOLD   130        // analog threshold for piezo sensing
#define SOUND_VOLUME     3          //Sets the sound output (1-7) 7 is pretty distorted
#define FAST_LIGHT       500        //1/2 second delay for a fast light blink
#define SLOW_LIGHT       1000       //1 second delay for a slow light blink
#define DEBUGGING        true       //True if you want messages sent to the Serial Monitor

//Set which Nano this sketch is running on... Masteror Slave
#define NANO_IS_MASTER   true       //True if this sketch is running on the Master, or change it to false if this is the Slave
#define I2C_ADDR         7          //This Nano's I2C address (need to reverse on the other nano) for vs. mode
#define I2C_SLAVE_ADDR   8          //The other Nano's I2C (need to reverse on the other nano) for vs. mode

//Set up same variables
byte TARGET_DELAY =      2;         //Delay, in seconds, between targets moving, showing, and hiding
byte gameMode =          1;         //Tracks the game mode:  1 = Normal (target_delay = 2), 2 = Fast (target_delay = 1), 3 = Stationary, 4 = versus
byte I2C_send_buf =      0;         //Data to be sent to other Nano in vs. mode
byte I2C_recv_buf =      0;         //Data to be received from other Nano in vs. mode

// The number of Sound Files available (change if you add more)
#define NumRicoFiles     23     //rico1.wav ... rico23.wav
#define NumWinFiles      3      //win1.wav ... win3.wav
#define NumLoseFiles     6      //lose1.wav ... lose6.wav
#define NumStartFiles    3      //start1.wav ... start3.wav
//Other wave files:
//  chanmode.wav   ... change modes  (this audio file seems to be broken)
//  normmode.wav   ... normal mode
//  statmode.wav   ... stationary mode
//  fastmode.wav   ... fast mode
//  vsmode.wav     ... versus mode
//  reset.wav      ... game reset
//  error.wav      ... unknown error
//  For vs. mode:
//    connect.wav  ... connection made
//    badconn.wav  ... bad connection
//    leftwin.wav  ... Left Side won
//    leftlose.wav ... Left Side lost
//    rightwin.wav ... Right Side won
//    righlose.wav ... Right Side lost

//Sound player
TMRpcm tmrpcm;   // create an object for playing sound
bool useSounds = true;

//Initialize the Servo object, an Array
Servo myServos[TOTAL_SERVOS];

//Create an array that will store the digital pins used by the servos
byte servoPinArray[TOTAL_SERVOS] = {2, 3, 4, 5, 6};

//Create an array that will store the analog pins used by the piezo sensors
byte piezoPinArray[TOTAL_SERVOS] = {A0, A1, A2, A3, A6};

int ServoMovementsClockwise[TOTAL_SERVOS];         //This sets the allowed clockwise movement of each servo
int ServoMovementsCounterClockwise[TOTAL_SERVOS];  //This sets the allowed counterclockwise movement of each servo

//Create an array that will store our randomly-generated list.  It will be zero based.  
//This array will hold a list of the servo numbers (0 - 4) to be activated in a random order.
byte ServoOrderArray[TOTAL_SERVOS];

//Create an array to store the targets that have been hit
//A False means not hit, and a True means hit
bool HitTargets[TOTAL_SERVOS];

void setup () {
  //Set up the random number generator seed by reading a random floating voltage from an unused pin
  randomSeed(analogRead(RND_SEED_PIN));
  
  //Customize each servo's absolute movements.  This allows us to fine tune each servo to work better on the physical board.
  ServoMovementsClockwise[0] = 1000;
  ServoMovementsClockwise[1] = 1000;
  ServoMovementsClockwise[2] = 1000;
  ServoMovementsClockwise[3] = 1000;
  ServoMovementsClockwise[4] = 1000;
  ServoMovementsCounterClockwise[0] = 2000;
  ServoMovementsCounterClockwise[1] = 2000;
  ServoMovementsCounterClockwise[2] = 2000;
  ServoMovementsCounterClockwise[3] = 2000;
  ServoMovementsCounterClockwise[4] = 2000;
  
  if (DEBUGGING) {
    Serial.begin(9600);
    Serial.println(F(" "));
    Serial.println(F("Sketch Beginning"));
  }
  
  // see if the SD card is present and can be initialized:
  if (!SD.begin(SD_CS_PIN)) {  
    if (DEBUGGING) {
       Serial.println(F("SD card fail.  Sounds not available."));  
    }
    useSounds = false;
  } else {
    if (DEBUGGING) {
       Serial.println(F("SD card ok.  Ready!"));  
    }
  }
  
  //Set the Start/reset Button as an input
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  //Set the Mode Button as an input
  pinMode(MODE_PIN, INPUT_PULLUP);
  //Set the LED light as an output
  pinMode(LIGHT_PIN, OUTPUT);
  //Turn the light off
  digitalWrite(LIGHT_PIN, LOW);
  
  //Attach each Servo and initialize the Piezo pins
  if (DEBUGGING) {
    Serial.println(F("Setting up servos and piezos..."));
  }
  for (byte ServoNum = 0; ServoNum < TOTAL_SERVOS; ServoNum++) {
    //Attach each servo.  Automatically calls "pinMode" function
    myServos[ServoNum].attach(servoPinArray[ServoNum]);   
    //Set piezo pins as inputs
    pinMode(piezoPinArray[ServoNum], INPUT_PULLUP);     //I don't know if this should be INPUT?  I think since the piezo's 2nd wire goes to ground, it should be PullUp
  }
  
  //Align all servos to the Left.  This is the "hidden" position for our targets.
  MoveServosLeft();

  //Rotate each servo in order so we can identify them
  for (byte ServoNum = 0; ServoNum < TOTAL_SERVOS; ServoNum++) {
    myServos[ServoNum].writeMicroseconds(ServoMovementsClockwise[ServoNum]);
    delay(1000);
    myServos[ServoNum].writeMicroseconds(ServoMovementsCounterClockwise[ServoNum]);
    delay(1000);
  }
  
  //Join the I2C bus so we can communicate with the other Nano
  if (NANO_IS_MASTER) {
    //Sets the I2C address of this Nano
    Wire.begin(I2C_ADDR); 
    //Enabled in Master version of code
    Wire.onReceive(receiveEventsOnMaster); // register event to listen to the other Nano's messages
  } else {
    //Sets the I2C address of this Nano
    Wire.begin(I2C_SLAVE_ADDR);
    //Enabled in Slave version of code
    Wire.onReceive(receiveEventsOnSlave); // register event to listen to the other Nano's messages
  }
  
  if (useSounds) {
    tmrpcm.speakerPin = SPEAKER_PIN;    //This is where the speaker connects.  The other end of the speaker goes to 5v
    tmrpcm.setVolume(SOUND_VOLUME);
    //Play one of the start wav files
    playRandomSound("start", NumStartFiles);
  }
  
  if (DEBUGGING) {
    Serial.println(F("Setup is completed"));
    Serial.println(F("Waiting for start button to be pressed..."));
  }
}

void loop() {
  //Wait for the operator to press the game mode button
  if (digitalRead(MODE_PIN) == LOW) {
    SwitchGameMode();
  }
  
  //Wait for the operator to press the start button
  if (digitalRead(BUTTON_PIN) == LOW) {
    PlayGame();
  }

  //Don't do ANYTHING else in the LOOP, other than checking for the start button or mode button to be pressed
}
