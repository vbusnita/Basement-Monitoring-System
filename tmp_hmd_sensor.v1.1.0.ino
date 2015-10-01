#include "PietteTech_DHT.h"

// system defines
#define DHTTYPE  DHT22                             // Sensor type DHT11/21/22/AM2301/AM2302
#define DHTPIN   A0         	                     // Digital pin used for communications
#define DHT_SAMPLE_INTERVAL  60 * 1000             // Sample every minute (millisecond value)
#define alarmTimerDelay 15 * 60000                 //Delay the alarm for 15 min (millisecond value)

//assume LCI's are idle until we check
String lci1Value = "false";
String lci2Value = "false";

//declaration
void dht_wrapper();    // must be declared before the lib initialization

// Lib instantiate
  PietteTech_DHT DHT(DHTPIN, DHTTYPE, dht_wrapper);

// globals
unsigned long DHTnextSampleTime;	        // Next time we want to start sample
unsigned long alarmTimer;
unsigned long dfuPushTimer;

int lci1 = D0;                           // Assign pin D0 of the core to lci1
int lci2 = D1;                          // Assign pin D1 of the core to lci2
int soundAlarm = D2;                     // Assign pin D2 of the core to the Buzzer

int port = 0;
int button = D3;
int val = 0;
int ledGreen = D4;
int ledRed = D5;

// Variables will change:
int ledGreenState = HIGH;         // the current state of the output pin
int ledRedState = LOW;
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
boolean useDebugPort = LOW;

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0;  // the last time the output pin was toggled
long debounceDelay = 50;    // the debounce time; increase if the output flickers

//Particle online variables
int tmp = 0;
int hmd = 0;

//Function Prototypes
void checkForDebugMode(void);
boolean sensorStatus(int value);  // Function for case statement
void sendData(String request, boolean debugging);
boolean lci1ExposedToWater(void);
boolean lci2ExposedToWater(void);
boolean checkLCIs(void);
int startAlarm(String command);

void setup()
{
    Time.zone(-4); //Set the time zone for the time function
    //Open the serial com port for debugging
    Serial.begin(9600);
    delay(3000); //Wait 3sec so we can start the serial monitor

    Serial.println("Program started !");

    //Configure core pins as INPUT/OUTPUT pins
    pinMode(lci1, INPUT);
    pinMode(lci2, INPUT);
    pinMode(soundAlarm, OUTPUT);
    pinMode(ledGreen, OUTPUT);
    pinMode(ledRed, OUTPUT);
    pinMode(button, INPUT);

    //Set the debug LEDs
    digitalWrite(ledRed, ledRedState);
    digitalWrite(ledGreen, ledGreenState);

    //Cloud variable and function declarations
    Spark.variable("temp", &tmp, INT);
    Spark.variable("humidity", &hmd, INT);
    Spark.function("soundAlarm", startAlarm);

    DHTnextSampleTime = 0; //Set the first sample time to start immediately
    alarmTimer = 0; //Set the alarm to trigger at first read
    dfuPushTimer = 0;
}

//Define wrapper in charge of calling like this for the PietteTech_DHT lib to work
void dht_wrapper() {DHT.isrCallback();}

void loop() {

    checkForDebugMode();  //Verify if the debug button was pressed

    //Verify if any LCI is tripped, sound Alarm if needed and set the proper values to send to the Server for analisys
    if(checkLCIs()) {
      if(millis() > alarmTimer) {
        startAlarm("startAlarm");
        alarmTimer = millis() + alarmTimerDelay; //Wait 15 min before repeating alarm if needed
      }
    }
    //Verify DHT sensor status, get the data from sensors, and send data to server for analisys
    if (millis() > DHTnextSampleTime) {   // Check if we need to start the next sample

                          // get DHT status
                          int result = DHT.acquireAndWait();
                            if(sensorStatus(result)) {

                              float humidity = DHT.getHumidity(); //Get the humidity reading
                              float degrees = DHT.getCelsius();   //Get the temperature reading

                              //Assign values for the Spark variables
                              tmp = degrees;
                              hmd = humidity;

                              //Format the request string in json format for the POST request
                              String request = "{\"sourceName\":\"Lyra\",\"temperature\":\"" + String(degrees) + "\",\"humidity\":\"" +
                              String(humidity) + "\",\"lci1\":\"" + String(lci1Value) + "\",\"lci2\":\"" + String(lci2Value) + "\"}";

                              //Send the request string and the status of the debug port usage to the sendData function
                              sendData(request, useDebugPort);

                              DHTnextSampleTime = millis() + DHT_SAMPLE_INTERVAL;  // set the time for next sample

                        } else {
                              //Notify the serial monitor of error with the sensor reading and place Photon in DFU mode for reflashing
                              Serial.println("An error occured while reading the DHT22 sensor...");
                              delay(2000);
                              Spark.publish("basement_leak", "DHT22 sensor ERROR!\nLyra is put in DFU mode!", 60, PRIVATE);
                              System.dfu();
                            }
        } // sample processing acording to delay
} //end main loop()

boolean sensorStatus(int value) {
  boolean state = false;
      Serial.print("Read sensor: ");
        switch (value) {
          case DHTLIB_OK:
              Serial.println("OK");
              state = true;
              break;
          case DHTLIB_ERROR_CHECKSUM:
              Serial.println("Error\n\r\tChecksum error");
              break;
          case DHTLIB_ERROR_ISR_TIMEOUT:
              Serial.println("Error\n\r\tISR time out error");
              break;
          case DHTLIB_ERROR_RESPONSE_TIMEOUT:
              Serial.println("Error\n\r\tResponse time out error");
              break;
          case DHTLIB_ERROR_DATA_TIMEOUT:
              Serial.println("Error\n\r\tData time out error");
              break;
          case DHTLIB_ERROR_ACQUIRING:
              Serial.println("Error\n\r\tAcquiring");
              break;
          case DHTLIB_ERROR_DELTA:
              Serial.println("Error\n\r\tDelta time to small");
              break;
          case DHTLIB_ERROR_NOTSTARTED:
              Serial.println("Error\n\r\tNot started");
              break;
          default:
              Serial.println("Unknown error");
              break;
          } //end switch
        return state;
}

void checkForDebugMode() {

  // read the state of the switch into a local variable:
  int reading = digitalRead(button);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH),  and you've waited
  // long enough since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer
    // than the debounce delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;
    // only toggle the LED if the new button state is HIGH
    if (buttonState == LOW) {
      ledRedState = !ledRedState;
      ledGreenState = !ledGreenState;
      useDebugPort = !useDebugPort;
      }
    }
  }
  // set the LED:
  digitalWrite(ledRed, ledRedState);
  digitalWrite(ledGreen, ledGreenState);
  // save the reading.  Next time through the loop,
  // it'll be the lastButtonState:
  lastButtonState = reading;
  return;
  } //End of checkForDebugMode function

void sendData(String request, boolean debugging) {

    Serial.print("\nDebug mode: ");
    Serial.println(port);
    Serial.println("Sending POST request: " + request);    //Sends these line to the console
    Serial.print(Time.timeStr());
    Serial.print("\n\n");

    if (debugging)
        Spark.publish("send_owl_data_debug", request, 60, PRIVATE);  //PRIVATE means nobody else can subscribe to my events
    else
        Spark.publish("send_owl_data", request, 60, PRIVATE);

} //end of sendData()

boolean lci1ExposedToWater() {
//Conditional statement the verifies is the water sensor is triggered
if(digitalRead(lci1) == LOW)
  return true;  //If water is present on either sensor return the value '1' (true)
    return false; //If water is NOT present return the value '0' (false)
}

boolean lci2ExposedToWater() {
  if(digitalRead(lci2) == LOW)
    return true;
      return false;   //If water is NOT present return the value '0' (false)
}

boolean checkLCIs() {
  boolean value = false;
  //Check the water sensors
  if(lci1ExposedToWater() || lci2ExposedToWater()) {
      if(lci1ExposedToWater()) {
        lci1Value = "true";
        value = true;
      } else lci1Value = "false";

      if(lci2ExposedToWater()) {
        lci2Value = "true";
        value = true;
      } else lci2Value = "false";
    }
    return value;
} //end checkLCIs function

int startAlarm(String command) {
if(command == "startAlarm") {

        //Publish webhook request to Pushover to push notification of leak to my iPhone
        Spark.publish("basement_leak", "Basement leak detected. Verify immediately!", 60, PRIVATE);
        Spark.publish("hue_alarm_start", NULL, 60, PRIVATE); //Start the Hue lights alarm

        //Trigger the alarm by making the pin 'D2' of the core 'HIGH'
        digitalWrite(soundAlarm, HIGH);
        delay(30000);   //Wait 30 seconds
        digitalWrite(soundAlarm, LOW);  //Turn off alarm

        //Stop the Hue lights alarm
        Spark.publish("hue_alarm_stop", NULL, 60, PRIVATE);
        return 1;
    }
else return -1;
}//end of startAlarm() function
