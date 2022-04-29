//asdadada
#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>

//todo:
//change from using strings to use a new data type for the data

//---------------------------

//can store the data as a string: mem location 0 is id, 1-15 is the description, 16-18 are values, 19-21 is min, 22-24 is max
//scroll up and down the list with the buttons. 
//each of the states simply appends data to the correct location in memory for later access

// can implement a class to store the data, may be better

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

void setup() {
  lcd.begin(16,2);
  lcd.setBacklight(7);
  lcd.clear();
  Serial.begin(9600);
}

enum state_e { SYNCHRONISATION = 3, INITIALISATION, WAITING, NEW_CHANNEL, VALUE, MAX, MIN }; // the main states
enum state_b { WAITING_PRESS = 8, WAITING_RELEASE }; // states for pressing the buttons

static String channelArray[26];//global declaration of the array and the colours for the screen
static int screenRedCount = 0;
static int screenGreenCount = 0;

//handles the placement of v and ^ for scrolling
void atTop(){ 
  lcd.setCursor(0,0);
  lcd.print(' ');
  lcd.setCursor(0,1);
  lcd.print('v');
}
void atBtm(){
  lcd.setCursor(0,0);
  lcd.print('^');
  lcd.setCursor(0,1);
  lcd.print(' ');
}
void inMiddle(){
  lcd.setCursor(0,0);
  lcd.print('^');
  lcd.setCursor(0,1);
  lcd.print('v');
}

//updates the diaply with current values
void updateDisplay(int channelArrayLength,int topDisplay){
  if(channelArrayLength == 1){ // if one value
    lcd.setCursor(1,0);
    /*Serial.println("DEBUG: " + (String) channelArrayLength);
    Serial.println("DEBUG: " + (String) topDisplay);*/
    lcd.print(channelArray[topDisplay][0]);
    lcd.print(channelArray[topDisplay][16]);
    lcd.print(channelArray[topDisplay][17]);
    lcd.print(channelArray[topDisplay][18]);
    /*
    for (int x = 0; x < channelArrayLength; x++){
      Serial.println("DEBUG: " + channelArray[x]);
    }
    */
  }else if(channelArrayLength > 1){ // if two values
    //copy above, but include two lines (topDisplay and topDisplay + 1)
    lcd.setCursor(1,0);
    lcd.print(channelArray[topDisplay][0]);
    lcd.print(channelArray[topDisplay][16]);
    lcd.print(channelArray[topDisplay][17]);
    lcd.print(channelArray[topDisplay][18]);
    lcd.setCursor(1,1);
    lcd.print(channelArray[topDisplay + 1][0]);
    lcd.print(channelArray[topDisplay + 1][16]);
    lcd.print(channelArray[topDisplay + 1][17]);
    lcd.print(channelArray[topDisplay + 1][18]);
  }
  if (channelArrayLength > 2){ // if more than two values (add v and ^ for scrolling)
    if(topDisplay == 0){//if at top of the list
      atTop();
    }else if((topDisplay + 2) == channelArrayLength){//if at bottom of the list
      atBtm();
    }else{
      inMiddle();
    }
  }

  for (int x = 0; x < channelArrayLength; x++){ // creates local channelMin, channelMax and channelVal values for comparison
    String channelMin = "   ";
    for (int y = 0; y < 3; y++){
      channelMin[y] = channelArray[x][y+19];
    }    
    String channelMax = "   ";
    for (int y = 0; y < 3; y++){
      channelMax[y] = channelArray[x][y+22];
    }
    String channelVal = "   ";
    for (int y = 0; y < 3; y++){
      channelVal[y] = channelArray[x][y+16];
    }
    //Serial.println(channelMin); //debugging these values
    //Serial.println(channelMax);
    //Serial.println(channelVal);
    if (channelMax >= channelMin){ // determine whether to change the colour of the display, colour will be changed until 5 new values hae been entered
      if (channelVal != "   "){
        if (channelVal < channelMin){
          screenGreenCount = 5;
        }else if(channelVal > channelMax){
          screenRedCount = 5;
        }
      }
    }
    //Serial.println("DEBUG: " + (String)screenGreenCount); // debugging the system for changing the colour of the display
    //Serial.println("DEBUG: " + (String)screenRedCount);
    if ((screenRedCount > 0) & (screenGreenCount > 0)){
      lcd.setBacklight(3); //yellow
    }
    else if (screenRedCount > 0){
      lcd.setBacklight(1);//red
    }
    else if(screenGreenCount > 0){
      lcd.setBacklight(2);//green
    }else{
      lcd.setBacklight(7);//white
    }
  }
  
}

void selectDisplay(){ // changes the display when "select" button is pressed
  lcd.setBacklight(5);//set it purple
  lcd.setCursor(0,0);
  lcd.print("F128493         ");
  lcd.setCursor(0,1);
  lcd.print("                ");
  }
void wipeDisplay(){ // clears the display upon releasing the select button, sets colour back to white
  lcd.setCursor(0,0);
  lcd.print("                ");
  lcd.setCursor(0,1);
  lcd.print("                ");
  lcd.setBacklight(7);
}

void loop() {
  static int sync_interval;
  static String message;
  static char protocol;
  static state_e state = SYNCHRONISATION; //initialise main states
  static String oldMessage;
  static int channelArrayLength;
  
  static int lastButton = 0;
  static int topDisplay = 0;
  //static bool notPlaced;
  
  switch (state){
    case WAITING: // The state of checking repeatedly for a C,V,X or N message; also handles menu, display and buttons
    // changes state accordingly
      {
        //Serial.println("Waiting message");
        message = "";
        message = Serial.readString(); // get message from serial monitor
        
        if (oldMessage){
          if (message != "-1"){ // if the message is valid
            protocol = message[0];
            if(protocol == 'C'){
              state = NEW_CHANNEL;
            }else if(protocol == 'V'){
              oldMessage = message;
              state = VALUE;
            }else if(protocol == 'X'){
              oldMessage = message;
              state = MAX;
            }else if(protocol == 'N'){
              oldMessage = message;
              state = MIN;
            }else if(isAlpha(protocol)){ // if message doesn't fit protocol report error to user
              oldMessage = message;
              Serial.println("ERROR: " + message);
            }
          }
        }
        static int pressed;
        static int updown;
        
        static state_b buttonState = WAITING_PRESS; //initialise button states

        switch (buttonState){
          case WAITING_PRESS: //while waiting for a button to be pressed or pressing up or down
            {
              updateDisplay(channelArrayLength, topDisplay);
              int button = lcd.readButtons();
              //if pressed now and it wasnt earlier
              pressed = button & ~lastButton;
              lastButton = button;
        
              //if(channelArrayLength > 2){
              if(pressed & (BUTTON_UP | BUTTON_DOWN | BUTTON_SELECT)){ // if a valid button has been pressed
                if(pressed & BUTTON_UP){
                  //if button up scroll up
                  if ((topDisplay != 0) & (channelArrayLength > 2)){
                    topDisplay--;
                    updateDisplay(channelArrayLength, topDisplay);
                  }
              
                }else if (pressed & BUTTON_DOWN){
                  //if button down scroll down
                  if ((channelArrayLength - topDisplay > 2) & (channelArrayLength > 2)){
                    topDisplay++;
                    updateDisplay(channelArrayLength, topDisplay);
                  }
                }else{
                  //if select, change display and state
                  selectDisplay();
                  buttonState = WAITING_RELEASE; //wait for button to be released
                }
                updown = pressed;
                //}
              }           
            }
          case WAITING_RELEASE: // when select is pressed currently
            {
              int button = lcd.readButtons();
              int released = ~button & lastButton; // if the button is not currently being pressedAND it was the last button to be pressed
              lastButton = button;
              if (released & pressed){//when released wipe dislay and restore old values
                wipeDisplay(); 
                updateDisplay(channelArrayLength, topDisplay);
                buttonState = WAITING_PRESS; // back to waiting
                
              }
            }
        }
                
        break;
      }
    case SYNCHRONISATION://This state synchronises the arduino program and the Serial Monitor
      {
        lcd.setBacklight(5);  // set to purple
        if (not sync_interval){
          sync_interval = millis();
        }
        char sync = Serial.read();
        if(sync=='X'){// if X is recieved from serial monitor
          //Serial.println("DEBUG: Synchronisation complete");
          state = INITIALISATION;
        }
        if (millis() - sync_interval >= 1000){ // every second send "Q"again
          sync_interval = millis();
          Serial.print('Q');
        }
        break;
      }
    case INITIALISATION:// This initialises the arduino, providing the backlight
      {
        Serial.println("BASIC");
        lcd.setBacklight(7);
        channelArrayLength = 0; //initialise length of channel array here
        state = WAITING;
        break;
      }
    case NEW_CHANNEL:// When a 'C' is the first character, create a new channel or rename old
      {
        Serial.println("DEBUG: NEW_CHANNEL");
        //Serial.println("-" + message + "-");
        String newChannel = "                     0255";//if not in array use this, if in array overwrite new description
        newChannel[0] = message[1];                     //onto the values stored in the array 
                                                        //-instead of using newChannel, use channelArray[x]
        //Serial.println("-"+newChannel+"-");
        int messageLen = message.length();
        if (messageLen > 15){ // descriptions longer than 15 characters are ignored
          messageLen = 15;  
        }
                
        //Serial.println("-"+newChannel+"-");

        bool inArray = false;
        // search array and if in array, overwrite description
        for (int x = 0; x < 26; x++){
          if (channelArray[x][0] == newChannel[0]){
            newChannel = channelArray[x]; //copy data from old channel            
            inArray = true;
            for (int y = 1; y < messageLen - 1 ; y++){
              newChannel[y-1] = message[y];
            }
            channelArray[x] = newChannel;
          }
        }

        // if not in array append "newChannel" string as defined above to the array
        if (inArray == false){ 
          for (int y = 1; y < messageLen - 1 ; y++){
            newChannel[y-1] = message[y];
          }
          //Serial.println("DEBUG: " + (String)channelArrayLength);
          channelArray[channelArrayLength] = newChannel;
          channelArrayLength ++;
        }

        //sorts the array so that it is in alphabetical order
        //this is done after each addition to the array to ensure easy displaying of array contents
        for (int x = 0; x < channelArrayLength; x++){
          for (int y = 0; y < channelArrayLength - 1; y++){
            if (channelArray[y] > channelArray[y+1]){
              String temp = channelArray[y+1];
              channelArray[y+1] = channelArray[y];
              channelArray[y] = temp;
            }
          }
        }
        /*//prints out he array and ength of the array for debugging purposes
        for (int x = 0; x < channelArrayLength; x++){
          Serial.println("DEBUG: " + channelArray[x]);
        }
        Serial.println(channelArrayLength);
        */
        state = WAITING;
        break;
      }
    case VALUE://  when a 'v' is detected
      {
        Serial.println("DEBUG: VALUE");
        //initalise a local value and give it a value
        String newVal;
        for (unsigned x=2; x < message.length() - 1;x++){
          newVal = newVal + message[x];
        }
        //Serial.println("-"+newVal+"-");

        //ignore if the value is outside the range 0 - 255
        if(newVal.toInt() < 0 or newVal.toInt() > 255){
          state = WAITING;
          break;
        }

        //Serial.println(newVal.length());
        char chan = message[1];
        for (int x = 0; x < 26; x++){
          if (channelArray[x][0] == chan){// if the channel has been initialised replace the contents of the channel with this, if not ignore
            Serial.println("Found channel");
            //reduce the timers for the display colour coding based on recent values
            if (screenRedCount > 0){
              screenRedCount--;
            }
            if (screenGreenCount > 0){
              screenGreenCount--;
            }
            Serial.println(screenGreenCount);
            //right-pad the values as being stored
            if (newVal.length() == 1){ // if one digit, pad with two spaces
              channelArray[x][16] = ' ';
              channelArray[x][17] = ' ';
              channelArray[x][18] = newVal[0];
              //Serial.println("Size 1");
            }else if(newVal.length() == 2){// if two digits, pad with one space
              channelArray[x][16] = ' ';
              channelArray[x][17] = newVal[0];
              channelArray[x][18] = newVal[1];
              //Serial.println("Size 2");
            }else if(newVal.length() == 3){//if three digits do not pad
              channelArray[x][16] = newVal[0];
              channelArray[x][17] = newVal[1];
              channelArray[x][18] = newVal[2];
              //Serial.println("Size 3");
            }
          }
        }
        /*//debugging the array
        for (int x = 0; x < channelArrayLength; x++){
          Serial.println("DEBUG: " + channelArray[x]);
        }*/

        state = WAITING;
        break;
      }
    case MAX:// when an 'x' is detected
      {
        Serial.println("DEBUG: MAX");
        
        //initialise a local variable for max and give it a value
        String newMax;
        for (unsigned x=2; x < message.length() - 1;x++){
          newMax = newMax + message[x];
        }
        //Serial.println(newMax);
        // if max outside 0-255 range then ignore
        if(newMax.toInt() < 0 or newMax.toInt() > 255){
          state = WAITING;
          break;
        }
        
        //Serial.println(newMax.length());
        char chan = message[1];
        for (int x = 0; x < 26; x++){
          if (channelArray[x][0] == chan){ //if channel has been initialised
            //Serial.println("Found channel");
            //handles padding of max, same system as for new values above
            if (newMax.length() == 1){
              channelArray[x][22] = ' ';
              channelArray[x][23] = ' ';
              channelArray[x][24] = newMax[0];
              //Serial.println("Size 1");
            }else if(newMax.length() == 2){
              channelArray[x][22] = ' ';
              channelArray[x][23] = newMax[0];
              channelArray[x][24] = newMax[1];
              //Serial.println("Size 2");
            }else if(newMax.length() == 3){
              channelArray[x][22] = newMax[0];
              channelArray[x][23] = newMax[1];
              channelArray[x][24] = newMax[2];
              //Serial.println("Size 3");
            }
          }
        }
        /*
        for (int x = 0; x < channelArrayLength; x++){
          Serial.println("DEBUG: " + channelArray[x]);
        }*/
        
        state = WAITING;
        break;
      } 
    case MIN:// when an 'n' is detected
      {
        Serial.println("DEBUG: MIN");

        //create a local min variable and give it a value
        String newMin;
        for (unsigned x=2;x < message.length() - 1;x++){
          newMin = newMin + message[x];
        }
        //Serial.println(newMin);

        if(newMin.toInt() < 0 or newMin.toInt() > 255){// ignore if outside range
          state = WAITING;
          break;
        }
        
        //Serial.println(newMin.length());
        char chan = message[1];
        for (int x = 0; x < 26; x++){ // change 26 to channelArrayNumber
          if (channelArray[x][0] == chan){ // if channel has been initialised
            //Serial.println("Found channel");
            //right-pad min, same as with values and max
            if (newMin.length() == 1){
              channelArray[x][19] = ' ';
              channelArray[x][20] = ' ';
              channelArray[x][21] = newMin[0];
              //Serial.println("Size 1");
            }else if(newMin.length() == 2){
              channelArray[x][19] = ' ';
              channelArray[x][20] = newMin[0];
              channelArray[x][21] = newMin[1];
              //Serial.println("Size 2");
            }else if(newMin.length() == 3){
              channelArray[x][19] = newMin[0];
              channelArray[x][20] = newMin[1];
              channelArray[x][21] = newMin[2];
              //Serial.println("Size 3");
            }
          }
        }/*
        for (int x = 0; x < channelArrayLength; x++){
          Serial.println("DEBUG: " + channelArray[x]);
        }*/
        
        state = WAITING;
        break;
      }
   }
}
