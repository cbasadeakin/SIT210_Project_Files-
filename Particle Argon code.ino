// This #include statement was automatically added by the Particle IDE.
#include <MQTT.h>

// Declare variables
int weatherCode;
int rainType;

// Start watering cycle at 6:00 AM
int START_TIME_HOUR = 14;          
int START_TIME_MINUTE = 37;
    
// End watering cycle at 9:00 AM
int FINISH_TIME_HOUR = 14;
int FINISH_TIME_MINUTE = 47;

// Pin definitions
const int MOISTURE_SENSOR_PIN = A1;

// Print messages
bool checkSoilMessage = false;
bool drySoilMessage = false;
bool drySoilMessageAgain = false;
bool optimalSoilMessage = false;
bool wetSoilMessage = false;
bool check = false;
String receivedMessage; // global variable to store the subscribed message

// Moisture sensor reading variables
int analogValueM;
const int DRY_THRESHOLD = 300;
const int WET_THRESHOLD = 700;

int drySoilCounter = 0;

// Control variables
bool isSleepModeEnabled = false;
bool isSleepModeActive = false;
bool isSleepModeActiveNo = false;
bool test = false;

// MQTT client and connection details
MQTT client("broker.emqx.io", 1883, callback);

// This function is called when a message is received on the subscribed topic
void callback(char* topic, byte* payload, unsigned int length) {
    // Convert payload to string
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    // Store the message in the global variable
    receivedMessage = message;
}

// This function will handle data received back from the webhook
int handleForecastReceived(const char *event, const char *data) {
  // Handle the integration response
  String receivedStr = String(data);
  
  int loc = 0;

  loc = receivedStr.indexOf("weathercode");
  
  weatherCode = (int) String(receivedStr.substring(0,loc)).toFloat();
  
  rainType = weatherCode;
  
  return rainType;
}

bool mqttConnect() {
    client.connect("sparkclient_" + String(Time.now()));
    if (client.isConnected()) {
        client.subscribe("MotorStatus");
        client.subscribe("Confirmation");
        return true;
    }
    return false;
}

void setup() {
    // Connect to the Wi-Fi network
    WiFi.connect();
    // Initialize serial communication at a baud rate of 9600
    Serial.begin(9600);
    // Connect to the MQTT broker with a unique client name based on the current time
    client.connect("sparkclient_" + String(Time.now()));
    // Subscribe to the "MotorStatus" topic on the MQTT broker
    client.subscribe("MotorStatus");
    client.subscribe("Confirmation");
    // Serial.print("Connected");
    Particle.syncTime(); /*sync to current time to the particle cloud*/
    // Subscribe to the integration response event
    Particle.subscribe("hook-response/get-forecast", handleForecastReceived);
    Particle.publish("get-forecast");
}

String getForecast(int rainType) {
    switch (rainType) {
        case 51:
          return "Light drizzle";
        case 53:
          return "Moderate drizzle";
        case 55:
          return "Dense drizzle";
        case 56:
          return "Light freezing drizzle";
        case 57:
          return "Dense freezing drizzle";
        case 61:
          return "Slight rain";
        case 63:
          return "Moderate rain";
        case 65:
          return "Heavy rain";
        case 66:
          return "Light freezing rain";
        case 67:
          return "Heavy freezing rain";
        case 80:
          return "Slight rain showers";
        case 81:
          return "Moderate rain showers";
        case 82:
          return "Violent rain showers";
        case 85:
          return "Slight snow showers";
        case 86:
          return "Heavy snow showers";
        default:
          return "No rain";
      }
}

void loop() {

    // Set to Australia Eastern Standard Time
    Time.zone(+10); 
    
    time_t now = Time.now();
    
    Particle.process();
    
    if (Time.hour(now) == START_TIME_HOUR && Time.minute(now) >= START_TIME_MINUTE && Time.hour(now) == FINISH_TIME_HOUR && Time.minute(now) < FINISH_TIME_MINUTE) {
        
        Serial.println("Checking the daily weather forecast...");
        delay(13000);
        
        String rain_t = getForecast(rainType);
        Serial.println(rain_t + " expected today");
        delay(2000);

        if (rain_t == "No rain") {
            while (Time.hour(now) == FINISH_TIME_HOUR && Time.minute(now) < FINISH_TIME_MINUTE) {
                
                now = Time.now();
                
                analogValueM = analogRead(MOISTURE_SENSOR_PIN);
    
                if (client.isConnected()) {
                    client.loop();
                }
                else {
                    mqttConnect();
                }
                if (checkSoilMessage == false) {
                    // Print to serial monitor and wait for one second
                    Serial.print("Measuring soil moisture level...  ");

                    delay(750);
                    
                    check = true;
                    checkSoilMessage = true;
                }
                // Check if the soil moisture level value is less than or equal to the wet threshold value
                if (analogValueM <= DRY_THRESHOLD) {
                    if (drySoilMessage == false) {
                            receivedMessage = "";
                            Serial.println("The soil is too dry");
                            Particle.publish("dry", "The soil in the pot is too dry and needs watering. Watering will start soon.", PRIVATE);
                            client.connect("sparkclient_" + String(Time.now()));
                            client.publish("Motor", "on");
                            drySoilMessage = true;
                            drySoilCounter = drySoilCounter + 1;
                    }
                }
                // Check if the dry soil counter value is greater than one
                if (drySoilCounter > 1) {
                    if (drySoilMessageAgain == false) {
                        Serial.println("It seems the soil is still dry");
                        Serial.println("The sensor may have detached from the soil or may be faulty");
                        Serial.print("Would you like to re-water anyway? ");
                        Serial.println("Awaiting user input...");
                        client.connect("sparkclient_" + String(Time.now()));
                        client.publish("Motor", "off");
                        client.publish("Gui", "show");
                        drySoilMessageAgain = true;
                    }
                }
                // Print to serial monitor "Confirmation yes received" if the received message is "yes"
                if (receivedMessage == "yes") {
                    Serial.println("Confirmation " + receivedMessage + " received");
                    receivedMessage = "";
                    drySoilCounter = 0;
                    drySoilMessage = false;
                }
               // Print to serial monitor "Confirmation no received" if the received message is "no"
                if (receivedMessage == "no") {
                    Serial.println("Confirmation " + receivedMessage + " received");
                    drySoilCounter = 0;
                    isSleepModeEnabled = true;
                }
                // Check if the soil moisture level value is greater than or equal to the dry 
                // threshold value and less than the wet threshold value
                if (analogValueM >= DRY_THRESHOLD && analogValueM < WET_THRESHOLD) {
                    if (optimalSoilMessage == false) {
                        Serial.println("The soil is optimal");
                        Particle.publish("optimal", "The soil in the pot is optimal and doesn't require watering.", PRIVATE);
                        client.connect("sparkclient_" + String(Time.now()));
                        client.publish("Motor", "off");
                        delay(1000);
                        optimalSoilMessage = true;
                        isSleepModeEnabled = true;
                    }
                }
                // Check if the soil moisture level value is greater than or equal to the wet threshold value
                if (analogValueM >= WET_THRESHOLD) {
                    if (wetSoilMessage == false) {
                        Serial.println("The soil is too wet");
                        Particle.publish("wet", "The soil in the pot is too wet. Watering will resume once the soil has dried out a bit.", PRIVATE);
                        Serial.println("Watering will resume once the soil has dried out a bit");
                        client.connect("sparkclient_" + String(Time.now()));
                        client.publish("Motor", "off");
                        delay(1000);
                        wetSoilMessage = true;
                        isSleepModeEnabled = true;
                    }
                }
                // Check if sleep mode is enabled and if the received message is "stopped" or "no"
                if (isSleepModeEnabled && receivedMessage == "stopped" || receivedMessage == "no") {
                    if (receivedMessage != "no") {
                        Serial.println("Watering " + receivedMessage);
                    }
                    // Serial.println("Watering " + receivedMessage);
                    Serial.println("Entering sleep mode to save power...");
                    client.connect("sparkclient_" + String(Time.now()));
                    client.publish("Motor", "off");
                    isSleepModeEnabled = false;
                    check = false;
                    checkSoilMessage = false;
                    drySoilMessage = false;
                    drySoilMessageAgain = false;
                    optimalSoilMessage = false;
                    wetSoilMessage = false;
                    client.disconnect();
                    delay(2000);
                      SystemSleepConfiguration config;
                      config.mode(SystemSleepMode::STOP)
                          .duration(1min);  // Sleep for fifteen minutes
                      SystemSleepResult res = System.sleep(config);
                }
                // Print to serial monitor "Watering started" if the received message is "started"
                if (receivedMessage == "started") {
                    Serial.println("Watering " + receivedMessage);
                    receivedMessage = "";
                }
                // Print to serial monitor "Watering stopped" if received message is "stopped"
                if (receivedMessage == "stopped") {
                    Serial.println("Watering " + receivedMessage);
                    receivedMessage = "";
                }
                // Exit loop if current time has reached or exceeded the finish time
                if (Time.hour(now) == FINISH_TIME_HOUR && Time.minute(now) > FINISH_TIME_MINUTE) {
                    break;
                }
            }
        }
        // Skip the watering cycle if rain is expected for the day
        else {
            Serial.println("The system will skip the scheduled watering cycle today");
            delay(2000);
            isSleepModeActive = true;
        }
    }
    // Print to serial monitor if current time is outside the watering cycle
    else {
        if (check == false) {
            Serial.println("The next watering cycle will start at 6:00am (AEST)");
            check = true;
        }
    }
    // Check if sleep mode is enabled
    if (isSleepModeActive) {
        Serial.println("Entering sleep mode to save power...");
        receivedMessage = "";
        isSleepModeActive = false;
        check = false;
        client.disconnect();
        delay(2000);
          SystemSleepConfiguration config;
          config.mode(SystemSleepMode::STOP)
              .duration(1min); // Sleep for twenty-four hours
          SystemSleepResult res = System.sleep(config);
    }
}
