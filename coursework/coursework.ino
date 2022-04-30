#include <Wire.h>
#include <Adafruit_RGBLCDShield.h>
#include <utility/Adafruit_MCP23017.h>

//todo:
//streamline code

//---------------------------
//UDCHARS: 24
//FREERAM: 64, 213
//NAMES: 124, 152, 163
//SCROLL: 132, 186, 222
//---------------------------

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

struct channel {
  char id;
  char description[16] = "               ";
  int value = -1;
  int minValue = 0;
  int maxValue = 255;
};

// definition of arrow characters for UI --------------------------------------------------------------------------------
byte upArrow[8] = {
  B00100,
  B01110,
  B11111,
  B00100,
  B00100,
  B00100,
  B00100,
  B00100,
};

byte downArrow[8] = {
  B00100,
  B00100,
  B00100,
  B00100,
  B00100,
  B11111,
  B01110,
  B00100,
};

void setup() {
  lcd.begin(16,2);
  lcd.setBacklight(7);
  lcd.clear();
  Serial.begin(9600);
  lcd.createChar(0, upArrow);
  lcd.createChar(1, downArrow);
}

enum state_e { SYNCHRONISATION = 3, INITIALISATION, WAITING, NEW_CHANNEL, VALUE, MAX, MIN }; // the main states
enum state_b { WAITING_PRESS = 8, WAITING_RELEASE }; // states for pressing the buttons

//static String channelArray[26];//global declaration of the array and the colours for the screen
static channel channelArray[26];
static int screenRedCount = 0;
static int screenGreenCount = 0;
static bool needScroll1;
static bool needScroll2;
static unsigned long now1 = millis();
static unsigned long now2 = millis();
static int scrollCount1;
static int scrollCount2;

//code sourced from lab worksheet 3, returns the free memory that the arduino has --------------------------------------------
#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char* sbrk(int incr);
#else // __ARM__
extern char *__brkval;
#endif // __arm__

int freeMemory() {
char top;
#ifdef __arm__
return &top - reinterpret_cast<char*>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
return &top - __brkval;
#else // __arm__
return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif // __arm__
}

//handles the displaying of arrows for scrolling
void atTop(){ 
  lcd.setCursor(0,0);
  lcd.print(' ');
  lcd.setCursor(0,1);
  lcd.write(byte(1));
}
void atBtm(){
  lcd.setCursor(0,0);
  lcd.write(byte(0));
  lcd.setCursor(0,1);
  lcd.print(' ');
}
void inMiddle(){
  lcd.setCursor(0,0);
  lcd.write(byte(0));
  lcd.setCursor(0,1);
  lcd.write(byte(1));
}

//updates the disply with current values
void updateDisplay(int channelArrayLength,int topDisplay){
  String dispVal;
  if(channelArrayLength == 1){ // if one value in array
    lcd.setCursor(1,0);
    if(channelArray[topDisplay].value > 99){
      dispVal = (String)channelArray[topDisplay].value;
    }else if(channelArray[topDisplay].value > 9){
      dispVal =  " " + (String)channelArray[topDisplay].value;
    }else{
      dispVal = "  " + (String)channelArray[topDisplay].value;
    }
    Serial.println('-'+dispVal+'-');
    /*Serial.println("DEBUG: " + (String) channelArrayLength);
    Serial.println("DEBUG: " + (String) topDisplay);*/
    lcd.print(channelArray[topDisplay].id);
    if (channelArray[topDisplay].value > -1){
      lcd.print(dispVal);
    }else{
      lcd.print("   ");
    }
    lcd.print(" ");// displaying name--------------------------------------------------------------------------
    //this will determine whether the name is long enough to require scrolling
    needScroll1 = false;
    for (int y = 10; y < 15; y++){
      if (channelArray[topDisplay].description[y] != ' '){
        needScroll1 = true;
      }
    }
    if (needScroll1 == true){
      //Serial.println("Needs to scroll");
      //now = millis();
      if(scrollCount1 > 5){
        scrollCount1 = 0;
      }
      
      for (int x = 0; x < 10; x++){
        lcd.write(channelArray[topDisplay].description[x+scrollCount1]);
      }
      if (millis() - now1 > 500){
        scrollCount1++;
        now1 = millis();
      }
      
    }else{
      for (int x = 0; x < 10; x++){
        lcd.print(channelArray[topDisplay].description[x]);
      } 
    }
    
  }else if(channelArrayLength > 1){ // if two values or more
    //copy above, but include two lines (topDisplay and topDisplay + 1)
    lcd.setCursor(1,0);
    if(channelArray[topDisplay].value > 99){
      dispVal = (String)channelArray[topDisplay].value;
    }else if(channelArray[topDisplay].value > 9){
      dispVal =  " " + (String)channelArray[topDisplay].value;
    }else{
      dispVal = "  " + (String)channelArray[topDisplay].value;
    }
    String dispVal2;
    if(channelArray[topDisplay + 1].value > 99){
      dispVal2 = (String)channelArray[topDisplay + 1].value;
    }else if(channelArray[topDisplay + 1].value > 9){
      dispVal2 =  " " + (String)channelArray[topDisplay + 1].value;
    }else{
      dispVal2 = "  " + (String)channelArray[topDisplay + 1].value;
    }
    lcd.print(channelArray[topDisplay].id);
    if (channelArray[topDisplay].value > -1){
      lcd.print(dispVal);
    }else{
      lcd.print("   ");
    }
    lcd.print(" ");// displaying name--------------------------------------------------------------------------
    
    needScroll1 = false;// determines if top half needs to scroll
    for (int y = 10; y < 15; y++){
      if (channelArray[topDisplay].description[y] != ' '){
        needScroll1 = true;
      }
    }
    if (needScroll1 == true){
      Serial.println("Needs to scroll");
      //now1 = millis();
      if(scrollCount1 > 5){//5 chars don't fit
        scrollCount1 = 0;
      }
      
      for (int x = 0; x < 10; x++){
        lcd.write(channelArray[topDisplay].description[x+scrollCount1]);
      }
      if (millis() - now1 > 500){
        scrollCount1++;
        now1 = millis();
      }
      
    }else{
      for (int x = 0; x < 10; x++){
        lcd.print(channelArray[topDisplay].description[x]);
      } 
    }
    
  
    lcd.setCursor(1,1);
    lcd.print(channelArray[topDisplay + 1].id);
    if (channelArray[topDisplay + 1].value > -1){
      lcd.print(dispVal2);
    }else{
      lcd.print("   ");
    }
    lcd.print(" ");// displaying name--------------------------------------------------------------------------
    needScroll2 = false;// determines if top half needs to scroll
    for (int y = 10; y < 15; y++){
      if (channelArray[topDisplay + 1].description[y] != ' '){
        needScroll2 = true;
      }
    }
    if (needScroll2 == true){
      Serial.println("Needs to scroll");
      //now2 = millis();
      if(scrollCount2 > 5){//5 chars don't fit
        scrollCount2 = 0;
      }
      
      for (int x = 0; x < 10; x++){
        lcd.write(channelArray[topDisplay + 1].description[x+scrollCount2]);
      }
      if (millis() - now2 > 500){
        scrollCount2++;
        now2 = millis();
      }
      
    }else{
      for (int x = 0; x < 10; x++){
        lcd.print(channelArray[topDisplay + 1].description[x]);
      } 
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
    if (channelArray[x].maxValue >= channelArray[x].minValue){ // determine whether to change the colour of the display, colour will be changed until 5 new values hae been entered
      if (channelArray[x].value != -1){
        if (channelArray[x].value < channelArray[x].minValue){
          screenGreenCount = 5;
        }else if(channelArray[x].value > channelArray[x].maxValue){
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
  lcd.setCursor(0,1);
  lcd.print("FREE: " + (String)freeMemory()); // outputs the amount of free ram to the select button interface ---------------------------------------
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
        //static int updown;
        
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
                //updown = pressed;
                //}
              }
              break;           
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
        Serial.println("UDCHARS, FREERAM, NAMES, SCROLL"); // update with all extension tasks -------------------------------------------------------
        lcd.setBacklight(7);
        channelArrayLength = 0; //initialise length of channel array here
        state = WAITING;
        break;
      }
    case NEW_CHANNEL:// When a 'C' is the first character, create a new channel or rename old
      {
        Serial.println("DEBUG: NEW_CHANNEL");
        //Serial.println("-" + message + "-");
        channel newChannel;
        newChannel.id = message[1];
        int messageLen = message.length();
        if (messageLen > 18){ // descriptions longer than 15 characters are ignored
          messageLen = 18;  
        }

        bool inArray = false;
        // search array and if in array, overwrite description
        for (int x = 0; x < 26; x++){
          if(channelArray[x].id == newChannel.id){
            newChannel.value = channelArray[x].value;
            newChannel.maxValue = channelArray[x].maxValue;
            newChannel.minValue = channelArray[x].minValue;
            inArray = true;
            for(int y = 2; y < messageLen - 1; y++){
              newChannel.description[y-2] = message[y];
            }
            channelArray[x] = newChannel;
            
          }
        }
        if (inArray == false){ 
          for (int y = 2; y < messageLen - 1; y++){
            newChannel.description[y-2] = message[y];
          }
          //Serial.println("DEBUG: " + (String)channelArrayLength);
          channelArray[channelArrayLength] = newChannel;
          channelArrayLength ++;
        }

        //sorts the array so that it is in alphabetical order
        //this is done after each addition to the array to ensure easy displaying of array contents

        for (int x = 0; x < channelArrayLength; x++){
          for (int y = 0; y < channelArrayLength - 1; y++){
            if (channelArray[y].id > channelArray[y+1].id){
              channel tempChan = channelArray[y+1];
              channelArray[y+1] = channelArray[y];
              channelArray[y] = tempChan;
            }
          }
        }   
        
        
        for (int x = 0; x < channelArrayLength; x++){
          Serial.println("DEBUG: " + (String)channelArray[x].id);
          Serial.print("DEBUG: ");
          for (int y = 0; y<15; y++){
            Serial.print(channelArray[x].description[y]);
          }
          Serial.println();
          Serial.println("DEBUG: " + (String)channelArray[x].value);
          Serial.println("DEBUG: " + (String)channelArray[x].maxValue);
          Serial.println("DEBUG: " + (String)channelArray[x].minValue);
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
        int intVal = newVal.toInt();
        //Serial.println("-"+newVal+"-");

        //ignore if the value is outside the range 0 - 255
        if(intVal < 0 or intVal > 255){
          state = WAITING;
          break;
        }

        //Serial.println(newVal.length());
        char chan = message[1];
        
        for (int x = 0; x < 26; x++){
          if (channelArray[x].id == chan){// if the channel has been initialised replace the contents of the channel with this, if not ignore
            Serial.println("Found channel");
            //reduce the timers for the display colour coding based on recent values
            if (screenRedCount > 0){
              screenRedCount--;
            }
            if (screenGreenCount > 0){
              screenGreenCount--;
            }
            //right-pad the values as being stored
            channelArray[x].value = intVal;
          }
        }
        for (int x = 0; x < channelArrayLength; x++){
          Serial.println("DEBUG: " + (String)channelArray[x].id);
          Serial.print("DEBUG: ");
          for (int y = 0; y<15; y++){
            Serial.print(channelArray[x].description[y]);
          }
          Serial.println();
          Serial.println("DEBUG: " + (String)channelArray[x].value);
          Serial.println("DEBUG: " + (String)channelArray[x].maxValue);
          Serial.println("DEBUG: " + (String)channelArray[x].minValue);
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
        int intMax = newMax.toInt();
        //Serial.println(newMax);
        // if max outside 0-255 range then ignore
        if(intMax < 0 or intMax > 255){
          state = WAITING;
          break;
        }
        
        //Serial.println(newMax.length());
        char chan = message[1];

        for (int x = 0; x < 26; x++){
          if (channelArray[x].id == chan){ //if channel has been initialised
            Serial.println("Found channel");
            channelArray[x].maxValue = intMax;
            }
        }
        
        for (int x = 0; x < channelArrayLength; x++){
          Serial.println("DEBUG: " + (String)channelArray[x].id);
          Serial.print("DEBUG: ");
          for (int y = 0; y<15; y++){
            Serial.print(channelArray[x].description[y]);
          }
          Serial.println();
          Serial.println("DEBUG: " + (String)channelArray[x].value);
          Serial.println("DEBUG: " + (String)channelArray[x].maxValue);
          Serial.println("DEBUG: " + (String)channelArray[x].minValue);
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
        int intMin = newMin.toInt();
        //Serial.println(newMin);

        if(intMin < 0 or intMin > 255){// ignore if outside range
          state = WAITING;
          break;
        }
        
        //Serial.println(newMin.length());
        char chan = message[1];
        
        for (int x = 0; x < 26; x++){ // change 26 to channelArrayNumber
          if (channelArray[x].id == chan){ // if channel has been initialised
            Serial.println("Found channel");
            channelArray[x].minValue = intMin;
          }
        }
        
        for (int x = 0; x < channelArrayLength; x++){
          Serial.println("DEBUG: " + (String)channelArray[x].id);
          Serial.print("DEBUG: ");
          for (int y = 0; y<15; y++){
            Serial.print(channelArray[x].description[y]);
          }
          Serial.println();
          Serial.println("DEBUG: " + (String)channelArray[x].value);
          Serial.println("DEBUG: " + (String)channelArray[x].maxValue);
          Serial.println("DEBUG: " + (String)channelArray[x].minValue);
        }
        
        state = WAITING;
        break;
      }
   }
}
