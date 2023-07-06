#include <Arduino.h>
#include <TeensyTimerTool.h>
using namespace TeensyTimerTool;

// CHIPPERIOD is in micro-seconds
#define CHIPPERIOD 0.5
#define VERBOSE

PeriodicTimer ppmChipTimer(GPT1);
const int ledPin = 14; // Pin 13

volatile int bWrite = 0;

// Function Declarations
void writeNextChip(void);

void setup() {
 
  CCM_CSCMR1 &= ~64; // Change peripheral clock source from OSC (24MHz) to ipg_clk_root (150MHz) - Page 1051 rev3  

  pinMode(ledPin, OUTPUT);
  digitalWriteFast(ledPin, bWrite);
  
  
  ppmChipTimer.begin(writeNextChip, CHIPPERIOD); // start interrupt timer
}

FASTRUN void loop() {
  // put your main code here, to run repeatedly:
}

// Function Definitions
FASTRUN void writeNextChip(void)
{
  digitalWriteFast(ledPin,!digitalReadFast(ledPin));
}