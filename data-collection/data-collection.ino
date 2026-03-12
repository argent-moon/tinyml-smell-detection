/*
Smell Detection: Data Collection
*/

#include <Multichannel_Gas_GMXXX.h>
#include <Wire.h>

#ifdef SOFTWAREWIRE
    #include <SoftwareWire.h>
    SoftwareWire myWire(3, 2);
    GAS_GMXXX<SoftwareWire> gas;
#else
    #include <Wire.h>
    GAS_GMXXX<TwoWire> gas;
#endif

void setup() {
  Serial.begin(115200);
  Wire.begin();
  gas.begin(Wire, 0x08); // I2C address
  
  // Wait for sensor to warm up
  //delay(120000);
  
  Serial.println("NO2,C2H5OH,CO,VOC"); // CSV header
}

void loop() {
  float no2 = gas.measure_NO2();
  float c2h5oh = gas.measure_C2H5OH();
  float co = gas.measure_CO();
  float voc = gas.measure_VOC();
  
  // Print in CSV format
  Serial.print(no2); Serial.print(",");
  Serial.print(c2h5oh); Serial.print(",");
  Serial.print(co); Serial.print(",");
  Serial.println(voc);
  
  delay(100); // 10 samples per second
}
