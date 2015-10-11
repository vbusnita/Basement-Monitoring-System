/******************************************************************************************************************
 * @file    tmp_hmd_sensor.ino
 * @authors Victor Busnita, Sabin Dragut
 * @version V1.1.0
 * @date    10-October-2015
 * @brief   Basement Monitor Application
******************************************************************************************************************/

/* Includes -----------------------------------------------------------------------------------------------------*/
#include "PietteTech_DHT.h"                        //Library for controlling the DHT22 sensor

/* System Defines------------------------------------------------------------------------------------------------*/
#define DHTTYPE  DHT22                             //Sensor type DHT11/21/22/AM2301/AM2302
#define DHTPIN   A0         	                   //Digital pin used for communications
#define DHT_SAMPLE_INTERVAL  30 * 60000            //DHT22 Sample every 30 min (millisecond value)
#define alarmTimerDelay 15 * 60000                 //Delay the alarm for 15 min (millisecond value)

/* Declaration of DHT wrapper------------------------------------------------------------------------------------*/
void dht_wrapper();                                //Must be declared before the lib initialization

/* Lib instantiate-----------------------------------------------------------------------------------------------*/
  PietteTech_DHT DHT(DHTPIN, DHTTYPE, dht_wrapper);

/* Global Variables----------------------------------------------------------------------------------------------*/
unsigned long DHTnextSampleTime;	           //Next time we want to start sample
unsigned long alarmTimer;
unsigned long dfuPushTimer;

//Variables used to store the DHT22 sensor readings
float humidity = 0.000000;
float degrees = 0.000000;

String fwVersion = "v1.1.0";                      //String used to store the version of the application

//Variables used for the Groove water sensors
boolean lci1Value = "false";                      //Assume LCI's are idle until we check
boolean lci2Value = "false";                      //

int lci1 = D0;                                    //Assign pin D0 of the core to lci1
int lci2 = D1;                                    //Assign pin D1 of the core to lci2
int soundAlarm = D2;                              //Assign pin D2 of the core to the Buzzer

int port = 0;                                     //Set the debug port to 0
int button = D3;
int val = 0;
int ledGreen = D4;
int ledRed = D5;

// Variables used for the debug LED state:
int ledGreenState = HIGH;                         //The current state of the output pin
int ledRedState = LOW;
int buttonState;                                  //The current reading from the input pin
int lastButtonState = LOW;                        //The previous reading from the input pin
boolean useDebugPort = LOW;

// the following variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long lastDebounceTime = 0;                        //The last time the output pin was toggled
long debounceDelay = 50;                          //The debounce time; increase if the output flickers

int tmp = 0;                                      //Particle online variables
int hmd = 0;                                      //

int startTime;                                    //Variable to store the start time of the program

/* Function Prototypes--------------------------------------------------------------------------------------------*/
void checkForDebugMode(void);
boolean sensorStatus(int value);
void sendData(String request, boolean debugging);
boolean lci1ExposedToWater(void);
boolean lci2ExposedToWater(void);
int alarmModule(String command);
boolean envSensorModule(void);
boolean lciSensorModule(void);
int uptime(String command);

/* This function is called once at start up ----------------------------------------------------------------------*/
void setup()
{
    Time.zone(-4); //Set the time zone for the time function
    startTime = Time.now();
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
    Spark.function("alarmModule", alarmModule);
    Spark.function("uptime", uptime);

    DHTnextSampleTime = 0; //Set the first sample time to start immediately
    alarmTimer = 0; //Set the alarm to trigger at first read
}

/* Define wrapper in charge of calling like this for the PietteTech_DHT lib to work-------------------------------*/
void dht_wrapper() {DHT.isrCallback();}

/* This function loops forever -----------------------------------------------------------------------------------*/
void loop() {

    checkForDebugMode();  //Verify if the debug button was pressed
    if (lciSensorModule()) {
      Serial.println("The LCI sensor module detected a problem!");
      if(millis() > alarmTimer) {
        alarmModule("start");
        alarmTimer = millis() + alarmTimerDelay; //Wait 15 min before repeating alarm if needed
      }
    } else {
      alarmTimer = 0;
      lci1Value = false;
      lci2Value = false;
    }
    //Verify DHT sensor status, get the data from sensors, and send data to server for analisys
    // Check if we need to start the next sample
    if (millis() > DHTnextSampleTime) {

            if (!envSensorModule()) {
              //If the sensor is not ready, let serial monitor know and notify me
              Serial.println("An error occured with the DHT22 sensor module");
              delay(2000);
              Particle.publish("basement_leak", "DHT22 ERROR!", 60, PRIVATE);
              degrees = 0.000000;
              humidity = 0.000000;
            } else
              Serial.println("The DHT22 sensor module is good to go!\n");

            //Format the request string in json format for the POST request in sendData
            String request = "{\"sourceName\":\"Lyra\",\"fwVersion\":\"" + fwVersion + "\",\"temperature\":\"" +
            String(degrees) + "\",\"humidity\":\"" + String(humidity) + "\",\"lci1\":\"" + String(lci1Value) +
            "\",\"lci2\":\"" + String(lci2Value) + "\"}";

            //Send the request string and the status of the debug port usage to the sendData function
            sendData(request, useDebugPort);
            // set the time for next sample
            DHTnextSampleTime = millis() + DHT_SAMPLE_INTERVAL;
      }
}


/******************************************************************************************************************
 * Function Name  : envSensorModule
 * Description    : Verifies if the DHT22 sensor is ready to be asked for a new reading of temperature and humidity
 * Input          : A0
 * Output         : None
 * Return         : Value of (true or false) in boolean type
******************************************************************************************************************/
boolean envSensorModule(void) {
  boolean status = false;
  // get DHT status
  int result = DHT.acquireAndWait();
    Serial.print("Read sensor: ");
      switch (result) {
        case DHTLIB_OK:
            Serial.println("OK");
            status = true;
            break;
        case DHTLIB_ERROR_CHECKSUM:
            Serial.println("Error\n\r\tChecksum error");
            return status;
        case DHTLIB_ERROR_ISR_TIMEOUT:
            Serial.println("Error\n\r\tISR time out error");
            return status;
        case DHTLIB_ERROR_RESPONSE_TIMEOUT:
            Serial.println("Error\n\r\tResponse time out error");
            return status;
        case DHTLIB_ERROR_DATA_TIMEOUT:
            Serial.println("Error\n\r\tData time out error");
            return status;
        case DHTLIB_ERROR_ACQUIRING:
            Serial.println("Error\n\r\tAcquiring");
            return status;
        case DHTLIB_ERROR_DELTA:
            Serial.println("Error\n\r\tDelta time to small");
            return status;
        case DHTLIB_ERROR_NOTSTARTED:
            Serial.println("Error\n\r\tNot started");
            return status;
        default:
            Serial.println("Unknown error");
            return status;
        } //end switch

    humidity = DHT.getHumidity(); //Get the humidity reading
    degrees = DHT.getCelsius();   //Get the temperature reading
    //Assign values for the Spark variables
    tmp = degrees;
    hmd = humidity;
  return status;
}
/******************************************************************************************************************/

/******************************************************************************************************************
 * Function Name  : lciSensorModule
 * Description    : Depending on the return of the lci1ExposedToWater and lci2ExposedToWater functions, sets the
 *                  proper values for the lci1Value and lci2Value variables (true/false)
 * Input          : None
 * Output         : None
 * Return         : Value of (true or false) in boolean type
******************************************************************************************************************/
boolean lciSensorModule() {
    boolean status = false;
    //Check the water sensors
    if(lci1ExposedToWater() || lci2ExposedToWater()) {
        if(lci1ExposedToWater()) {
          lci1Value = true;
          status = true;
        } else lci1Value = false;

        if(lci2ExposedToWater()) {
          lci2Value = true;
          status = true;
        } else lci2Value = false;
      }
      return status;
  }
/******************************************************************************************************************/

/******************************************************************************************************************
 * Function Name  : checkForDebugMode
 * Description    : Verifies if the push button has been pressed (goes from LOW to HIGH), and causes the
 *                  'useDebugPort' variable to go to HIGH when push button pressed.
 * Input          : D3, (LOW to HIGH)
 * Output         : D4, D5 (HIGH to turn on the proper LED for debug)
 * Return         : None
******************************************************************************************************************/
void checkForDebugMode() {
  // read the state of the switch into a local variable:
  int reading = digitalRead(button);
  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState)
    lastDebounceTime = millis();

  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;
    // only toggle the LED if the new button state is LOW
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
  lastButtonState = reading;
  return;
}
/******************************************************************************************************************/

/******************************************************************************************************************
 * Function Name  : sendData
 * Description    : It sends data from sensor and other stuff to the Django database server throug a webhook
 *                  It uses the proper server depending on what the checkForDebugMode() function return
 * Takes in       : The formatted string 'request' to be sent to the Django database server and
                    the return of the checkForDebugMode function in boolean format (true/false)
 * Input          : None
 * Output         : None
 * Return         : None
******************************************************************************************************************/
void sendData(String request, boolean debugging) {

    //Print stuff on the serial monitor for debugging
    Serial.println("Sending data to server...");
    Serial.println("Debug mode: " + String(port));
    Serial.println("Sending POST request: " + request);
    Serial.println(Time.timeStr() + "\n");

    //Check to see if debug port is being used, if not use port 80
    if (debugging)
        Particle.publish("send_owl_data_debug", request, 60, PRIVATE);
    else
        Particle.publish("send_owl_data", request, 60, PRIVATE);
}
/******************************************************************************************************************/

/******************************************************************************************************************
 * Function Name  : lci1ExposedToWater
 * Description    : Verifies if the water sensor connected to D0 is triggered
 * Input          : D0, LOW (activates the lci1 flag)
 * Output         : None
 * Return         : Value of (true or false) in boolean type
******************************************************************************************************************/
boolean lci1ExposedToWater() {
//Conditional statement the verifies is the water sensor is triggered
if(digitalRead(lci1) == LOW)
  return true;  //If water is present on either sensor return the value '1' (true)
    return false; //If water is NOT present return the value '0' (false)
}
/******************************************************************************************************************/

/******************************************************************************************************************
 * Function Name  : lci2ExposedToWater
 * Description    : Verifies if the water sensor connected to D1 is triggered
 * Input          : D1, LOW (activates the lci2 flag)
 * Output         : None
 * Return         : Value of (true or false) in boolean type
******************************************************************************************************************/
boolean lci2ExposedToWater() {
  if(digitalRead(lci2) == LOW)
    return true;
      return false;   //If water is NOT present return the value '0' (false)
}
/******************************************************************************************************************/

/******************************************************************************************************************
 * Function Name  : alarmModule
 * Description    : When triggered, sends the notifications requests, starts the Hue lights alarm, starts buzzer
 * Taken in       : The command string ("start")
 * Input          : None
 * Output         : D2, HIGH (activates the buzzer)
 * Return         : Value of (1 or -1) in INT type
 *                  Returns a negative number on failure
******************************************************************************************************************/
int alarmModule(String command) {
	if(command != "start")
		  return -1;

	//Publish webhook request to Pushover to push notification of leak
	Particle.publish("basement_leak", "Basement leak detected. Verify immediately!", 60, PRIVATE);
	Particle.publish("hue_alarm_start", NULL, 60, PRIVATE); //Start the Hue lights alarm

	//Trigger the alarm by making the pin 'D2' of the core 'HIGH'
	digitalWrite(soundAlarm, HIGH);
	delay(30000);   //Wait 30 seconds
	digitalWrite(soundAlarm, LOW);  //Turn off alarm

	//Stop the Hue lights alarm and reset hue and brightness
	Particle.publish("hue_alarm_stop", NULL, 60, PRIVATE);
	return 1;
}
/******************************************************************************************************************/

/******************************************************************************************************************
 * Function Name  : uptime
 * Description    : When called it will trigger a notification request that will notify me of the uptime lapsed
 *                  since the begining of the program
 * Takes in       : The command string ("getUptime")
 * Input          : None
 * Output         : None
 * Return         : Value of (1 or -1) in INT type
 *                  Returns a negative number on failure
******************************************************************************************************************/
int uptime(String command) {
  if (command != "getUptime")
    return -1;

  //Declare the necessary variables for calculating the uptime in years, days, hours and minutes
  String uptimeData;
  int numberOfMinutes = 0;
  int numberOfHours = 0;
  int numberOfDays = 0;
  int numberOfYears = 0;
  int min = 0;
  int hours = 0;
  int days = 0;
  int years = 0;

  //Get the difference in time since the Photon was powered on
  double result = difftime(Time.now(), startTime);
  //Extract minutes from result
  numberOfMinutes = result / 60;

//Calculate the uptime in minutes, hours, days and years
//Start with getting the minutes and calculate hours, if present
  if (numberOfMinutes > 60) {
    numberOfHours = numberOfMinutes / 60;
    min = numberOfMinutes % 60;
  } else {
    min = numberOfMinutes;
    hours = 0;
  }
    //Get the hours and calculate days, if present
    if (numberOfHours > 24) {
        numberOfDays = numberOfHours / 24;
        hours = numberOfHours % 24;
        } else {
          hours = numberOfHours;
          days = 0;
        }
          //Get the days and calculate years, if present
          if (numberOfDays > 365) {
            numberOfYears = numberOfDays / 365;
            days = numberOfDays % 365;
            } else {
              days = numberOfDays;
              years = 0;
            }

  //Format the uptimeData string by ignoring 0's for hours, days and years
  if (hours == 0 && days == 0 && years == 0)
    uptimeData = String::format("Uptime in (mm): %02dmin", min);
    else if (days == 0 && years == 0)
      uptimeData = String::format("Uptime in (hh:mm): %02d:%02d", hours, min);
        else if (years == 0) {
          if (days < 99)
            uptimeData = String::format("Uptime in (dd:hh:mm): %02d:%02d:%02d", days, hours, min);
              uptimeData = String::format("Uptime in (ddd:hh:mm): %03d:%02d:%02d", days, hours, min);
  } else {
    if(days < 99)
        uptimeData = String::format("Uptime in (yy:dd:hh:mm): %02d:%02d:%02d:%02d", years, days, hours, min);
          uptimeData = String::format("Uptime in (yy:ddd:hh:mm): %02d:%03d:%02d:%02d", years, days, hours, min);
  }

  //Send the dataString to Pushover.net for request of push notification
  Particle.publish("basement_leak", uptimeData, 60, PRIVATE);
  //Print stuff on the serial monitor for debugging
  Serial.println("Uptime Requested...");
  Serial.println(String(uptimeData) + "\n");
  return 1;
}
/******************************************************************************************************************/
