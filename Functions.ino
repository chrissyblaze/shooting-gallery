void SwitchGameMode() {
  if (DEBUGGING) {
    Serial.println(F("Changing game mode."));
  }
  
  //Increase game mode by one, cycle back to 1 if you get past 4
  gameMode++;
  if (gameMode > 4) {
    gameMode = 1;
  }
  switch (gameMode) {
    case 1:    //Normal mode
      if (DEBUGGING) {
        Serial.println(F("Changing to NORMAL game mode (TARGET_DELAY = 2)."));
      }
      playSound("normmode.wav");
      TARGET_DELAY = 2;
      break;
    case 2:    //Fast mode
      if (DEBUGGING) {
        Serial.println(F("Changing to FAST game mode (TARGET_DELAY = 1)."));
      }
      playSound("fastmode.wav");
      TARGET_DELAY = 1;
      break;
    case 3:   //Stationary mode
      if (DEBUGGING) {
        Serial.println(F("Changing to STATIONARY game mode."));
      }
      playSound("statmode.wav");
      break;
    case 4:   //Vs. mode
      if (DEBUGGING) {
        Serial.println(F("Changing to VERSUS game mode."));
      }
      playSound("vsmode.wav");
      break;
  }
  if (DEBUGGING) {
    Serial.println(F("Mode change completed.  Waiting for start button to be pressed..."));
  }
  delay(2000);    //wait for button debounce
}



void PlayGame() {
  delay(2000);   //delay to protect from start/reset button being held down too long
  
 //Reset the HitTarget array
  for (byte ServoNum = 0; ServoNum < TOTAL_SERVOS; ServoNum++) {
    HitTargets[ServoNum] = false;
  }
  
  //Choose the correct game type
  if (gameMode == 1 || gameMode == 2) {
    PlayRegularGame();
  } else if (gameMode == 3) {
    PlayStationaryGame();
  } else if (gameMode == 4) {
    PlayVsGame();
  } else {
    if (DEBUGGING) {
      Serial.println(F("Undefined game mode!"));
    }
    playSound("error.wav");
    if (DEBUGGING) {
      Serial.println(F("Waiting for start button to be pressed, although you should change modes first!"));
    }
  } 
  delay(2000);   //important delay, to avoid button bounce or continuous reading when holding down the reset/start button
}


void PlayRegularGame() {
  bool gameWasReset = false;   //Only used when the user presses the Reset button mid-game
  
  if (DEBUGGING) {
    if (gameMode == 1) {
      Serial.println(F("Starting shooting gallery game in Normal mode!"));
    } else {
      Serial.println(F("Starting shooting gallery game in Fast mode!"));
    }
  }

  //Hide the targets
  MoveServosLeft();
  //Pause before targets start moving
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
    GetRandomServoOrder();
      
    //Move servos in the order list
    for (byte ServoNum = 0; ServoNum < TOTAL_SERVOS; ServoNum++) {
      if (HitTargets[ServoOrderArray[ServoNum]] == false) {
        if (DEBUGGING) {
          Serial.print(F("Moving servo: "));
          Serial.println(ServoOrderArray[ServoNum]);
          Serial.print(F("This servo is connected to Pin: "));
          Serial.println(servoPinArray[ServoOrderArray[ServoNum]]);
        }
          
        //Move the servo clockwise (SHOW)
        myServos[ServoOrderArray[ServoNum]].writeMicroseconds(ServoMovementsClockwise[ServoNum]);

        //Wait for the servo to physically move before looking at the sensor
        delay(1000);
        //Look for a "hit" on the target
        //This loop makes the Arduino read the pin over and over during the wait delay time
        for (int z = 0; z < TARGET_DELAY * 1000; z++) {
           delay(1);
           int PiezoSensor = analogRead(piezoPinArray[ServoOrderArray[ServoNum]]);
           if (PiezoSensor >= PIEZOTHRESHOLD) {
              if (DEBUGGING) {
                Serial.print(F("Hit above threshold!  Piezo pin: A"));
                Serial.print(piezoPinArray[ServoOrderArray[ServoNum]] - 14);
                Serial.print(" - Sensor reading: ");
                Serial.println(PiezoSensor);
              }
              HitTargets[ServoOrderArray[ServoNum]] = true;
              //Play random ricochet sound
              playRandomSound("rico", NumRicoFiles);
              //Flash the hit light once for each target already hit, so this becomes more each time a new target is hit
              flashLight(FAST_LIGHT, CountHits());
              //if (DEBUGGING) {
                //Serial.print(F("Hits: "));
                //Serial.println(CountHits());
              //}
              break;
           }
        }   
            
        //Only move the target back to hidden if the target wasn't hit
        if (HitTargets[ServoOrderArray[ServoNum]] == false) {
          //Move the servo counterclockwise (HIDE)
          myServos[ServoOrderArray[ServoNum]].writeMicroseconds(ServoMovementsCounterClockwise[ServoNum]);
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
    //Plays the reset sound
    playSound("reset.wav");
  } else {
    if (hitAllTargets) {
      //Flash the light five times
      if (DEBUGGING) {
        Serial.println(F("Player won! He hit all 5 targets!"));
        Serial.println(F("Flashing Win lights"));
      }
      //Plays a random winning sound
      playRandomSound("win", NumWinFiles);
      //Flash the hit light 
      flashLight(FAST_LIGHT, 5);
    } else {
      if (DEBUGGING) {
        Serial.println(F("Player lost because he did NOT hit all 5 targets."));
        Serial.println(F("Flashing Lose lights"));
      }
      //Plays a random losing sound
      playRandomSound("lose", NumLoseFiles);
      //Flash the light 
      flashLight(FAST_LIGHT, 1);
      //Flash the light 
      flashLight(100, 2);
    }
  }
  
  if (DEBUGGING) {
    Serial.println(F("Game over!"));
    Serial.println(F("Waiting for start button to be pressed..."));
  }
}


void PlayStationaryGame() {
  if (DEBUGGING) {
    Serial.println(F("Starting shooting gallery game in Stationary mode!"));
  }
  
  //We're in stationary mode, so un-hide the targets
  MoveServosRight();  
  
  //Now, we just wait for any hits, and when all five targets have been hit, declare a winner
  //Also, we look for a reset command, if pushed
  while (HitTargets[0] == false || HitTargets[1] == false || HitTargets[2] == false || HitTargets[3] == false || HitTargets[4] == false) {
    if (digitalRead(BUTTON_PIN) == LOW) {
      if (DEBUGGING) {
        Serial.println(F("Reset button pressed!"));
      }
      //Plays the reset sound
      playSound("reset.wav");
      break;
    }
    
    //Read all piezo sensors
    for (byte PiezoNum = 0; PiezoNum < TOTAL_SERVOS; PiezoNum++) {
      int PiezoSensor = analogRead(piezoPinArray[PiezoNum]);
      
      if (PiezoSensor >= PIEZOTHRESHOLD) {
        if (DEBUGGING) {
          Serial.print(F("Target "));
          Serial.print(PiezoNum + 1);
          Serial.print(F(" on Pin A"));
          Serial.print(piezoPinArray[PiezoNum] - 14);
          Serial.print(F(" hit above threshold!  Sensor reading: "));
          Serial.println(PiezoSensor);
        }
        //Since the target was hit, hide the target so the player only sees the remaining unhit targets
        myServos[PiezoNum].writeMicroseconds(ServoMovementsCounterClockwise[PiezoNum]);
        //Record the hit
        HitTargets[PiezoNum] = true;
        //Play random ricochet sound
        playRandomSound("rico", NumRicoFiles);
        //Flash the hit light 
        flashLight(FAST_LIGHT, CountHits());
      }
    }
  }
  
  //All done with game
  
  //Flash the light three times
  if (DEBUGGING) {
    Serial.println(F("Player won! He hit all 5 targets!"));
    Serial.println(F("Flashing Win lights"));
  }
  //Plays a random winning sound
  playRandomSound("win", NumWinFiles);
  //Flash the hit light 
  flashLight(FAST_LIGHT, 5);
  
  if (DEBUGGING) {
    Serial.println(F("Game over!"));
    Serial.println(F("Waiting for start button to be pressed..."));
  }
}



byte CountHits() {
  //Used to see how many target hits the player currently has
  byte Hits = 0;
  for (byte TargetNum = 0; TargetNum < TOTAL_SERVOS; TargetNum++) {
    if (HitTargets[TargetNum] == true) {
      Hits++;
    }
  }
  return Hits;
}



void PlayVsGame() {
  if (DEBUGGING) {
    Serial.println(F("Should not be in this mode, it is not available yet. Press button/reset to leave this mode."));
  }
  
  //Working on new code here to connect games
  
  if (NANO_IS_MASTER) {
    MasterVsGame();
  } else {
    SlaveVsGame();
  }
  
  if (DEBUGGING) {
    Serial.println(F("Game over!"));
    Serial.println(F("Waiting for start button to be pressed, although this mode is not ready, and you should change modes."));
  } 
}


void GetRandomServoOrder() {
  //Variable used to store temporary random numbers
  byte TempRandomNum = 0;      
  
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
     if (DEBUGGING) {
       Serial.print(F("Playing random file: "));
       Serial.println(soundFileName);
     }
     tmrpcm.play(soundFileNameBuffer);
   }
}



void playSound(String name) {
  if (useSounds) { 
    char soundFileNameBuffer[name.length() + 1];
    name.toCharArray(soundFileNameBuffer, name.length() + 1);
    if (DEBUGGING) {
      Serial.print(F("Playing file: "));
      Serial.println(name);
    }
    tmrpcm.play(soundFileNameBuffer);
  }
}



void MoveServosRight() {
  if (DEBUGGING) {
    Serial.println(F("Moving all servos clockwise..."));
  }
  
  for (byte ServoNum = 0; ServoNum < TOTAL_SERVOS; ServoNum++) {
    myServos[ServoNum].writeMicroseconds(ServoMovementsClockwise[ServoNum]); 
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
    myServos[ServoNum].writeMicroseconds(ServoMovementsCounterClockwise[ServoNum]);
  } 
  
  if (DEBUGGING) {
    Serial.println(F("Moved all servos counterclockwise."));
  }
}
