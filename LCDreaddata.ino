#include <Wire.h>
#include <ldc1312_lib.h>

float data;
uint16_t drive;
int wlevel;
float shift; // linear shift value from base data
LDC131X ldc1312(true); // inductive sensor

void setup() {
  // initialize I2C communication
  Wire.begin();
  // initialize serial communication
  Serial.begin(115200);
}

void loop() {
  // reset 
  ldc1312.LDC_resetLDC();
  delay(1000);

  // get device info
  uint16_t mID = ldc1312.LDC_readManufacturerID();
  //Serial.print("Manufacturer ID: 0x");
  //Serial.println(mID,HEX);
  uint16_t dID = ldc1312.LDC_readDeviceID();
  //Serial.print("Device ID: 0x");
  //Serial.println(dID,HEX);

  // sleep mode
  ldc1312.LDC_setConfig(0x2801);

  // configure sensor
  ldc1312.LDC_setMUXConfig(0x020F); // MUX  33MHz deglitch
  ldc1312.LDC_setConversionTime(1,0x04D6); // RCOUNT
  ldc1312.LDC_setSettleTime(1,0x000A); // SETTLECOUNT
  ldc1312.LDC_setGain(3); // GAIN
  ldc1312.LDC_setOffset(1,0x1100); // FREQUENCY OFFSET, internal fref = 43MHz
  ldc1312.LDC_setDriveCurrent(1,0x5000); 
  // get initial first reading (assuming starting full) and adjust for shift from base data
  ldc1312.LDC_setConfig(0x5801);
  ldc1312.LDC_readData(1);
  delay(100);
  data = float(ldc1312.LDC_readData(1));
  ldc1312.LDC_setConfig(0x2801);
  shift = 310.0 - data;
  Serial.println(shift);
  delay(2000);
  //Serial.println("configured");

  while (true) {
    // turn on sensor
    ldc1312.LDC_setConfig(0x5801);
   // Serial.println("on");
    // ignore first sensor readings as they are zero
    ldc1312.LDC_readData(1);
    delay(100);
    // now take reading
   // drive = ldc1312.LDC_getDriveCurrent(0);
   // Serial.print("drive current: ");
   // Serial.println(drive);
   // Serial.print("reading: 0x");
   // Serial.println(data);
    data = float(ldc1312.LDC_readData(1));
    // back to sleep
    ldc1312.LDC_setConfig(0x2801);
    wlevel = int(36+0.00165*sqrt(1.22*pow(10.0,7.0)*data-2.255*pow(10.0,9.0)));
    Serial.println(data);
    delay(100);
  }
}
