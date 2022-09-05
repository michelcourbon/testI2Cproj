#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include "Adafruit_MCP9600.h"
#include <ArduinoJson.h>
#include <EasyButton.h>

#define I2C_ADDRESS (0x67)

Adafruit_MCP9600 mcp;
// to configure acquisition..
DynamicJsonDocument config(1024);

// comment output
DynamicJsonDocument comment(2048);

// sensor output
DynamicJsonDocument sensors(1024);
double hotJunction;
double coldJunction;
String strSensors;
int dt;
unsigned long myTime;

// use flash button on the nodemcu to stop acqui
#define BUTTON_PIN 0
EasyButton button(BUTTON_PIN);
boolean etat;

// specific to MCP9600 TK sensor
void initializeMCP()
{
  String commentStr = "";
  /* Initialise the driver with I2C_ADDRESS and the default I2C bus. */
  if (! mcp.begin(I2C_ADDRESS)) {
    comment["module"] = "Sensor not found. Check wiring!";
    serializeJson(comment,commentStr);
    Serial.println(commentStr);
    while (1);
  }

  comment["module"] = "Found MCP9600!";
  serializeJson(comment,commentStr);
  Serial.println(commentStr);

  mcp.setADCresolution(MCP9600_ADCRESOLUTION_16);
  /*
  Serial.print("ADC resolution set to ");
  switch (mcp.getADCresolution()) {
    case MCP9600_ADCRESOLUTION_18:   Serial.print("18"); break;
    case MCP9600_ADCRESOLUTION_16:   Serial.print("16"); break;
    case MCP9600_ADCRESOLUTION_14:   Serial.print("14"); break;
    case MCP9600_ADCRESOLUTION_12:   Serial.print("12"); break;
  }
  Serial.println(" bits");
  */

  mcp.setThermocoupleType(MCP9600_TYPE_K);
  /*
  Serial.print("Thermocouple type set to ");
  switch (mcp.getThermocoupleType()) {
    case MCP9600_TYPE_K:  Serial.print("K"); break;
    case MCP9600_TYPE_J:  Serial.print("J"); break;
    case MCP9600_TYPE_T:  Serial.print("T"); break;
    case MCP9600_TYPE_N:  Serial.print("N"); break;
    case MCP9600_TYPE_S:  Serial.print("S"); break;
    case MCP9600_TYPE_E:  Serial.print("E"); break;
    case MCP9600_TYPE_B:  Serial.print("B"); break;
    case MCP9600_TYPE_R:  Serial.print("R"); break;
  }
  Serial.println(" type");
  */

  mcp.setFilterCoefficient(3);
  // Serial.print("Filter coefficient value set to: ");
  // Serial.println(mcp.getFilterCoefficient());

  /*
  mcp.setAlertTemperature(1, 30);
  Serial.print("Alert #1 temperature set to ");
  Serial.println(mcp.getAlertTemperature(1));
  mcp.configureAlert(1, true, true);  // alert 1 enabled, rising temp
  */

  mcp.enable(true);
}

// attend le JSon de la config pour commencer !!
void readConfig() 
{
  String configStr=""; 
  while(!Serial.available()) {}
  configStr = Serial.readStringUntil('\n');
  deserializeJson(config, configStr);
  if (config.containsKey("dt")){
    dt = config["dt"];
    if ( dt<210) dt = 210;
    etat = true;
  } else {
    etat = false;
  }
}

// wait  & read onfig on serial input ! & configure the MCP chip
void startProcess() {
  // Serial.println(" waiting config");
  while (!etat) {
    readConfig();
  }
  // Serial.println("MCP9600 HW test");
  initializeMCP();
}

// feature : hardware pressed on the flash button ESP8266 stop the acquisition & return at setup

// Callback.
void onPressed() {
  String commentStr="";
  comment["module"] = "Button has been pressed by user";
  serializeJson(comment,commentStr);
  Serial.println(commentStr);
  startProcess();
}

// arduino setup for ROS
void setup()
{
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
  // Initialize the button.
  button.begin();
  // Attach the callback to button evt
  button.onPressed(onPressed);  
  // start the acquisition
  startProcess();
}


// arduino infinite loop : the acquisition process after setup
// -----------------------
void loop()
{
  // press on flash button to stop acquisition
  button.read();
  // dans la boucle infini, l'arrivée d'une chaine JSON renvoi vers la lecture de la configuration
  if (Serial.available() > 0) {
    etat = false;
    startProcess();    
  }
  
  // normal acquisition
  hotJunction = mcp.readThermocouple();
  coldJunction = mcp.readAmbient();
  myTime = millis();
  sensors["hotJct"] = hotJunction;
  sensors["coldJct"] = coldJunction;
  sensors["time"] = myTime;
  strSensors="";
  serializeJson(sensors, strSensors);
  Serial.println(strSensors);
  delay(dt);
}