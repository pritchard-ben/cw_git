#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>

//todo:
//rename nVal to newVal etc for val, min, max
//streamline code

//---------------------------

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

struct channel {
  char id;
  char description[16] = "               ";
  int value = -1;
  int minValue = 0;
  int maxValue = 255;
};

void setup() {
  lcd.begin(16,2);
  lcd.setBacklight(7);
  lcd.clear();
  Serial.begin(9600);
}

enum state_e { SYNCHRONISATION = 3, INITIALISATION, WAITING, NEW_CHANNEL, VALUE, MAX, MIN }; // the main states
enum state_b { WAITING_PRESS = 8, WAITING_RELEASE }; // states for pressing the buttons

//static String channelArray[26];//global declaration of the array and the colours for the screen
static channel newChannelArray[26];
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
  String dispVal;
  if(channelArrayLength == 1){ // if one value in array
    lcd.setCursor(1,0);
    if(newChannelArray[topDisplay].value > 99){
      dispVal = (String)newChannelArray[topDisplay].value;
    }else if(newChannelArray[topDisplay].value > 9){
      dispVal =  " " + (String)newChannelArray[topDisplay].value;
    }else{
      dispVal = "  " + (String)newChannelArray[topDisplay].value;
    }
    Serial.println('-'+dispVal+'-');
    /*Serial.println("DEBUG: " + (String) channelArrayLength);
    Serial.println("DEBUG: " + (String) topDisplay);*/
    lcd.print(newChannelArray[topDisplay].id);
    if (newChannelArray[topDisplay].value > -1){
      lcd.print(dispVal);
    }else{
      lcd.print("   ");
    }
  }else if(channelArrayLength > 1){ // if two values
    //copy above, but include two lines (topDisplay and topDisplay + 1)
    lcd.setCursor(1,0);
    if(newChannelArray[topDisplay].value > 99){
      dispVal = (String)newChannelArray[topDisplay].value;
    }else if(newChannelArray[topDisplay].value > 9){
      dispVal =  " " + (String)newChannelArray[topDisplay].value;
    }else{
      dispVal = "  " + (String)newChannelArray[topDisplay].value;
    }
    String dispVal2;
    if(newChannelArray[topDisplay + 1].value > 99){
      dispVal2 = (String)newChannelArray[topDisplay + 1].value;
    }else if(newChannelArray[topDisplay + 1].value > 9){
      dispVal2 =  " " + (String)newChannelArray[topDisplay + 1].value;
    }else{
      dispVal2 = "  " + (String)newChannelArray[topDisplay + 1].value;
    }
    lcd.print(newChannelArray[topDisplay].id);
    if (newChannelArray[topDisplay].value > -1){
      lcd.print(dispVal);
    }else{
      lcd.print("   ");
    }
    lcd.setCursor(1,1);
    lcd.print(newChannelArray[topDisplay + 1].id);
    if (newChannelArray[topDisplay + 1].value > -1){
      lcd.print(dispVal2);
    }else{
      lcd.print("   ");
    }
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

  for (int x = 0; x < channelArrayLength; x++){
    if (newChannelArray[x].maxValue >= newChannelArray[x].minValue){ // determine whether to change the colour of the display, colour will be changed until 5 new values hae been entered
      if (newChannelArray[x].value != -1){
        if (newChannelArray[x].value < newChannelArray[x].minValue){
          screenGreenCount = 5;
        }else if(newChannelArray[x].value > newChannelArray[x].maxValue){
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
        channel newChan;
        newChan.id = message[1];
        //Serial.println("-"+newChannel+"-");
        int messageLen = message.length();
        if (messageLen > 15){ // descriptions longer than 15 characters are ignored
          messageLen = 15;  
        }
                
        //Serial.println("-"+newChannel+"-");

        bool inArray = false;
        // search array and if in array, overwrite description
        for (int x = 0; x < 26; x++){
          if(newChannelArray[x].id == newChan.id){
            newChan.value = newChannelArray[x].value;
            newChan.maxValue = newChannelArray[x].maxValue;
            newChan.minValue = newChannelArray[x].minValue;
            inArray = true;
            for(int y = 2; y < messageLen - 1; y++){
              newChan.description[y-2] = message[y];
            }
            newChannelArray[x] = newChan;
            
          }
        }

        // if not in array append "newChannel" string as defined above to the array
        if (inArray == false){ 
          for (int y = 1; y < messageLen - 1 ; y++){
            newChannel[y-1] = message[y];
          }
          for (int y = 2; y < messageLen - 1; y++){
            newChan.description[y-2] = message[y];
          }
          //Serial.println("DEBUG: " + (String)channelArrayLength);
          newChannelArray[channelArrayLength] = newChan;
          channelArrayLength ++;
        }

        //sorts the array so that it is in alphabetical order
        //this is done after each addition to the array to ensure easy displaying of array contents

        for (int x = 0; x < channelArrayLength; x++){
          for (int y = 0; y < channelArrayLength - 1; y++){
            if (newChannelArray[y].id > newChannelArray[y+1].id){
              channel tempChan = newChannelArray[y+1];
              newChannelArray[y+1] = newChannelArray[y];
              newChannelArray[y] = tempChan;
            }
          }
        }
        
        for (int x = 0; x < channelArrayLength; x++){
          Serial.println("DEBUG: " + (String)newChannelArray[x].id);
          Serial.print("DEBUG: ");
          for (int y = 0; y<15; y++){
            Serial.print(newChannelArray[x].description[y]);
          }
          Serial.println();
          Serial.println("DEBUG: " + (String)newChannelArray[x].value);
          Serial.println("DEBUG: " + (String)newChannelArray[x].maxValue);
          Serial.println("DEBUG: " + (String)newChannelArray[x].minValue);
        }
        Serial.println(channelArrayLength);
        
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
        int nVal = newVal.toInt();
        //Serial.println("-"+newVal+"-");

        //ignore if the value is outside the range 0 - 255
        if(newVal.toInt() < 0 or newVal.toInt() > 255){
          state = WAITING;
          break;
        }

        //Serial.println(newVal.length());
        char chan = message[1];
        
        for (int x = 0; x < 26; x++){
          if (newChannelArray[x].id == chan){// if the channel has been initialised replace the contents of the channel with this, if not ignore
            Serial.println("Found channel");
            //reduce the timers for the display colour coding based on recent values
            if (screenRedCount > 0){
              screenRedCount--;
            }
            if (screenGreenCount > 0){
              screenGreenCount--;
            }
            //right-pad the values as being stored
            newChannelArray[x].value = nVal;
          }
        }
        for (int x = 0; x < channelArrayLength; x++){
          Serial.println("DEBUG: " + (String)newChannelArray[x].id);
          Serial.print("DEBUG: ");
          for (int y = 0; y<15; y++){
            Serial.print(newChannelArray[x].description[y]);
          }
          Serial.println();
          Serial.println("DEBUG: " + (String)newChannelArray[x].value);
          Serial.println("DEBUG: " + (String)newChannelArray[x].maxValue);
          Serial.println("DEBUG: " + (String)newChannelArray[x].minValue);
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
        int nMax = newMax.toInt();
        //Serial.println(newMax);
        // if max outside 0-255 range then ignore
        if(newMax.toInt() < 0 or newMax.toInt() > 255){
          state = WAITING;
          break;
        }
        
        //Serial.println(newMax.length());
        char chan = message[1];

        for (int x = 0; x < 26; x++){
          if (newChannelArray[x].id == chan){ //if channel has been initialised
            Serial.println("Found channel");
            newChannelArray[x].maxValue = nMax;
            }
        }
        
        for (int x = 0; x < channelArrayLength; x++){
          Serial.println("DEBUG: " + (String)newChannelArray[x].id);
          Serial.print("DEBUG: ");
          for (int y = 0; y<15; y++){
            Serial.print(newChannelArray[x].description[y]);
          }
          Serial.println();
          Serial.println("DEBUG: " + (String)newChannelArray[x].value);
          Serial.println("DEBUG: " + (String)newChannelArray[x].maxValue);
          Serial.println("DEBUG: " + (String)newChannelArray[x].minValue);
        }
        
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
        int nMin = newMin.toInt();
        //Serial.println(newMin);

        if(newMin.toInt() < 0 or newMin.toInt() > 255){// ignore if outside range
          state = WAITING;
          break;
        }
        
        //Serial.println(newMin.length());
        char chan = message[1];
        
        for (int x = 0; x < 26; x++){ // change 26 to channelArrayNumber
          if (newChannelArray[x].id == chan){ // if channel has been initialised
            Serial.println("Found channel");
            newChannelArray[x].minValue = nMin;
          }
        }
        
        for (int x = 0; x < channelArrayLength; x++){
          Serial.println("DEBUG: " + (String)newChannelArray[x].id);
          Serial.print("DEBUG: ");
          for (int y = 0; y<15; y++){
            Serial.print(newChannelArray[x].description[y]);
          }
          Serial.println();
          Serial.println("DEBUG: " + (String)newChannelArray[x].value);
          Serial.println("DEBUG: " + (String)newChannelArray[x].maxValue);
          Serial.println("DEBUG: " + (String)newChannelArray[x].minValue);
        }
        
        state = WAITING;
        break;
      }
   }
}
