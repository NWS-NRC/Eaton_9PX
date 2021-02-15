#include <Adafruit_SleepyDog.h>

#define SYNC  0x16
#define ACK   0x06
#define NACK  0x15

const int led_pin = 13;
unsigned long ledTimer = 0UL;

unsigned long statSync = 0UL;
unsigned long lastUpdate = 0UL;
byte boot = 0;
byte updateInfo = 0;

/*********************From Report 001*************************/
byte acPresent = 0;
byte charging = 0;
byte configFail = 0;
byte discharging = 0;
byte goodStat = 0;

/*********************From Report 002*************************/
byte internalFail = 0;
byte needReplaced = 0;
byte overload = 0;
byte shutdownImminent = 0;

/*********************From Report 003*************************/
byte fanFail = 0;
byte overTemp = 0;

/*********************From Report 006*************************/
byte capacity = 0;
uint16_t runTime = 0;

/*********************From Report 0049*************************/
float inputFreq = 0.00;
float inputVoltage = 0.00;

/*********************From Report 0066*************************/
int current = 0;
int efficiency = 0;
int outFreq = 0;
float outVoltage = 0.0;

/*********************From Report 007*************************/
int loadPercent = 0;
int battT = 0;
float battV = 0.0;

byte syncCheck = 0;

byte onBatt = 0;
byte onBatt_AlertSent = 0;
byte lowBatt = 0;
byte lowBatt_AlertSent = 0;
byte deadBatt = 0;
byte deadBatt_AlertSent = 0;
byte sysError = 0;
byte sysError_AlertSent = 0;
byte fanError = 0;
byte overTemp_AlertSent = 0;
byte outputDisable_AlertSent = 0;

const byte statusChk[] = {0x81, 0x88, 0xa1, 0x01, 0x01, 0x03, 0x00, 0x00, 0x00, 0x01, 0xa3}; //Report F001
const byte errorChk[] = {0x81, 0x88, 0xa1, 0x01, 0x02, 0x03, 0x00, 0x00, 0x00, 0x01, 0xa0}; //Report F002
const byte fan_tempChk[] = {0x81, 0x88, 0xa1, 0x01, 0x03, 0x03, 0x00, 0x00, 0x00, 0x01, 0xa1}; //Report F003
const byte cap_Runtime[] = {0x81, 0x88, 0xa1, 0x01, 0x06, 0x03, 0x00, 0x00, 0x00, 0x01, 0xa4}; //Report F006
const byte inputCheck[] = {0x81, 0x88, 0xa1, 0x01, 0x31, 0x03, 0x00, 0x00, 0x00, 0x01, 0x93}; //Report F049
const byte outputCheck[] = {0x81, 0x88, 0xa1, 0x01, 0x42, 0x03, 0x00, 0x00, 0x00, 0x01, 0xe0}; //Report F066
const byte battStats[] = {0x81, 0x88, 0xa1, 0x01, 0x07, 0x03, 0x00, 0x00, 0x00, 0x01, 0xa5}; //Report F007

//------------------------------------Response Data-----------------------------------//
byte initialResponse = 0;
int inV_Response = 0;
int outV_Response = 0;
int battV_Response = 0;
const char prompt_Response[] = "\r\n>\r\n";
const char initial_Response[] = "\r\n??\r\n>";
const char sysStat_Response[] = "\r\n\r\n*** SYSTEM STATUS ***\r\n\r\n";
const char outEnabled_Response[] = "OUTPUT ENABLED";
const char outDisabled_Response[] = "OUTPUT DISABLED";
const char onAC_Response[] = "ON AC LINE";
const char onBattery_Response[] = "ON BATTERY";
const char inputV_Response[] = "INPUT VOLTAGE = ";
const char outputV_Response[] = "OUTPUT VOLTAGE = ";
const char batteryV_Response[] = "BATTERY VOLTAGE = ";
const char final_Response[] = "\r\n>\r\n*********************\r\n\n>";


void setup() {
  Serial.begin(9600); //USB Comms
  Serial1.begin(2400); //UPS Comms
  Serial3.begin(9600); //ACU Comms
  delay(5000);
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, HIGH); 
  ledTimer = millis();
  while (Serial.available() > 0)
  {
    byte c = Serial.read();
  }
  while (Serial1.available() > 0)
  {
    byte c = Serial1.read();
  }
  Watchdog.enable(10000);
  updateInfo = 1;
  while (Serial3.available() > 0)
  {
    byte c = Serial3.read();
  }
  lastUpdate = millis();
}

void loop() {
  delay(10);
  Watchdog.reset();
  if (Serial3.available() > 0)
  {
    updateInfo = 0;
    char queryBuff[5];
    byte buffPosition = 0;
    while (Serial3.available() > 0)
    {
      char c  = char(Serial3.read());
      queryBuff[buffPosition] = c;     
      Serial.print(F("Request: "));
      Serial.println(c);
      if (c == '\r' && buffPosition == 0)
      {
        Serial3.print(initial_Response);
        Serial.println(F("Sending Init"));
        buffPosition = 0;
      }
      else if (c == '\r' && buffPosition != 0)
      {
        sendResponse();
        Serial.println(F("Sent Response"));
        updateInfo = 1;
        buffPosition = 0;
      }
      else
      {
        buffPosition++;
      }
      delay(600);
    }
  }
  if (syncCheck == SYNC)
  {
    if (updateInfo == 1)
    {
      syncUPS();
      Serial.println(F("*************************************************"));
      Serial.println();
      getData();
      Serial.print(F("UPS On-Battery Alarm: "));
      if (onBatt == 1)
      {
        Serial.println(F("True"));
      }
      else
      {
        Serial.println(F("False"));
      }
      Serial.print(F("UPS Low-Battery Alarm: "));
      if (lowBatt == 1)
      {
        Serial.println(F("True"));
      }
      else
      {
        Serial.println(F("False"));
      }
      Serial.print(F("UPS Over Temperature Alarm: "));
      if (fanError == 1)
      {
        Serial.println(F("True"));
      }
      else
      {
        Serial.println(F("False"));
      }
      Serial.print(F("UPS System Error Alarm: "));
      if (sysError == 1)
      {
        Serial.println(F("True"));
      }
      else
      {
        Serial.println(F("False"));
      }
      if (!boot)
      {
        boot = 1;
      }
      updateInfo = 0;
      lastUpdate = millis();
    }
  }
  else
  {
    syncUPS();
  }
  
  if (ledTimer > millis())
  {
    ledTimer = millis();
  }
  if ((millis() - ledTimer) > 2000UL)
  {
    changeLED();
  }
}

void syncUPS()
{
  //Serial.println(F("Sending Sync"));
  if (Serial1.available() > 0)
  {
    while (Serial1.available() > 0)
    {
      byte c = Serial1.read();
      delay(5);
    }
  }
  Serial1.write(SYNC);
  delay(1000);
  syncCheck = 0;
  unsigned long syncCheckTimer = 0UL;
  syncCheckTimer = millis();
  while (syncCheck != SYNC || (millis() - syncCheckTimer) > 5000UL )
  {
    //Serial.println(F("Reset WDT"));
    delay(10);
    Watchdog.reset();
    syncCheck = Serial1.read();
    if (syncCheck == SYNC)
    {
      Serial.println(syncCheck, HEX);
      Serial.println(F("Sync Received"));
    }
    else
    {
      Serial.println(syncCheck, HEX);
      Serial.println(F("Checking...."));
      Serial1.write(SYNC);
      delay(1000);
    }
  }
}

void sendResponse()
{
  Serial3.print(sysStat_Response);
  Serial3.print(outEnabled_Response);
  Serial3.print(prompt_Response);
  if (onBatt == 0)
  {
    Serial3.print(onAC_Response);
    Serial3.print(prompt_Response);
    Serial3.print(inputV_Response);
    Serial3.print(inV_Response, DEC);
    Serial3.print(F(" VOLTS"));
  }
  else
  {
    Serial3.print(onBattery_Response);
  }
  Serial3.print(prompt_Response);
  Serial3.print(outputV_Response);
  Serial3.print(outV_Response, DEC);
  Serial3.print(F(" VOLTS"));
  Serial3.print(prompt_Response);
  Serial3.print(batteryV_Response);
  Serial3.print(battV_Response, DEC);
  Serial3.print(F(" VOLTS"));
  Serial3.print(final_Response);
}

void getData()
{
  checkStatus();
  checkErrors();
  checkFanTemp();
  checkCapRun();
  checkInputInfo();
  checkOutputInfo();
  checkBattStats();
}

void checkStatus()
{
  //Serial.println(F("Debug 1"));
  byte response[50];
  int responseLength = 0;
  byte acknowledge = 0x00;
  for (int i = 0; i < sizeof(statusChk); i++)
  {
    Serial1.write(statusChk[i]);
  }
  //Serial.println(F("Debug 2"));
  while (Serial1.available() == 0)
  {
    ;;
  }
  //Serial.println(F("Debug 3"));
  if (Serial1.available() > 0)
  {
    while (acknowledge != 0x06)
    {
      acknowledge = Serial1.read();
    }
    delay(200);
    while (Serial1.available() > 0)
    {
      response[responseLength] = Serial1.read();
      responseLength++;
      delay(10);
    }
  }
  acPresent = response[3];
  charging = response[5];
  configFail = response[6];
  discharging = response[7];
  goodStat = response[8];

  Serial1.write(ACK);
  /*for (int i = 0; i < responseLength; i++)
    {
    Serial.print(response[i], HEX);
    Serial.print(F(" "));
    }*/
  Serial.print(F("AC Present: "));
  if (acPresent == 1)
  {
    Serial.println(F("True"));
  }
  else
  {
    Serial.println(F("False"));
    onBatt = 1;
    if (onBatt_AlertSent == 0)
    {
      sendOnBattery_Alert();
    }
  }
  Serial.print(F("Batteries Charging: "));
  if (charging == 1)
  {
    Serial.println(F("True"));
  }
  else
  {
    Serial.println(F("False"));
  }
  if (acPresent == 1 && charging == 1)
  {
    if (onBatt == 1)
    {
      onBatt = 0;
      sendACLineRecovered_Alert();
    }
  }
  Serial.print(F("Configuration Fail: "));
  if (configFail == 1)
  {
    Serial.println(F("True"));
  }
  else
  {
    Serial.println(F("False"));
  }
  Serial.print(F("Batteries Discharging: "));
  if (acPresent == 0 && discharging == 1)
  {
    Serial.println(F("True"));
  }
  else
  {
    Serial.println(F("False"));
  }
  Serial.print(F("Overall Status: "));
  if (goodStat == 1)
  {
    Serial.println(F("Good"));
  }
  else
  {
    Serial.println(F("Fault"));
  }
}

void checkErrors()
{
  byte response[50];
  int responseLength = 0;
  byte acknowledge = 0x00;
  for (int i = 0; i < sizeof(errorChk); i++)
  {
    Serial1.write(errorChk[i]);
  }
  while (Serial1.available() == 0)
  {
    ;;
  }
  if (Serial1.available() > 0)
  {
    while (acknowledge != 0x06)
    {
      acknowledge = Serial1.read();
    }
    delay(200);
    while (Serial1.available() > 0)
    {
      response[responseLength] = Serial1.read();
      responseLength++;
      delay(10);
    }
  }
  Serial1.write(ACK);

  internalFail = response[3];
  needReplaced = response[4];
  overload = response[5];
  shutdownImminent = response[6];

  /*for (int i = 0; i < responseLength; i++)
    {
    Serial.print(response[i], HEX);
    Serial.print(F(" "));
    }*/
  Serial.println();
  Serial.print(F("Internal Failure: "));
  if (internalFail == 1)
  {
    Serial.println(F("True"));
    sysError = 1;
  }
  else
  {
    Serial.println(F("False"));
  }
  Serial.print(F("UPS Needs Replaced: "));
  if (needReplaced == 1)
  {
    Serial.println(F("True"));
    sysError = 1;
  }
  else
  {
    Serial.println(F("False"));
  }
  Serial.print(F("System Overload: "));
  if (overload == 1)
  {
    Serial.println(F("True"));
  }
  else
  {
    Serial.println(F("False"));
  }
  Serial.print(F("UPS Shutdown Imminent: "));
  if (shutdownImminent == 1)
  {
    Serial.println(F("True"));
  }
  else
  {
    Serial.println(F("False"));
  }
  if (internalFail == 0 && needReplaced == 0)
  {
    sysError = 0;
  }
}

void checkFanTemp()
{
  byte response[50];
  int responseLength = 0;
  byte acknowledge = 0x00;
  for (int i = 0; i < sizeof(fan_tempChk); i++)
  {
    Serial1.write(fan_tempChk[i]);
  }
  while (Serial1.available() == 0)
  {
    ;;
  }
  if (Serial1.available() > 0)
  {
    while (acknowledge != 0x06)
    {
      acknowledge = Serial1.read();
    }
    delay(200);
    while (Serial1.available() > 0)
    {
      response[responseLength] = Serial1.read();
      responseLength++;
      delay(10);
    }
  }
  Serial1.write(ACK);

  fanFail = response[6];
  overTemp = response[7];

  /*for (int i = 0; i < responseLength; i++)
    {
    Serial.print(response[i], HEX);
    Serial.print(F(" "));
    }*/
  Serial.println();
  Serial.print(F("Fan Failure: "));
  if (fanFail == 1)
  {
    Serial.println(F("True"));
    fanError = 1;
  }
  else
  {
    Serial.println(F("False"));
  }
  Serial.print(F("UPS OverTemp: "));
  if (overTemp == 1)
  {
    Serial.println(F("True"));
    fanError = 1;
    if (overTemp_AlertSent == 0)
    {
      overTemp_Alert();
    }
  }
  else
  {
    Serial.println(F("False"));
  }
  if (fanFail == 0 && overTemp == 0)
  {
    fanError = 0;
    overTemp_AlertSent = 0;
  }
}

void checkCapRun()
{
  byte response[50];
  int responseLength = 0;
  byte acknowledge = 0x00;
  for (int i = 0; i < sizeof(cap_Runtime); i++)
  {
    Serial1.write(cap_Runtime[i]);
  }
  while (Serial1.available() == 0)
  {
    ;;
  }
  if (Serial1.available() > 0)
  {
    while (acknowledge != 0x06)
    {
      acknowledge = Serial1.read();
    }
    delay(200);
    while (Serial1.available() > 0)
    {
      response[responseLength] = Serial1.read();
      responseLength++;
      delay(10);
    }
  }
  Serial1.write(ACK);

  capacity = response[3];
  uint8_t runTime1 = response[4];
  uint8_t runTime2 = response[5];
  runTime = ((runTime2 << 8) | runTime1);
  //Serial.println();
  /*for (int i = 0; i < responseLength; i++)
    {
    if (i == 3 || i == 4)
    {
      Serial.print(F("("));
    }
    Serial.print(response[i], HEX);
    if (i == 3 || i == 5)
    {
      Serial.print(F(")"));
    }
    Serial.print(F(" "));
    }*/
  Serial.println();
  Serial.print(F("Battery Capacity: "));
  Serial.print(capacity);
  Serial.println(F("%"));
  Serial.print(F("Battery Runtime: "));
  Serial.print(runTime, DEC);
  Serial.print(F(" seconds / "));
  Serial.print(runTime / 60, DEC);
  Serial.println(F(" minutes"));
  int timeLeft = int(runTime / 60);
  //Serial.println(timeLeft);
  if (timeLeft < 5 && onBatt)
  {
    lowBatt = 1;
    if (lowBatt_AlertSent = 0)
    {
      lowBatt_Alert();
    }
  }
  else
  {
    lowBatt = 0;
    lowBatt_AlertSent = 0;
  }
}

void checkInputInfo()
{
  byte response[50];
  int responseLength = 0;
  byte acknowledge = 0x00;
  for (int i = 0; i < sizeof(inputCheck); i++)
  {
    Serial1.write(inputCheck[i]);
  }
  while (Serial1.available() == 0)
  {
    ;;
  }
  if (Serial1.available() > 0)
  {
    while (acknowledge != 0x06)
    {
      acknowledge = Serial1.read();
    }
    delay(200);
    while (Serial1.available() > 0)
    {
      response[responseLength] = Serial1.read();
      responseLength++;
      delay(10);
    }
  }
  Serial1.write(ACK);

  uint8_t freq1 = response[3];
  uint8_t freq2 = response[4];
  uint8_t voltage1 = response[5];
  uint8_t voltage2 = response[6];
  inputFreq = float (((freq2 << 8) | freq1)) / 10;
  inputVoltage = float (((voltage2 << 8) | voltage1)) / 10;
  inV_Response = int(inputVoltage);

  /*for (int i = 0; i < responseLength; i++)
    {
    Serial.print(response[i], HEX);
    Serial.print(F(" "));
    }*/
  Serial.println();
  Serial.print(F("Input Frequency: "));
  Serial.print(inputFreq, 1);
  Serial.println(F(" Hz"));
  Serial.print(F("Input Voltage: "));
  Serial.print(inputVoltage, 1);
  Serial.println(F(" VAC\n"));
}

void checkOutputInfo()
{
  byte response[50];
  int responseLength = 0;
  byte acknowledge = 0x00;
  for (int i = 0; i < sizeof(outputCheck); i++)
  {
    Serial1.write(outputCheck[i]);
  }
  while (Serial1.available() == 0)
  {
    ;;
  }
  if (Serial1.available() > 0)
  {
    while (acknowledge != 0x06)
    {
      acknowledge = Serial1.read();
    }
    delay(200);
    for (int x = 0; x < 4; x++)
    {
      while (Serial1.available() == 0)
      {
        ;;
      }
      while (Serial1.available() > 0)
      {
        response[responseLength] = Serial1.read();
        //Serial.print(response[responseLength], HEX);
        //
        //Serial.print(F(" "));
        responseLength++;
        delay(10);
      }
      //Serial.println();
      Serial1.write(ACK);
      //delay(200);
    }
    uint8_t current1 = response[14];
    uint8_t current2 = response[15];
    current = int((current2 << 8) | current1);
    efficiency = int(response[17]);
    outFreq = int(response[20]);
    uint8_t outV1 = response[38];
    uint8_t outV2 = response[39];
    outVoltage = (float((outV2 << 8 | outV1)) / 10);
    outV_Response = int(outVoltage);
    Serial.print(F("Output Voltage: "));
    Serial.print(outVoltage, 1);
    Serial.println(F(" VAC"));
    /*Serial.print(F("Output Frequency: "));
      Serial.print(outFreq, DEC);
      Serial.println(F(" Hz"));
      Serial.print(F("Output Efficiency: "));
      Serial.print(efficiency, DEC);
      Serial.println(F("%"));*/
    Serial.print(F("Output Current: "));
    float c2 = float(current);   
    Serial.print(c2/10, 1);
    Serial.println(F(" Amps"));
    Serial.println();
  }
}

void checkBattStats()
{
  byte response[50];
  int responseLength = 0;
  byte acknowledge = 0x00;
  for (int i = 0; i < sizeof(battStats); i++)
  {
    Serial1.write(battStats[i]);
  }
  while (Serial1.available() == 0)
  {
    ;;
  }
  if (Serial1.available() > 0)
  {
    while (acknowledge != 0x06)
    {
      acknowledge = Serial1.read();
    }
    delay(200);
    int cnt = 0;
    for (int x = 0; x < 2; x++)
    {
      while (Serial1.available() == 0)
      {
        ;;
      }
      while (Serial1.available() > 0)
      {
        response[responseLength] = Serial1.read();
        //Serial.print(cnt);
        //Serial.print(F(" "));
        //Serial.println(response[responseLength], HEX);
        //Serial.print(F(" "));
        //cnt++;
        responseLength++;
        delay(10);
      }
      //Serial.println();
      Serial1.write(ACK);
      delay(200);
    }
    /*for (int i = 0; i < responseLength; i++)
    {
      Serial.print(response[i]);
      Serial.print(F(" "));
    }*/
    //Serial.println();
    //loadPercent = int(response[6]);
    //uint8_t battTemp1 = response[7];
    //uint8_t battTemp2 = response[8];
    //battT = int((battTemp2 << 8) | battTemp1);
    uint8_t battV1 = response[13];
    //Serial.println(response[13], BIN);
    uint8_t battV2 = response[14];
    //Serial.println(response[14], BIN);
    battV = float(int(battV1 << 8 | battV2) / 10);
    battV_Response = int(battV);
    if ((battV < 45.0) && (capacity == 100))
    {
      deadBatt = 1;
      if (deadBatt_AlertSent == 0)
      {
        deadBatt_Alert();
      }
    }
    else
    {
      deadBatt = 0;
      deadBatt_AlertSent = 0;
    }
    //Serial.print(F("Battery Load %: "));
    //Serial.print(loadPercent);
    //Serial.println(F("%"));
    //float tempT = (battT - 273.15) * 9 / 5 + 32; //kelvin temp to fahrenheit
    //Serial.print(F("Battery Temperature: "));
    //Serial.print(tempT, 1);
    //Serial.println(F(" F"));
    Serial.print(F("Battery Voltage: "));
    Serial.print(battV, 1);
    Serial.println(F(" VDC"));
    Serial.println();
  }
}

void sendOnBattery_Alert()
{
  for (uint8_t i = 0; i < 3; i++)
  {
    Serial3.print(7, DEC);
  }
  Serial3.println();
  String onBatt = "ON BATTERY\r\n>";
  Serial3.print(onBatt);
  onBatt_AlertSent = 1;
  onBatt = 1;
}

void sendACLineRecovered_Alert()
{
  for (uint8_t i = 0; i < 3; i++)
  {
    Serial3.print(7, DEC);
  }
  Serial3.println();
  String acReturn = "AC LINE RETURNED\r\n>";
  Serial3.print(acReturn);
  onBatt = 0;
}

void sendOutputDisabled_Alert()
{
  for (uint8_t i = 0; i < 3; i++)
  {
    Serial3.print(7, DEC);
  }
  Serial3.println();
  String outD = "OUTPUT DISABLED\r\n>";
  Serial3.print(outD);
  outputDisable_AlertSent = 1;
}

void sendOutputEnabled_Alert()
{
  for (uint8_t i = 0; i < 3; i++)
  {
    Serial3.print(7, DEC);
  }
  Serial3.println();
  String outE = "OUTPUT ENABLED\r\n>";
  Serial3.print(outE);
}

void lowBatt_Alert()
{
  for (uint8_t i = 0; i < 3; i++)
  {
    Serial3.print(7, DEC);
  }
  Serial3.println();
  String lowB = "LOW BATTERY\r\n>";
  Serial3.print(lowB);
  lowBatt_AlertSent = 1;
}

void deadBatt_Alert()
{
  for (uint8_t i = 0; i < 3; i++)
  {
    Serial3.print(7, DEC);
  }
  Serial3.println();
  String deadB = "DEAD BATTERY\r\n>";
  Serial3.print(deadB);
  deadBatt_AlertSent = 1;
}

void overTemp_Alert()
{
  for (uint8_t i = 0; i < 3; i++)
  {
    Serial3.print(7, DEC);
  }
  Serial3.println();
  String overT = "OVERTEMPERATURE\r\n>";
  Serial3.print(overT);
  overTemp_AlertSent = 1;
}

void changeLED()
{
  digitalWrite(led_pin, !digitalRead(led_pin));
  ledTimer = millis();
}
