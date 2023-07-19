#include <Arduino.h>
#include <TeensyTimerTool.h>
#include <SPI.h>
using namespace TeensyTimerTool;

/* DATALEN in bits */
#define DATALEN 4
/* CHIPPERIOD in micro-seconds */
#define CHIPPERIOD 4
#define SAMPPERIOD 1
/* TIMER_COUNT is xxx for 150MHz ipg_clk_root */
#define TIMER_COUNT 150
/* Enable system messages */
#define VERBOSE


/* Finite State Machine */
enum FSMState {IDLE, TIME, FEED, DONE};
volatile FSMState state;

/* ledPin - use LED_BUILTIN (pin 13) for testing */
const int ledPin = LED_BUILTIN;

volatile int bWrite = 0;
volatile int chipCount = 0;
volatile boolean bitConsumed = 0;
//const int initData[DATALEN] = {0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
//int data[DATALEN] = {0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
const int initData[DATALEN] = {0, 1, 0, 0};
int data[DATALEN] = {0, 1, 0, 0};

boolean startTransmission = 0;
boolean stIdlePromptFlag = 1;

/* Function Declarations */
static void writeNextChip(void);
static void sampChannel(void);
static void restoreData(void);

void setup() {
  pinMode(ledPin, OUTPUT);

  CCM_CSCMR1 &= ~CCM_CSCMR1_PERCLK_CLK_SEL; // Using 150MHz ipg_clk_root

  /* ROUTE CLOCK TO GPTs */
  CCM_CCGR1 |= CCM_CCGR1_GPT1_BUS(CCM_CCGR_ON); // Enable clock to GPT1 module
  CCM_CCGR0 |= CCM_CCGR0_GPT2_BUS(CCM_CCGR_ON); // Enable clock to GPT2 module
  
  /* INIT */
  cli();

  GPT1_CR = 0;                            // Disable for configuration
  GPT1_PR = 1 - 1;                        // Prescale GPT1 source clock by 1 => 24 MHz or 150 MHz depending on source clock set by CCM_CSCMR1
  GPT1_CR = GPT_CR_CLKSRC(1)              /* 150 MHz peripheral clock as clock source */         
            & ~GPT_CR_FRR                 /* Restart counter upon compare event */;             
// | GPT_CR_FRR /* Free-Run, do not reset */;                                                   
  GPT1_OCR1 = (CHIPPERIOD*TIMER_COUNT)-1; // set the value for when a compare event is generated = (req period / timer period) - 1
  GPT1_IR = GPT_IR_OF1IE;                 // enable GPT interrupts, write 1s to their enable bits in IR
  // Enable Interrupt
  NVIC_ENABLE_IRQ(GPT1_SR&(GPT_SR_OF1));
  attachInterruptVector(IRQ_GPT1, &writeNextChip);
  NVIC_SET_PRIORITY(IRQ_GPT1, 0); // Top priority, lower number higher priority
  NVIC_ENABLE_IRQ(IRQ_GPT1);

  GPT2_CR = 0;
  GPT2_PR = 1 - 1;                        // Prescale GPT2 source clock by 1 => 24 MHz or 150 MHz depending on source clock set by CCM_CSCMR1
  GPT2_CR = GPT_CR_CLKSRC(1)              /* 150 MHz peripheral clock as clock source */
            & ~GPT_CR_FRR                 /* Restart counter upon compare event */;
// | GPT_CR_FRR /* Free-Run, do not reset */;
  GPT2_OCR1 = (SAMPPERIOD*TIMER_COUNT)-1; // set the value for when a compare event is generated = (req period / timer period) - 1
  GPT2_IR = GPT_IR_OF1IE;                 // enable GPT interrupts, write 1s to their enable bits in IR
  // Enable Interrupt
  NVIC_ENABLE_IRQ(GPT2_SR&(GPT_SR_OF1));
  attachInterruptVector(IRQ_GPT2, &sampChannel);
  NVIC_SET_PRIORITY(IRQ_GPT2, 0); // Top priority, lower number higher priority
  NVIC_ENABLE_IRQ(IRQ_GPT2);

  sei();
  
  while (!Serial) {}
  state = IDLE;
  chipCount = 0;
  digitalWriteFast(ledPin, bWrite); // Initialize by transmitting a 0
}

void loop() {
  // put your main code here, to run repeatedly:
  
  switch (state) {
    case IDLE:
      if (stIdlePromptFlag) {
        Serial.println("Enter s to start transmission...");
        stIdlePromptFlag = 0;
      }
      
      if (Serial.available() > 0) {
        char receivedChar = Serial.read();

        if (receivedChar == 's') {
          startTransmission = 1;
          Serial.println("TRANSMITTING...");
        }
      }

      if (startTransmission) {
        state = TIME;
        digitalWriteFast(ledPin, data[0]);  // Transmit the first bit
        bWrite = data[1];                   // feed in next data bit
        chipCount++;                        // interrupt timer should not have begun at here, so it is safe
        bitConsumed = 1;

        /* enable timer */
        GPT1_CR |= GPT_CR_EN;
        GPT2_CR |= GPT_CR_EN; 

      } else {
        state = IDLE;
      }
      break;
    
    case TIME: // Transition State
      
      if (bitConsumed) {
        state = FEED;
        
      }
      break;

    case FEED:
      cli();
      if (chipCount < DATALEN) {
        bWrite = data[chipCount];
        data[chipCount] = 0; // used to overwrite consumed data bits to 0, should be optional
        bitConsumed = 0;
        state = TIME;
      } else {
        bWrite = 0; // pad a 0 at the end
        bitConsumed = 0;
        state = DONE;
      }
      sei();
      break;

    case DONE:
      
      if (bitConsumed) { // the padded 0 at the end to be written to output pin first before ending timer and going back idle state
        /* disable timer */
        GPT1_CR &= ~GPT_CR_EN; 
        GPT2_CR &= ~GPT_CR_EN; 

        restoreData();
        chipCount = 0;
        state = IDLE;
        startTransmission = 0;
        stIdlePromptFlag = 1;
        Serial.println("DONE!");
      }
      break;

    default:
      Serial.println("ERROR!");
      cli();
      chipCount = 0;
      sei();
      state = IDLE;
      startTransmission = 0;
      stIdlePromptFlag = 1;
      break;
  }
}

// Function Definitions
static void writeNextChip(void)
{
  //digitalWriteFast(ledPin,!digitalReadFast(ledPin));
  if(GPT1_SR&(GPT_SR_OF1))
  {
    GPT1_SR = GPT_SR_OF1; // write to GPT1_SR[OF1] clears the interrupt

    digitalWriteFast(ledPin, bWrite);
    
    chipCount++;
    bitConsumed = 1;
  }
}

static void sampChannel(void) {

  if(GPT2_SR&(GPT_SR_OF1))
  {
    GPT2_SR = GPT_SR_OF1; // write to GPT2_SR[OF1] clears the interrupt
    Serial.println("Sampling...");
  }
}

static void restoreData(void) {
  for (int i = 0; i < DATALEN; i++) {
    data[i] = initData[i];
  }
}

