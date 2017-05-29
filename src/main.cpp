#include "mbed.h"
#include <string>
#include <cJSON.h>
#include <Flasher.h>
#include <SawTooth.h>
#include <PID.h>
#include <WS2812.h>
#include <PixelArray.h>
#include <Adafruit_ADS1015.h>

#define WS2812_BUF 6
#define NUM_COLORS 6
#define NUM_LEDS_PER_COLOR 10

PixelArray px(WS2812_BUF);

// NOTE: timer is critical for different platforms:
// K64F, KL46Z: 0, 5, 5, 0
// LPC1768: 3, 11, 10, 11
// NUCLEO_F401RE: 3, 12, 9, 12
// NUCELO_F746ZG: 32, 105, 70, 123

WS2812 ws(D11, 1, 3, 12, 9, 12);
//WS2812 ws(p11, 1);
//int colorbuf[4] = { 0x00000033, 0x0000FFFF, 0x0000FF00, 0x00001199}; //blue, orange, green, red
int colorbuf[NUM_COLORS] = {0xff0000ff,0xffff0000,0xff00ff00,0xffffff00,0xffff8000,0xfff00fff};

I2C i2c(D4, D5);
Serial pc(USBTX, USBRX);
// Adafruit_ADS1015 ads(&i2c);

//****************************************************************************/
// Defines PID parameters
//****************************************************************************/
#define SAMPLES 5
#define RATE  0.2
#define Kc    0.65
#define Ti    0.001
#define Td    0.0
#define HIGH_FACTOR 3
#define LOW_FACTOR 2

PID controllerA(Kc, Ti, Td, RATE);
PID controllerB(Kc, Ti, Td, RATE);
//ADC pins
AnalogIn tempReadA(A0);
AnalogIn tempReadB(A1);

//PWM pins
// PwmOut heaterA(p21);
// PwmOut heaterB(p22);
PwmOut heaterA(D9);
PwmOut heaterB(D10);

// DAC pin18
//AnalogOut aout(p18);

//Ticker flipper;
//Ticker flipper2;
//PwmOut led1(LED1);
//DigitalOut led1(LED1);
//DigitalOut led2(LED2);
//DigitalOut led3(LED3);
//DigitalOut led4(LED4);

SawTooth sawTooth(A3, 0.5);
Flasher led3(LED3);
Flasher led4(LED4, 2);

double setPointA, setPointB, kc, ti, td;

void readPC() {
  // Note: you need to actually read from the serial to clear the RX interrupt
  // Example command:
  // {"setPointA":20, "setPointB":45, "kc":0.08, "ti":0.005, "td":0.0}
  string holder;
  cJSON *json;
  // parameters list

  char temp;
  while(temp != '\n') {
    temp = pc.getc();
    holder += temp;
  }
  if (holder.length() < 10) return;

  json = cJSON_Parse(holder.c_str());
  if (!json) {
    printf("Error before: [%s]\n", cJSON_GetErrorPtr());
  } else {
    setPointA = cJSON_GetObjectItem(json, "setPointA")->valuedouble;
    setPointB = cJSON_GetObjectItem(json, "setPointB")->valuedouble;
    kc = cJSON_GetObjectItem(json, "kc")->valuedouble;
    ti = cJSON_GetObjectItem(json, "ti")->valuedouble;
    td = cJSON_GetObjectItem(json, "td")->valuedouble;
    cJSON_Delete(json);
  }

  controllerA.setSetPoint(setPointA);
  controllerA.setTunings(kc, ti, td);
  controllerB.setSetPoint(setPointB);
  controllerB.setTunings(kc, ti, td);
  printf("setPoints: %3.1f'C %f3.1'C\n", setPointA, setPointB);
  printf("%s\n", holder.c_str());
}

// Function to convert ADC reading to actual temp reading
double theta[3] = {1050.7, -4826, 5481.5};
double readRTD(double x) {
  return theta[0] + x*theta[1] + x*x*theta[2];
}

int main() {
  double tempA, tempB, outA, outB;
  double read_bufferA[SAMPLES] = {25};
  double read_bufferB[SAMPLES] = {25};
  double sumA = 0, sumB = 0;
  long int reading = 0;

  pc.attach(&readPC);

  heaterA.period_ms(50);
  heaterB.period_ms(50);

  // Init PIC controllers
  controllerA.setInputLimits(0.0, 350.0);
  controllerA.setOutputLimits(0.0, 1.0);
  controllerA.setSetPoint(20);
  //controllerA.setBias(0.0);
  controllerA.setMode(1);

  controllerB.setInputLimits(0.0, 350.0);
  controllerB.setOutputLimits(0.0, 1.0);
  controllerB.setSetPoint(20);
  controllerA.setBias(0);
  controllerB.setMode(MANUAL_MODE);

  while(1) {
    sumA = 0;
    sumB = 0;
    // Moving average
    for (int i=SAMPLES - 1; i>0; i--) {
      read_bufferA[i] = read_bufferA[i-1];
      read_bufferB[i] = read_bufferB[i-1];
    }

    read_bufferA[0] = readRTD(tempReadA.read());
    read_bufferB[0] = readRTD(tempReadB.read());

    for (int i=0; i<SAMPLES; i++) {
      sumA += read_bufferA[i];
      sumB += read_bufferB[i];
    }

    tempA = sumA/SAMPLES;
    tempB = sumB/SAMPLES;

    //printf("Tube Sealer Temperature A: %3.4f'C\n", temp*3.3);
    //printf("normalized: 0x%04X \n", tempReadA.read_u16());
    controllerA.setProcessValue(tempA);
    controllerB.setProcessValue(tempB);
    outA = controllerA.compute();
    outB = controllerB.compute();

    if(tempA >= setPointA) {
      outA = outA/HIGH_FACTOR;
    }

    if((setPointA - tempA) <= 0.6) {
      outA = outA/LOW_FACTOR;
    }

    if(tempB >= setPointB) {
      outB = outB/HIGH_FACTOR;
    }

    if((setPointB - tempB) <= 0.6) {
      outB = outB/LOW_FACTOR;
    }

    // Update Heaters PWM output
    heaterA.write(outA);
    heaterB.write(outB);
    printf("HeaterA: Temp: %3.1f 'C, PWM: %3.3f %%; HeaterB: Temp: %3.1f 'C, PWM: %3.3f %%;\n", tempA, outA*100, tempB, outB*100);
    wait(RATE);

    // Test WS2812
    //ws.useII(WS2812::GLOBAL);
    //ws.setII(0xAA);
    // ws.write(&colorbuf[0]);
    // wait(RATE);
    // ws.write(&colorbuf[1]);
    // wait(RATE);
    // ws.write(&colorbuf[2]);
    // wait(RATE);
    // ws.write(&colorbuf[3]);
    // wait(RATE);
    // ws.write(&colorbuf[4]);
    // wait(RATE);
    // ws.write(&colorbuf[5]);
  }
}
