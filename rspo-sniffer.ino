//--------------------------------------------------------------------------------------------------
// 29.08.2020 - Stephan Rau: initial version
//--------------------------------------------------------------------------------------------------
// Assumptions:
//  - long low phase is start sequence
//  - data is not more than 32 bits, else long variable for code is not enough
//  - a zero is repesented by a longer low than high phase
//  - a one is repesentend by a longer high than low phase
//--------------------------------------------------------------------------------------------------

const unsigned int min_init  = 2000; // min low phase of init phase in us
const unsigned int min_valid = 150;  // min duration time for valid pulse
const byte RF_RCV_PIN        = 2;    // main rcv input
const byte RF_VAL_PIN        = 13;   // copy rcv value to on board LED

// serial strings ----------------------------------------------------------------------------------
const String ENABLE_CMD     = "on";  // enable sniffing
const String DISABLE_CMD    = "off"; // disable sniffing
const char   CMD_END        = '\n';  // character for command end

// global variables --------------------------------------------------------------------------------
static bool   sniff_on      = false;
static String serialInStr;           // read string from serial port


//--------------------------------------------------------------------------------------------------
void setup() {
  Serial.begin(19200); // 9600 higher speed needed else print is not finshed before next interrupt
  pinMode(RF_VAL_PIN, OUTPUT);
  pinMode(RF_RCV_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(RF_RCV_PIN), count_time, CHANGE);
}


//--------------------------------------------------------------------------------------------------
void loop() {

  // indicate that rcv is receiving something
  digitalWrite(RF_VAL_PIN, digitalRead(RF_RCV_PIN));

  // check and read command from PC
  if ( Serial.available() > 0 ) {
    serialInStr = Serial.readStringUntil(CMD_END);

    if ( serialInStr == DISABLE_CMD ) {
      sniff_on = false;
    }

    if ( serialInStr == ENABLE_CMD ) {

      // print header line

      // start sequence
      Serial.print("Start Seq High,");
      Serial.print("Start Seq Low,");

      // zero encoding
      Serial.print("Zeros High,");
      Serial.print("Zeros Low,");

      // one encoding
      Serial.print("Ones High,");
      Serial.print("Ones Low,");

      // number of bits counted, should be 24 in most cases for rc switches
      Serial.print("Num of Bits,");

      // finally the read code value
      Serial.println("Code");

      sniff_on = true;
    }
  }
}


//--------------------------------------------------------------------------------------------------
void count_time() {

  // variables needed for calculation (static do they do not get deleted when function finished)
  static unsigned int currentTime = 0;
  static unsigned int lastTime = 0;
  static unsigned int duration = 0;
  static unsigned int lastDuration = 0;
  static byte lastState = 0;
  static byte state = 0;
  static byte count_data_bits = 0;
  static unsigned int zeros_high = 0;
  static unsigned int zeros_low = 0;
  static unsigned int ones_high = 0;
  static unsigned int ones_low = 0;
  static unsigned long code = 0;

  if ( sniff_on ) {

    currentTime = micros();
    duration = currentTime - lastTime;
    state = digitalRead(RF_RCV_PIN);

    // last duration was a long low => start phase  => print sniffed data set
    if ( (duration > min_init) && (lastState == 0) ) {

      // printing is reduces as much as possible,
      // else it tooks too much time and the result gets falsified

      // start sequence
      Serial.print(lastDuration);     Serial.print(","); // High
      Serial.print(duration);         Serial.print(","); // Low
      // zero encoding
      Serial.print(zeros_high);       Serial.print(","); // High
      Serial.print(zeros_low);        Serial.print(","); // Low
      // one encoding
      Serial.print(ones_high);        Serial.print(","); // High
      Serial.print(ones_low);         Serial.print(","); // Low
      // number of bits counted, should be 24 in most cases for rc switches
      Serial.print(count_data_bits);  Serial.print(",");
      // finaly the read code value
      Serial.println(code, HEX);

      // reset everything for the next data set
      count_data_bits = 0;
      code = 0;
    } else {
      
      // after start sequence evaluation all data on next high phase
      // last bit will be evaled on high phase of start seqeunce
      
      // if printing took longer than high phase of first bit stange things could happen
      if ( state == 1 ) {
        
        count_data_bits++;
        code <<= 1; // shift by one
        
        //    high phase  > low phase
        if ( lastDuration > duration  && (lastState == 0) ) {
          ones_high = lastDuration;
          ones_low = duration;
          code |= 1; // set current value to 1
        } else {
          zeros_high = lastDuration;
          zeros_low = duration;
          //code &= 0; // set current value to 0 (not needed is default)
        }
        
      }
    }

    // save data for evaluation in next change phase
    lastTime = currentTime;
    lastState = state;
    lastDuration = duration;
    
  }

}
