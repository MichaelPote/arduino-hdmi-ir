#ifndef IR_H__
#define IR_H__

#include "Arduino.h"
#include "digitalWriteFast.h"


#define SYSCLOCK 16000000 //16mhz for Uno
#define IR_SEND_DUTY_CYCLE 30 // 30 saves power and is compatible to the old existing code

#define ENABLE_PWM TCNT1 = 0; (TCCR1A |= _BV(COM1A1))  // Clear OC1A/OC1B on Compare Match when up-counting. Set OC1A/OC1B on Compare Match when downcounting.
#define DISABLE_PWM (TCCR1A &= ~(_BV(COM1A1)))

//NEC IR PROTOCOL:

#define MICROS_PER_TICK 560
#define TICKS_PER_UNIT 1 //Unit is 560us ~= 21.5 half ticks.

#define IR_STATE_NONE 0
#define IR_STATE_HEADER 1
#define IR_STATE_ONE 2
#define IR_STATE_ZERO 3
#define IR_STATE_TAIL 4
#define IR_STATE_REPEAT 5
#define IR_STATE_WAIT 6 
//^ SPECIAL: No data left for this command, but 110ms isnt up yet, wait.

#define TICKS_TOTAL_HEADER (TICKS_PER_UNIT * 24) //13.5ms
#define TICKS_MARK_HEADER (TICKS_PER_UNIT * 16) //9ms

#define TICKS_TOTAL_ONE (TICKS_PER_UNIT * 4) //2.25ms
#define TICKS_MARK_ONE (TICKS_PER_UNIT)     //560us

#define TICKS_TOTAL_ZERO (TICKS_PER_UNIT * 2) //1.12ms
#define TICKS_MARK_ZERO (TICKS_PER_UNIT)     //560us

#define TICKS_TOTAL_TAIL (TICKS_PER_UNIT)
#define TICKS_MARK_TAIL (TICKS_PER_UNIT)

#define TICKS_TOTAL_REPEAT (TICKS_PER_UNIT * 20) //11.25ms
#define TICKS_MARK_REPEAT (TICKS_PER_UNIT * 4) //2.25ms

#define TICKS_PER_COMMAND (TICKS_PER_UNIT * 197) //110ms

union LongUnion
{
  struct {
        uint8_t LowByte;
        uint8_t MidLowByte;
        uint8_t MidHighByte;
        uint8_t HighByte;
  } Bytes;

  struct {
        uint16_t LowWord;
        uint16_t HighWord;
  } Words;
  
  uint32_t Long;
};

class IRSenderClass {

  public:

    IRSenderClass();

    void begin(uint8_t aSendPin);
    void enableSending();
    void disableSending();

    

    void send(uint16_t address, uint8_t command);
    void sendRepeat();
    
    uint8_t sendPin;
    bool carrierPulse = false;

    uint8_t  dataBuffer[256];
    volatile uint8_t dataReadCursor = 0;
    volatile uint8_t dataWriteCursor = 0;
    
    uint8_t state = IR_STATE_NONE;
    uint16_t totalTicksLeft = 0; //Time left for the current state (in ticks)
    uint16_t markTicksLeft = 0;  //Time left for the mark portion of the current state (in ticks)
    uint16_t commandTicksLeft = 0; //Time left for this entire command, after which another command or repeat can be transmitted.
    
    void tick();

    void setCommand();
    void setNextState();
    void writeData(uint8_t data[], uint8_t len);
    
};

//IRSenderClass IRSender;

extern IRSenderClass IRSender;

#endif
