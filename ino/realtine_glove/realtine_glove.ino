#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include "model.h" //Replace this with your model file

Adafruit_MPU6050 mpu1; // First MPU6050 sensor at address 0x68
Adafruit_MPU6050 mpu2; // Second MPU6050 sensor at address 0x69
#define NUM_SAMPLES 100 //Number of smaples to be read, during motion detection
#define NUM_AXES 23 //acc 3 axis + gyro 3 axis

float features[NUM_AXES * NUM_SAMPLES]; 
const int flexPin1 = 34;  // D19 on ESP32
const int flexPin2 = 35;  // Change to your second flex sensor pin
const int flexPin3 = 32;  // Change to your third flex sensor pin

const float alphaMPU = 0.2; // Weight for the exponential moving average of MPU6050 data
const float alphaFlex = 0.2; // Weight for the exponential moving average of flex sensor data
const int meanFilterSize = 5; // Size of the mean filter

int filteredAngle1 = 0;
int filteredAngle2 = 0;

int flexValues1[meanFilterSize] = {0};
int flexValues2[meanFilterSize] = {0};
int flexValues3[meanFilterSize] = {0};
int flexIndex1 = 0;
int flexIndex2 = 0;
int flexIndex3 = 0;
//==================
Eloquent::ML::Port::RandomForest classifier; //This class is inside the model.h file
// Calibration values for each flex sensor
const int minStraight1 = 3190;
const int maxBent1 = 3260; // Adjust based on your sensor behavior

const int minStraight2 = 2825;
const int maxBent2 = 2955; // Adjust based on your sensor behavior

const int minStraight3 = 3155;
const int maxBent3 = 3215; // Adjust based on your sensor behavior



void setup(void) {
  Serial.begin(115200);
  while (!Serial)
    delay(10);

  Serial.println("Adafruit MPU6050 and Flex Sensor test!");

  // Try to initialize the first MPU6050 sensor
  if (!mpu1.begin(0x68)) {
    Serial.println("Failed to find MPU6050 chip 1");
    while (1) {
      delay(10);
    }
  }

  // Try to initialize the second MPU6050 sensor
  if (!mpu2.begin(0x69)) {
    Serial.println("Failed to find MPU6050 chip 2");
    while (1) {
      delay(10);
    }
  }

  mpu1.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu1.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu1.setFilterBandwidth(MPU6050_BAND_21_HZ);

  mpu2.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu2.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu2.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("");

  // Initialize your classifier with your model file
}

void updateMeanFilter(int* values, int& index, int newValue) {
  values[index] = newValue;
  index = (index + 1) % meanFilterSize;
}

int getMeanValue(const int* values) {
  int sum = 0;
  for (int i = 0; i < meanFilterSize; ++i) {
    sum += values[i];
  }
  return sum / meanFilterSize;
}

void loop() {
  /* Get new sensor events with the readings from the first MPU6050 sensor */
  sensors_event_t a1, g1, temp1;
  mpu1.getEvent(&a1, &g1, &temp1);

  // Map the Y-axis accelerometer reading to a 0 to 90-degree range for the first sensor
  int angle1 = map(a1.acceleration.y, -8, 8, 0, 90);

  // Update filter with new data for the first sensor
  filteredAngle1 = alphaMPU * angle1 + (1 - alphaMPU) * filteredAngle1;

  /* Get new sensor events with the readings from the second MPU6050 sensor */
  sensors_event_t a2, g2, temp2;
  mpu2.getEvent(&a2, &g2, &temp2);

  // Map the Y-axis accelerometer reading to a 0 to 90-degree range for the second sensor
  int angle2 = map(a2.acceleration.y, -8, 8, 0, 90);

  // Update filter with new data for the second sensor
  filteredAngle2 = alphaMPU * angle2 + (1 - alphaMPU) * filteredAngle2;

  /* Read data from the flex sensors */
  int flexValue1 = analogRead(flexPin1);
  int flexValue2 = analogRead(flexPin2);
  int flexValue3 = analogRead(flexPin3);

  // Apply mean filter for each flex sensor
  updateMeanFilter(flexValues1, flexIndex1, flexValue1);
  updateMeanFilter(flexValues2, flexIndex2, flexValue2);
  updateMeanFilter(flexValues3, flexIndex3, flexValue3);

  // Get mean values from the mean filter for each flex sensor
  int meanFlexValue1 = getMeanValue(flexValues1);
  int meanFlexValue2 = getMeanValue(flexValues2);
  int meanFlexValue3 = getMeanValue(flexValues3);

  // Map the mean flex sensor values to angles
  int angleFlex1 = map(meanFlexValue1, minStraight1, maxBent1, 0, 90);
  int angleFlex2 = map(meanFlexValue2, minStraight2, maxBent2, 0, 90);
  int angleFlex3 = map(meanFlexValue3, minStraight3, maxBent3, 0, 90);

  // Calculate pitch, yaw, and roll angles for the first sensor
  float pitch1 = atan2(-a1.acceleration.x, sqrt(a1.acceleration.y * a1.acceleration.y + a1.acceleration.z * a1.acceleration.z)) * 180.0 / PI;
  float roll1 = atan2(a1.acceleration.y, a1.acceleration.z) * 180.0 / PI;
  float yaw1 = atan2(a1.acceleration.z, a1.acceleration.x) * 180.0 / PI;

  // Calculate pitch, yaw, and roll angles for the second sensor
  float pitch2 = atan2(-a2.acceleration.x, sqrt(a2.acceleration.y * a2.acceleration.y + a2.acceleration.z * a2.acceleration.z)) * 180.0 / PI;
  float roll2 = atan2(a2.acceleration.y, a2.acceleration.z) * 180.0 / PI;
  float yaw2 = atan2(a2.acceleration.z, a2.acceleration.x) * 180.0 / PI;

 
  // Move to the next line for the next set of data
    int index = 0; 
        features[index] =static_cast<float>(filteredAngle1) ;
        features[index + 1] = static_cast<float>(filteredAngle2);
        features[index + 2] =static_cast<float> (angleFlex1);
        features[index + 3] = static_cast<float>(angleFlex2);
        features[index + 4] = static_cast<float>(angleFlex3);
        features[index + 5] = static_cast<float>(pitch1);
        features[index + 6] = static_cast<float>(roll1);
        features[index + 7] = static_cast<float>(yaw1);
        features[index + 8] = static_cast<float>(pitch2);
        features[index + 9] = static_cast<float>(roll2);
        features[index + 10] = static_cast<float>(yaw2);
        features[index + 11] = static_cast<float>(a1.acceleration.x);
        features[index + 12] = static_cast<float>(a1.acceleration.y);
        features[index + 13] = static_cast<float>(a1.acceleration.z);
        features[index + 14] = static_cast<float>(g1.gyro.x);
        features[index + 15] = static_cast<float>(g1.gyro.y);
        features[index + 16] = static_cast<float>(g1.gyro.z);
        features[index + 17] = static_cast<float>(a2.acceleration.x);
        features[index + 18] = static_cast<float>(a2.acceleration.y);
        features[index + 19] = static_cast<float>(a2.acceleration.z);
        features[index + 20] = static_cast<float>(g2.gyro.x);
        features[index + 21] = static_cast<float>(g2.gyro.y);
        features[index + 22] = static_cast<float>(g2.gyro.z);
String gesture = classifier.predictLabel(features);
    
    // Process the predicted gesture (e.g., control relays)
    if (gesture == "a") {
        Serial.println("A");

    } else if (gesture == "b") {
        Serial.println("B");
    }
    else if (gesture == "c") {
        Serial.println("C");
    }
    else if (gesture == "d") {
        Serial.println("D");
    }
    else if (gesture == "e") {
        Serial.println("E");
    }
    else if (gesture == "f") {
        Serial.println("F");
    }
    else if (gesture == "g") {
        Serial.println("G");
    }
    else if (gesture == "h") {
        Serial.println("H");
    }else if (gesture == "i") {
        Serial.println("I");
    }
    else if (gesture == "j") {
        Serial.println("J");
    }else if (gesture == "k") {
        Serial.println("K");
    }else if (gesture == "l") {
        Serial.println("L");
    }else if (gesture == "m") {
        Serial.println("M");
    }else if (gesture == "n") {
        Serial.println("N");
    }else if (gesture == "o") {
        Serial.println("O");
    }else if (gesture == "p") {
        Serial.println("P");
    }else if (gesture == "q") {
        Serial.println("Q");
    }else if (gesture == "r") {
        Serial.println("R");
    }else if (gesture == "s") {
        Serial.println("S");
    }else if (gesture == "t") {
        Serial.println("T");
    }else if (gesture == "u") {
        Serial.println("U");
    }else if (gesture == "v") {
        Serial.println("V");
    }else if (gesture == "w") {
        Serial.println("W");
    }else if (gesture == "x") {
        Serial.println("X");
    }else if (gesture == "y") {
        Serial.println("Y");
    }else if (gesture == "z") {
        Serial.println("Z");
    }else if (gesture == "relax") {
        Serial.println("RELAX");
    }
    
 delay(20);
}
