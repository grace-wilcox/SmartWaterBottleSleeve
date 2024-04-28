#include <Wire.h>
#include <ldc1312_lib.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>

#define CLICKTHRESHHOLD 80 // click threshold for accelerometer
double levelx = 0; // accel values for level position
double levely = 0;
double levelz = 0;
double range = 1; // level position margin of error
int levelreadingT = 200; // time between consecutive level checks in ms
int levelpass = 10; // number of level checks to pass -> total level time min is levelpass * levelreadingT in ms
float ldcrange = 5; // LDC reading margin of error
float data; // LDC raw data
int oldwlevel; // previous water level
int wlevel = 100; // water level, start out full
int volume = 0; // volume consumed

LDC131X ldc1312(true); // inductive sensor
Adafruit_LIS3DH lis = Adafruit_LIS3DH(); // accelerometer

void setup() {
  // initialize I2C communication
  Wire.begin();
  // initialize serial communication
  Serial.begin(115200);
  if (! lis.begin(0x18)) { 
    Serial.println("Couldnt start");
    while (1) yield();
  }
  delay(1000);
  // configure accelerometer
  configureLIS();
  // configure LDC
  configureLDC();
}


void loop() {
  // look for movement
  uint8_t click = lis.getClick();
  if (click == 0) return;
  if (! (click & 0x30)) return;
  Serial.println("movement!");
  // movement found ! check if level
  int n = 0; // number of level passes in a row
  while (n < levelpass) {
    if (checkLevel()) {
      n=n+1; 
      Serial.print("level count: ");
      Serial.println(n);
    }
    else n=0;
    delay(levelreadingT);
  }
  // take reading now that bottle is level
  readLDC();
  Serial.print("raw data: ");
  Serial.println(data);
  // calculate new water level position
  waterlevel();
  Serial.print("water level reading: ");
  Serial.println(wlevel);
  // check if water level has changed
  if (wlevel == oldwlevel) return;
  // if water level has risen, assume refill
  if (wlevel > oldwlevel) {
    refill();
  }
  updatevolume();
  Serial.print("total volume consumed: ");
  Serial.println(volume);
  delay(1000);
}

// check if accelerometer is level
bool checkLevel() {
  sensors_event_t event;
  lis.getEvent(&event);
  if ((event.acceleration.x > levelx-range) && (event.acceleration.x < levelx+range)) {
    if ((event.acceleration.y > levely-range) && (event.acceleration.y < levely+range)) {
      if ((event.acceleration.z > levelz-range) && (event.acceleration.z < levelz+range)) {
        return true;
      }
    }
  }
  return false;
}

// configure accelerometer, do this in neutral/level position
void configureLIS() {
  // set level position
  sensors_event_t event;
  lis.getEvent(&event);
  levelx = event.acceleration.x;
  levely = event.acceleration.y;
  levelz = event.acceleration.z;
  // set up click for accelerometer interrupt
  lis.setClick(1, CLICKTHRESHHOLD);
}

// configure LDC
void configureLDC() {
  // reset 
  ldc1312.LDC_resetLDC();
  delay(500);
  // sleep mode
  ldc1312.LDC_setConfig(0x2801);
  // configure sensor -- just played around with values until I got decent resolution and readings, will depend on coil
  ldc1312.LDC_setMUXConfig(0x020F); // MUX (33MHz deglitch)
  ldc1312.LDC_setConversionTime(1,0x04D6); // RCOUNT
  ldc1312.LDC_setSettleTime(1,0x000A); // SETTLECOUNT
  ldc1312.LDC_setGain(3); // GAIN
  ldc1312.LDC_setOffset(1,0x1100); // FREQUENCY OFFSET, internal fref = 43MHz
  ldc1312.LDC_setDriveCurrent(1,0x5000); // DRIVE CURRENT
}

// take reading from LDC
void readLDC() {
  // turn on sensor
  ldc1312.LDC_setConfig(0x5801);
  Serial.println("on");
  // ignore first sensor readings as they are zero
  ldc1312.LDC_readData(1);
  delay(100);
  // now take reading
  data = float(ldc1312.LDC_readData(1));
  // back to sleep
  ldc1312.LDC_setConfig(0x2801);
  Serial.println("reading taken");
}

// converts inductance data to water level in percentage of bottle height (100% = full, top of bottle)
// calibrated for this jar, this coil, and this target using quadratic regression in Excel
void waterlevel() {
  // save old level for computing change in volume
  oldwlevel = wlevel;
  wlevel = roundvolume(36+0.00165*sqrt(1.22*pow(10.0,7.0)*data-2.255*pow(10.0,9.0)));
}

// adds difference in volume between previous and current measurement to volume total
void updatevolume() {
  volume = volume + (oldwlevel-wlevel);
}

// when current reading > previous reading
// adds remaining volume from previous reading and then resets previous to full
void refill() {
  volume = volume + oldwlevel;
  oldwlevel = 100;
}

// rounds to nearest 10%, bottom half out of sensor's reach
// would be ideal with two coils that can cover the two halves of the bottle height
int roundvolume(float n) {
  int rounded;
  if (n < 56) rounded = 50;
  else if (n < 66) rounded = 60;
  else if (n < 76) rounded = 70;
  else if (n < 86) rounded = 80;
  else if (n < 96) rounded = 90;
  else rounded = 100;
  return rounded;
}
