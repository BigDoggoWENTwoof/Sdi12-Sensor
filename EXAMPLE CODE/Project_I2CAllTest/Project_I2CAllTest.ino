/*Test code for all I2C devices
   Adresses for devices are as follows:
   0x23 - BH1750
   0x28 - Arduino board (ZERO ONLY)
   0x50 - RTC Memory
   0x68 - RTC
   0x69 - IMU (NEEDS TO BE UPDATED IN THE BMX160 HEADER[.h] FILE)
   0x76 - BME680 (NEED TO CHECK HEADER [.h] ADDRESS IS CORRECT)
*/

//Common library - needed for I2C communications
#include <Wire.h>

//Component Specific setups
//RTC
#include "RTClib.h"
//#include <DateTime.h>
RTC_DS1307 rtc;
//char DoTW[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//IMU
#include <DFRobot_BMX160.h>
DFRobot_BMX160 IMU;

//BH
#include <hp_BH1750.h>
hp_BH1750 DLDR;

//BME
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"
Adafruit_BME680 AQS;
#define SEALEVELPRESSURE_HPA (1013.25)

int opt = 0;
char input;

void setup() {
  //Serial setup
  Serial.begin(9600);
  Serial.flush();
  if (!Serial) {
    Serial.println("Serial failed to initialise");
  } else {
    Serial.println("Serial connected");
    Serial.println();
  }

  Wire.begin();

  //RTC Setup
  rtc.begin();
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  if (rtc.begin() == false) {
    Serial.println("RTC NOT CONNECTED");
  }

  //IMU Setup
  IMU.begin();
  if (IMU.begin() == false) {
    Serial.println("IMU NOT CONNECTED");
  }

  //DLDR Setup
  DLDR.begin(BH1750_TO_GROUND);
  if (DLDR.begin(BH1750_TO_GROUND) == false) {
    Serial.println("DLDR NOT CONNECTED");
  }

  //AQS Setup
  AQS.begin(0x76);
  if (AQS.begin(0x76) == false) {
    Serial.println("AQS NOT CONNECTED");
  }
  AQS.setTemperatureOversampling(BME680_OS_8X);
  AQS.setHumidityOversampling(BME680_OS_2X);
  AQS.setPressureOversampling(BME680_OS_4X);
  AQS.setIIRFilterSize(BME680_FILTER_SIZE_3);
  AQS.setGasHeater(320, 150);  // 320*C for 150 ms

  //I2C Scanner to display conected components
  byte error, address;
  int nDevices = 0;

  Serial.println("Scanning for I2C components");
  Serial.println();
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) {
        Serial.print("0");
      }
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0) {
    Serial.println("No I2C components connected");
  } else {
    Serial.println();
    Serial.println("All components found");
  }
  Serial.println("Enter Y to continue to testing");
  do {
    input = Serial.read();
    //Serial.println("waiting...");
  } while (input != 'y');
}

void loop() {
  Serial.println();
  Serial.println("Choose the component to test:\n  1 - RTC\n  2 - IMU\n  3 - BH1750\n  4 - BME680\n");
  do {
    opt = Serial.parseInt();
  } while (Serial.available() <= 0);

  switch (opt) {
    case 1:
      {
        Serial.println("----------");
        Serial.println("Do thing 1 - RTC\n");

        if (rtc.begin() == false) {
          Serial.println("No RTC found, cannot complete reading");
          break;
        }
        DateTime now = rtc.now();

        Serial.print(now.hour(), DEC);
        Serial.print(":");
        Serial.print(now.minute(), DEC);
        Serial.print(":");
        Serial.println(now.second(), DEC);
        Serial.println("----------");

        Serial.print(now.day(), DEC);
        Serial.print("/");
        Serial.print(now.month(), DEC);
        Serial.print("/");
        Serial.println(now.year(), DEC);
        Serial.println("----------");
      }
      break;

    case 2:
      {
        Serial.println("----------");
        Serial.println("Do thing 2 - IMU\n");
        if (IMU.begin() == false) {
          Serial.println("No IMU found, cannot complete reading");
          break;
        }

        sBmx160SensorData_t Omagn, Ogyro, Oaccel;
        IMU.getAllData(&Omagn, &Ogyro, &Oaccel);

        Serial.println("Displying Magnetometer in uTesla");
        Serial.print("X: ");
        Serial.print(Omagn.x);
        Serial.print(" Y: ");
        Serial.print(Omagn.y);
        Serial.print(" Z: ");
        Serial.print(Omagn.z);
        Serial.println(" uT");
        Serial.println("----------");

        Serial.println("Displying Gyroscope in g");
        Serial.print("X: ");
        Serial.print(Ogyro.x);
        Serial.print(" Y: ");
        Serial.print(Ogyro.y);
        Serial.print(" Z: ");
        Serial.print(Ogyro.z);
        Serial.println(" g");
        Serial.println("----------");

        Serial.println("Displying Accelerometer in m/s^2");
        Serial.print("X: ");
        Serial.print(Oaccel.x);
        Serial.print(" Y: ");
        Serial.print(Oaccel.y);
        Serial.print(" Z: ");
        Serial.print(Oaccel.z);
        Serial.println(" m/s^2");
        Serial.println("----------");
      }
      break;

    case 3:
      {
        Serial.println("----------");
        Serial.println("Do thing 3 - DLDR\n");
        if (DLDR.begin(BH1750_TO_GROUND) == false) {
          Serial.println("No DLDR found, cannot complete reading");
          break;
        }

        DLDR.start();
        float lux = DLDR.getLux();
        Serial.print("Light level: ");
        Serial.print(lux);
        Serial.println(" lx");
        Serial.println("----------");
      }
      break;

    case 4:
      {
        Serial.println("----------");
        Serial.println("Do thing 4 - AQS\n");
        if (AQS.begin() == false) {
          Serial.println("No AQS found, cannot complete reading");
          break;
        }
        if (AQS.performReading() == true) {

          Serial.print("Temperature = ");
          Serial.print(AQS.temperature);
          Serial.println(" *C");

          Serial.print("Pressure = ");
          Serial.print(AQS.pressure / 100.0);
          Serial.println(" hPa");

          Serial.print("Humidity = ");
          Serial.print(AQS.humidity);
          Serial.println(" %");

          Serial.print("Gas = ");
          Serial.print(AQS.gas_resistance / 1000.0);
          Serial.println(" KOhms");

          Serial.print("Approx. Altitude = ");
          Serial.print(AQS.readAltitude(SEALEVELPRESSURE_HPA));
          Serial.println(" m");
          Serial.println("----------");
        } else if (AQS.performReading() == false) {
          Serial.println("Reading failed to run");
        }
      }
      break;

    default:
      {
        Serial.print(opt);
        Serial.println(" was not an option, try again");
      }
  }
  delay(1000);  //time to read
}
