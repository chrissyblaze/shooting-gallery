
/*
Comm Protocol:
Master:
Codes 1-100
#1:  Are you there?
#2:  Ready to start game?
#3:  About to send target order as 5 Servo numbers
#4-#8:  The Servo order, 5 separate Bytes
#9:  Begin game
#20:  Master has hit all 5 targets.  Game over.  Master wins.
#21:  Received code #171, slave won.  Game over.  ACK

*/

// function that executes whenever data is received from the other Nano
// this function is registered as an event, see setup()
void receiveEventsOnMaster(int howMany) {
  if (DEBUGGING) {
    Serial.print(F("Receiving data from Slave: "));
  }
  while (1 < Wire.available()) { // loop through all but the last
    char c = Wire.read();    // receive byte as a character
    if (DEBUGGING) {
      Serial.print(c);         // print the character
    }
  }
  I2C_recv_buf = Wire.read();       // receive byte as an integer
  if (DEBUGGING) {
    Serial.println(I2C_recv_buf);         // print the integer
  }
}

void MasterVsGame() {
  GetRandomServoOrder();

  I2C_send_buf = 0;
  while (1 == 1) {
    if (digitalRead(BUTTON_PIN) == LOW) {
      if (DEBUGGING) {
        Serial.println(F("Reset button pressed!"));
      }
      //Plays the reset sound
      playSound("reset.wav");
      break;
    }

    Wire.beginTransmission(I2C_SLAVE_ADDR); // transmit to Slave Nano
    Wire.write("x is ");        // sends five bytes
    Wire.write(I2C_send_buf);              // sends one byte
    Wire.endTransmission();    // stop transmitting

    I2C_send_buf++;
    delay(500);
  }

}
