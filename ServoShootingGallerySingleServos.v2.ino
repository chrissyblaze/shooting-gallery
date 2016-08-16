//Shooting Gallery Controller
//Written by Christian Donica
//Being used by Cub Scouts Pack 3001, Medford, OR
//July 31, 2016

/*
Made for the Arduino Nano
Pinout:
Analog:
  A0 - A4 = Piezo sensors 1 through 5 to detect hits on targets.  The other wires on the Piezos goes to GND (because pins A0 - A4 are high)
  A5 = Mode change button.  The other wire on the button goes to GND (because pin A5 is high)  //NOT IMPLEMENTED YET
  A6 = Start/reset Button pin.  The other wire on the Button goes to GND (because pin 8 is high)
  A7 = Do not use!  Left unconnected for Random Seed function

Digital:
  D2 - D6 = Servo control signals for Servos 1 through 5
  D7 =  Light(s) pin.  The other wire on the Lights goes to GND (because pin 7 is high)
  D8 =  Speaker pin - the other speaker wire goes to Vcc
  D9 =  SPI connection to other Arduino (SS) for vs. mode
  D10 = SD Card adapter Chip Select (CS) pin (SS)
  D11 = SD Card adapter MOSI pin (and also to other Arduino for vs. mode)
  D12 = SD Card adapter MISO pin (and also to other Arduino for vs. mode)
  D13 = SD Card adapter SCK pin (and also to other Arduino for vs. mode)
Note: The SD Card adapter also needs to be connected to the Nano's GND and Vcc (5v) pins
*/

#include <Servo.h>
#include <SPI.h>
#include <SD.h>
#include <TMRpcm.h>   //Sound Library

//Define some constants
#define MODE_PIN         A5         //The Mode Change button pin.  The other wire of the button is connected to GND
#define BUTTON_PIN       A6         //The start/reset button pin.  The other wire of the button is connected to GND.
#define RND_SEED_PIN     A7         //Left unconnected for the Random Seed Function
#define LIGHT_PIN        7          //The LED light is connected to this pin.  The other wire of the light goes to GND
#define SPEAKER_PIN      8          //The speaker is connected to this pin (the other end of the speaker goes to Vcc 5v)
#define OTHER_NANO_PIN   9          //SS to connect to second Arduino via SPI
#define SD_ChipSelectPin 10         //For SD Adapter card CS (SS) pin
#define TOTAL_SERVOS     5          //Change this to match your set up
#define CLOCKWISE        1000       //The Pulsewidth to move a servo all the way Clockwise (right) (out of 180 degrees).  Modify this up or down to adjust range of movement.  Default is 1000, but 1100 works better on my Parallax servo.
#define CENTER           1500       //The Pulsewidth to move a servo to the center (out of 180 degrees).  Default is 1500. (This is not really used in this program, but here for future needs.)
#define COUNTERCLOCKWISE 2000       //The Pulsewidth to move a servo all the way Counterclockwise (out of 180 degrees).  Default is 2000.
#define GAME_CYCLES      4          //This is the number of times the game will cycle through the targets until it shuts down and waits for the start button to be pressed again
#define PIEZOTHRESHOLD   130        // analog threshold for piezo sensing
#define SOUND_VOLUME     3          //Sets the sound output (1-7) 7 is pretty distorted
#define FAST_LIGHT       500        //1/2 second delay for a fast light blink
#define SLOW_LIGHT       1000       //1 second delay for a slow light blink

//Set up same variables
byte TARGET_DELAY =      2;         //Delay, in seconds, between targets moving, showing, and hiding
bool DEBUGGING =         true;      //True if you want messages sent to the Serial Monitor
byte gameMode =          1;         //Tracks the game mode:  1 = Normal (target_delay = 2), 2 = Fast (target_delay = 1), 3 = Stationary, 4 = versus

// The number of Sound Files available (change if you add more)
#define NumRicoFiles     23     //rico1.wav ... rico23.wav
#define NumWinFiles      3      //win1.wav ... win3.wav
#define NumLoseFiles     6      //lose1.wav ... lose6.wav
#define NumStartFiles    3      //start1.wav ... start3.wav
//Other wave files:
//  chanmode.wav   ... change modes
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

//Macro to add 2 to Pins to match our code.  Servo Pins actually go from 2 to 6, but our code goes from 0 to 4
#define SERVO_TO_PIN(x) (x+2)

//Initialize the Servo object, an Array
Servo myServos[TOTAL_SERVOS];

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
  
  //Customize each servo's absolute movments
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
  
  if (!SD.begin(SD_ChipSelectPin)) {  // see if the card is present and can be initialized:
    if (DEBUGGING) {
       Serial.println(F("SD card fail.  Sounds not available."));  
    }
    useSounds = false;
  } else {
    if (DEBUGGING) {
       Serial.println(F("SD card ok.  Ready!"));  
    }
  }
  
  if (useSounds) {
    tmrpcm.speakerPin = SPEAKER_PIN;    //This is where the speaker connects.  The other end of the speaker goes to 5v
    tmrpcm.setVolume(SOUND_VOLUME);
    //Play one of the start wav files
    playRandomSound("start", NumStartFiles);
  }
  
  //Set the Start/reset Button as an input
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  //Set the Mode Button as an input
  pinMode(MODE_PIN, INPUT_PULLUP);
  //Set the LED light as an output
  pinMode(LIGHT_PIN, OUTPUT);
  //Turn the light off
  digitalWrite(LIGHT_PIN, LOW);
    
  //Attach each Servo
  if (DEBUGGING) {
    Serial.println(F("Setting up servos..."));
  }
  for (byte ServoNum = 0; ServoNum < TOTAL_SERVOS; ServoNum++) {
    myServos[ServoNum].attach(SERVO_TO_PIN(ServoNum));
  }
  
  //Align all servos to the Left.  This is the "hidden" position for our targets.
  MoveServosLeft();

  //Rotate each servo in order so we can identify them
  for (byte x = 0; x < TOTAL_SERVOS; x++) {
    myServos[x].writeMicroseconds(CLOCKWISE);
    delay(1000);
    myServos[x].writeMicroseconds(COUNTERCLOCKWISE);
    delay(1000);
  }
  
  //Reset the HitTarget array
  for (byte ServoNum = 0; ServoNum < TOTAL_SERVOS; ServoNum++) {
    HitTargets[ServoNum] = false;
  }
  
  if (DEBUGGING) {
    Serial.println(F("Setup is completed"));
    Serial.println(F("Waiting for start button to be pressed..."));
  }
}

void loop() {
   byte TempRandomNum = 0;      //Variable used to store temporary random numbers
   int PiezoSensor;             //The value returned by the Piezo sensor
   bool gameWasReset = false;   //Only used when the user presses the Reset button mid-game

  //Wait for the operator to press the game mode button
  if (digitalRead(MODE_PIN) == LOW) {
    if (DEBUGGING) {
      Serial.println(F("Changing game mode."));
      Serial.println(F("Playing file chanmode.wav."));
    }
    tmrpcm.play("chanmode.wav");
    //Increase game mode by one, cycle back to 1 if you get to 4 (we're not using 4 right now, which is vs. mode)
    gameMode++;
    if (gameMode == 4) {
      gameMode = 1;
    }
    switch (gameMode) {
      case 1:    //Normal mode
        if (DEBUGGING) {
          Serial.println(F("Changing to NORMAL game mode (TARGET_DELAY = 2)."));
          Serial.println(F("Playing file normmode.wav."));
        }
        tmrpcm.play("normmode.wav");
        TARGET_DELAY = 2;
        break;
      case 2:    //Fast mode
        if (DEBUGGING) {
          Serial.println(F("Changing to FAST game mode (TARGET_DELAY = 1)."));
          Serial.println(F("Playing file fastmode.wav."));
        }
        tmrpcm.play("fastmode.wav");
        TARGET_DELAY = 1;
        break;
      case 3:   //Stationary mode
        if (DEBUGGING) {
          Serial.println(F("Changing to STATIONARY game mode."));
          Serial.println(F("Playing file statmode.wav."));
        }
        tmrpcm.play("statmode.wav");
        break;
      case 4:   //Vs. mode, which is also an error for now
        if (DEBUGGING) {
          Serial.println(F("Changing to VERSUS game mode."));
          Serial.println(F("Playing file vsmode.wav."));
          Serial.println(F("Playing file error.wav."));
        }
        tmrpcm.play("vsmode.wav");
        tmrpcm.play("error.wav");
        break;
    }
    delay(2000);    //wait for button debounce
  }
  
  //Wait for the operator to press the start button
  if (digitalRead(BUTTON_PIN) == LOW) {
    if (gameMode == 1 || gameMode == 2) {
      if (DEBUGGING) {
        Serial.println(F("Starting shooting gallery game in Normal or Fast mode!"));
      }
    
      //Hide the targets
      MoveServosLeft();
      delay(2000);
    
      //Control the servos for the shooting gallery game
      
      //Repeat the game Game_Cycles times.  It will go through all TOTAL_SERVOS number of moves before starting over with a new
      //random order.  So, basically GameCycles x Total_Servos = the total number of things that will move until the game stops.
      for (byte i = 0; i < GAME_CYCLES; i++) {
        if (DEBUGGING) {
          Serial.print(F("Beginning game cycle: "));
          Serial.print(i + 1);
          Serial.print(F(" of "));
          Serial.println(GAME_CYCLES);
        }
        
        //Determine random order for the servos to move so it isn't the same pattern each cycle
  
        //Set the array to defaults of -1
        for (byte j = 0; j < TOTAL_SERVOS; j++) {
           ServoOrderArray[j] = -1;
        }
  
        //Now, cycle through the array, adding a random number each time, but not allowing duplicates
        for (byte k = 0; k < TOTAL_SERVOS; k++) {
           //initalize our flag variable
           bool flag = false;
  
           //Repeat selecting random numbers until a number is found that hasn't already been saved in the array
           while (flag == false) {
              //Get a random number
              //Will return a number between 0 and TOTAL-SERVOS - 1, which matches the indexes on our myServos array, which is zero based
              TempRandomNum = random(0, TOTAL_SERVOS);    
  
              //Check to see if the random number is already in our Servo Order array.  If not, save it and exit this While
              flag = true;    //Goes false if we've already used that servo number
              for (byte m = 0; m < TOTAL_SERVOS; m++) {
                 if (ServoOrderArray[m] == TempRandomNum) {
                     //If the number storad at array element m is the same as our random number temp, then skip out and pick a different random number
                     flag = false;
                     break;                   
                 }
              }
           }
  
           //We've left the random number generator loop with a servo number that hasn't been used yet.  Save this number now!  
           ServoOrderArray[k] = TempRandomNum;
        }
        
        //Move servos in the order list
        for (byte ServoNum = 0; ServoNum < TOTAL_SERVOS; ServoNum++) {
          if (HitTargets[ServoOrderArray[ServoNum]] == false) {
            if (DEBUGGING) {
              Serial.print(F("Moving servo: "));
              Serial.println(ServoOrderArray[ServoNum]);
              Serial.print(F("This servo is connected to Pin: "));
              Serial.println(SERVO_TO_PIN(ServoOrderArray[ServoNum]));
            }
              
            //Move the servo clockwise (SHOW)
            myServos[ServoOrderArray[ServoNum]].writeMicroseconds(CLOCKWISE);
  
            //Wait for the servo to move before looking at the sensor
            delay(1000);
            //Look for a "hit" on the target
            //This loop makes the Arduino read the pin over and over during the wait delay time
            for (int z = 0; z < TARGET_DELAY * 1000; z++) {
               delay(1);
               PiezoSensor = analogRead(ServoOrderArray[ServoNum]);
               if (PiezoSensor >= PIEZOTHRESHOLD) {
                  if (DEBUGGING) {
                    Serial.print(F("Hit above threshold!  Sensor reading: "));
                    Serial.println(PiezoSensor);
                  }
                  HitTargets[ServoOrderArray[ServoNum]] = true;
                  //Play random ricochet sound
                  playRandomSound("rico", NumRicoFiles);
                  //Flash the hit light once for each target already hit
                  for (byte TargetNum = 0; TargetNum < TOTAL_SERVOS; TargetNum++) {
                    if (HitTargets[TargetNum] == true) {
                      flashLight(SLOW_LIGHT, 1);
                    }
                  }
                  break;
               }
            }   
                
            //Only move the target back to hidden if the target wasn't hit
            if (HitTargets[ServoOrderArray[ServoNum]] == false) {
              //Move the servo counterclockwise (HIDE)
              myServos[ServoOrderArray[ServoNum]].writeMicroseconds(COUNTERCLOCKWISE);
            }
  
            //Check to see if the player hit ALL targets.  If not, delay till the next round
            bool hitAllTargets = true;
            for (byte TargetNum = 0; TargetNum < TOTAL_SERVOS; TargetNum++) {
              if (HitTargets[TargetNum] == false) {
                hitAllTargets = false;
              }
            }
  
            if (!hitAllTargets) {
              //Wait    (MIGHT NEED MORE TIME FOR RELOADING GUNS)
              delay(TARGET_DELAY * 1000);
            }
          }
          if (digitalRead(BUTTON_PIN) == LOW) {
            if (DEBUGGING) {
              Serial.println(F("Reset button pressed!"));
            }
            i = GAME_CYCLES;  //to tell the loop that all game cycles have been played
            gameWasReset = true;
            break;
          }
        }
      }
      
      //The Game Cycle Loop is over.  Now see if the player won or lost, and play sounds and flash lights
  
      //Check to see if the player hit ALL targets.  If so, display WIN light (or play a win sound if enabled)
      bool hitAllTargets = true;
      for (byte TargetNum = 0; TargetNum < TOTAL_SERVOS; TargetNum++) {
        if (HitTargets[TargetNum] == false) {
          hitAllTargets = false;
        }
      }
  
      if (gameWasReset) {
        if (useSounds) {
          //Plays the reset sound
          if (DEBUGGING) {
            Serial.println(F("Playing Reset.wav sound."));
          }
          tmrpcm.play("reset.wav");
          delay(3000);  //important delay, to avoid button bounce when holding down the reset button
        }
      } else {
        if (hitAllTargets) {
          //Flash the light three times
          if (DEBUGGING) {
            Serial.println(F("Player won! He hit all 5 targets!"));
            Serial.println(F("Flashing Win lights"));
          }
          if (useSounds) {
            //Plays a random winning sound
            playRandomSound("win", NumWinFiles);
          }
          //Flash the hit light 
          flashLight(FAST_LIGHT, 5);
        } else {
          if (DEBUGGING) {
            Serial.println(F("Player lost because he did NOT hit all 5 targets."));
            Serial.println(F("Flashing Lose lights"));
          }
          if (useSounds) {
            //Plays a random losing sound
            playRandomSound("lose", NumLoseFiles);
          }
          //Flash the light 
          flashLight(FAST_LIGHT, 1);
          //Flash the light 
          flashLight(100, 2);
        }
      }
  
      //Reset the HitTarget array
      for (byte ServoNum = 0; ServoNum < TOTAL_SERVOS; ServoNum++) {
        HitTargets[ServoNum] = false;
      }
      
      if (DEBUGGING) {
        Serial.println(F("Game over!"));
        Serial.println(F("Waiting for start button to be pressed..."));
      }
    } else if (gameMode == 3) {
      if (DEBUGGING) {
        Serial.println(F("Starting shooting gallery game in Stationary mode!"));
      }
      delay(2000);    ///button debounce
      //We're in stationary mode, so un-hide the targets
      MoveServosRight();  
      
      int PiezoSensor1;
      int PiezoSensor2;
      int PiezoSensor3;
      int PiezoSensor4;
      int PiezoSensor5;
      
      //Now, we just wait for any hits, and when all five targets have been hit, declare a winner
      //Also, we look for a reset command, if pushed
      while (HitTargets[0] == false || HitTargets[1] == false || HitTargets[2] == false || HitTargets[3] == false || HitTargets[4] == false) {
        if (digitalRead(BUTTON_PIN) == LOW) {
          if (DEBUGGING) {
            Serial.println(F("Reset button pressed!"));
          }
          break;
        }
        PiezoSensor1 = analogRead(A0);
        PiezoSensor2 = analogRead(A1);
        PiezoSensor3 = analogRead(A2);
        PiezoSensor4 = analogRead(A3);
        PiezoSensor5 = analogRead(A4);
       
        if (PiezoSensor1 >= PIEZOTHRESHOLD) {
          if (DEBUGGING) {
            Serial.print(F("Target 1 hit above threshold!  Sensor reading: "));
            Serial.println(PiezoSensor1);
          }
          HitTargets[0] = true;
          //Play random ricochet sound
          playRandomSound("rico", NumRicoFiles);
          //Flash the hit light 
          flashLight(FAST_LIGHT, 1);
        } else if (PiezoSensor2 >= PIEZOTHRESHOLD) {
          if (DEBUGGING) {
            Serial.print(F("Target 2 hit above threshold!  Sensor reading: "));
            Serial.println(PiezoSensor2);
          }
          HitTargets[1] = true;
          //Play random ricochet sound
          playRandomSound("rico", NumRicoFiles);
          //Flash the hit light 
          flashLight(FAST_LIGHT, 2);
        } else if (PiezoSensor3 >= PIEZOTHRESHOLD) {
          if (DEBUGGING) {
            Serial.print(F("Target 3 hit above threshold!  Sensor reading: "));
            Serial.println(PiezoSensor3);
          }
          HitTargets[2] = true;
          //Play random ricochet sound
          playRandomSound("rico", NumRicoFiles);
          //Flash the hit light 
          flashLight(FAST_LIGHT, 3);
        } else if (PiezoSensor4 >= PIEZOTHRESHOLD) {
          if (DEBUGGING) {
            Serial.print(F("Target 4 hit above threshold!  Sensor reading: "));
            Serial.println(PiezoSensor4);
          }
          HitTargets[3] = true;
          //Play random ricochet sound
          playRandomSound("rico", NumRicoFiles);
          //Flash the hit light 
          flashLight(FAST_LIGHT, 4);
        } else if (PiezoSensor5 >= PIEZOTHRESHOLD) {
          if (DEBUGGING) {
            Serial.print(F("Target 5 hit above threshold!  Sensor reading: "));
            Serial.println(PiezoSensor5);
          }
          HitTargets[4] = true;
          //Play random ricochet sound
          playRandomSound("rico", NumRicoFiles);
          //Flash the hit light 
          flashLight(FAST_LIGHT, 5);
        }
      }
      
      //Reset the HitTarget array
      for (byte ServoNum = 0; ServoNum < TOTAL_SERVOS; ServoNum++) {
        HitTargets[ServoNum] = false;
      }
      
      delay(2000);    ///button debounce
      
      if (DEBUGGING) {
        Serial.println(F("Game over!"));
        Serial.println(F("Waiting for start button to be pressed..."));
      }
    } else if (gameMode == 4) {
      if (DEBUGGING) {
        Serial.println(F("Should not be in this mode, it is not available yet."));
        Serial.println(F("Playing file error.wav."));
      }
      tmrpcm.play("error.wav");
      if (DEBUGGING) {
        Serial.println(F("Game over!"));
        Serial.println(F("Waiting for start button to be pressed, although you should change to a working mode first."));
      }
    } else {
      if (DEBUGGING) {
        Serial.println(F("Undefined game mode!"));
        Serial.println(F("Playing file error.wav."));
      }
      tmrpcm.play("error.wav");
      if (DEBUGGING) {
        Serial.println(F("Waiting for start button to be pressed, although you should change modes first!"));
      }
    }
  }

  //Don't do ANYTHING else in the LOOP, other than checking for the start button or mode button to be pressed
}

void flashLight(int delayer, int numFlashes) {
  for (byte num = 0; num < numFlashes; num++) {
    digitalWrite(LIGHT_PIN, HIGH);
    delay(delayer);
    digitalWrite(LIGHT_PIN, LOW); 
  }
}

void playRandomSound(String name, int numFiles) {
   if (useSounds) { 
     String soundFileName = name + String(random(1, numFiles + 1)) + ".wav";
     char soundFileNameBuffer[soundFileName.length() + 1];
     soundFileName.toCharArray(soundFileNameBuffer, soundFileName.length() + 1);
     Serial.println("Playing random file: " + soundFileName);
     tmrpcm.play(soundFileNameBuffer);
   }
}

void MoveServosRight() {
  if (DEBUGGING) {
    Serial.println(F("Moving all servos clockwise..."));
  }
  
  for (byte ServoNum = 0; ServoNum < TOTAL_SERVOS; ServoNum++) {
    myServos[ServoNum].writeMicroseconds(CLOCKWISE);
  } 

  if (DEBUGGING) {
    Serial.println(F("Moved all servos clockwise."));
  }
}

void MoveServosLeft() {
  if (DEBUGGING) {
    Serial.println(F("Moving all servos counterclockwise..."));
  }
  
  for (byte ServoNum = 0; ServoNum < TOTAL_SERVOS; ServoNum++) {
    myServos[ServoNum].writeMicroseconds(COUNTERCLOCKWISE);
  } 
  
  if (DEBUGGING) {
    Serial.println(F("Moved all servos counterclockwise."));
  }
}
