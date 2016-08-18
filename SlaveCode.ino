/*
Comm protocol

Slave:
Codes 150-250
#150:  Received your code #1, and yes I am here.  ACK
#151:  Received your code #2, and ready to start game.  ACK
#152:  Received the 5 Servo numbers.  ACK
#153:  Error:  Did not receive codes #4-8:  5 good unique servo
numbers  (master cycle back to #3)
#154:  Error:  Did not receive codes #4-8 after 5 tries.  Cancelling game.
#155:  Beginning game.  ACK
#170:  Received code #20, master won.  Game over.  ACK
#171:  Slave has hit all 5 targets.  Game over.  Slave wins.

*/
// function that executes whenever data is received from the other Nano
// this function is registered as an event, see setup()

void receiveEventsOnSlave(int howMany) {
  if (DEBUGGING) {
    Serial.print(F("Receiving data from Master!: "));
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

void SlaveVsGame() {
  while (1 == 1) {
    if (digitalRead(BUTTON_PIN) == LOW) {
      if (DEBUGGING) {
        Serial.println(F("Reset button pressed!"));
      }
      //Plays the reset sound
      playSound("reset.wav");
      break;
    }
    Wire.beginTransmission(I2C_ADDR); // transmit to Master Nano
  
    Wire.write("x is ");        // sends five bytes
    Wire.write(I2C_send_buf);              // sends one byte
    Wire.endTransmission();    // stop transmitting
  
    I2C_send_buf++;
    delay(500);
  }
}
