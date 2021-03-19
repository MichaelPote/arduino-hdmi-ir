#include "IR.h"


IRSenderClass IRSender;

IRSenderClass::IRSenderClass() {
  
}

void IRSenderClass::begin(uint8_t aSendPin)
{
  sendPin = aSendPin;

  pinMode(sendPin, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

Serial.println("Clearing Buffer...");
  memset(dataBuffer, 0, sizeof(dataBuffer));  

Serial.println("INIT Timer...");

  noInterrupts();           // disable all interrupts

  // initialize timer1 to generate PWM:
  const uint32_t pwmval = (SYSCLOCK / 2000) / 38;    // 210,52 for 38 kHz @16 MHz clock, 2000 instead of 1000 because of Phase Correct PWM
  TCCR1A = _BV(WGM11);                               // PWM, Phase Correct, Top is defined by ICR1
  TCCR1B = _BV(WGM13) | _BV(CS10);                   // PWM, Phase Correct, CS10 -> no prescaling
  ICR1 = pwmval - 1;
  OCR1A = ((pwmval * IR_SEND_DUTY_CYCLE) / 100) - 1;
  TCNT1 = 0;  //Clear counter value.

  // Initalise timer2 to perform the sending operations:
  TCCR2A = 0;         //No PWM or pin output.
  TCCR2B = _BV(CS22); //CS22 -> 1/64 prescaling
  TCNT2 = 0; //Clear counter value.
  OCR2A = 140;
  //TIMSK2 = _BV(OCIE2A); //Enable the Interrupt for counter2 on match with OCR2A.
  

  interrupts();             // enable all interrupts
  Serial.println("DONE.");

}

void IRSenderClass::enableSending()
{
  TIMSK2 = _BV(OCIE2A); //Enable the Interrupt for counter2 on match with OCR2A.
  digitalWriteFast(LED_BUILTIN, true); //Make sure LED is ON.
}

void IRSenderClass::disableSending()
{
  TIMSK2 = 0; //Disable all Interrupts for counter2.
  digitalWriteFast(LED_BUILTIN, false); //Make sure LED is off.
}

void IRSenderClass::send(uint16_t address, uint8_t command)
{
  //https://www.sbprojects.net/knowledge/ir/nec.php
  
    LongUnion data;

    // Address 16 bit LSB first
    if ((address & 0xFF00) == 0) 
    {
        // assume 8 bit address -> send 8 address bits and then 8 inverted address bits LSB first
        data.Bytes.LowByte = address;
        data.Bytes.MidLowByte = ~data.Bytes.LowByte;
        
    } else 
    {
        data.Words.LowWord = address;
    }

    // send 8 command bits and then 8 inverted command bits LSB first
    data.Bytes.MidHighByte = command;
    data.Bytes.HighByte = ~command;

    uint8_t states[34];

    states[0] = IR_STATE_HEADER;

    uint32_t aData = data.Long;
    for (uint_fast8_t bit = 0; bit < 32; bit++, aData >>= 1)
    {
            if (aData & 1) {  // Send a 1
                states[bit+1] = IR_STATE_ONE;
            } else {  // Send a 0
                states[bit+1] = IR_STATE_ZERO;
            }
    }
    states[33] = IR_STATE_TAIL;

    writeData(states, 34);
}

void IRSenderClass::sendRepeat()
{
  uint8_t states[2];
  states[0] = IR_STATE_REPEAT;
  states[1] = IR_STATE_TAIL;

  writeData(states, 2);
}

void IRSenderClass::writeData(uint8_t data[], uint8_t len)
{
  noInterrupts();
  uint8_t cur = dataWriteCursor;
  for (uint8_t i = 0; i < len; i++)
  {
    dataBuffer[cur] = data[i];
    cur++;
    if (cur == dataReadCursor) break;
  }
  dataWriteCursor = cur;
  if (dataWriteCursor != dataReadCursor)
  {
    dataBuffer[dataWriteCursor] = IR_STATE_NONE;
    dataWriteCursor++;
  }
  
  enableSending();
  interrupts();
  
}

void IRSenderClass::setCommand()
{
  uint8_t cur = dataReadCursor;
  bool found = false;
  while (cur != dataWriteCursor)
  {
    if ((dataBuffer[cur] == IR_STATE_HEADER) || (dataBuffer[cur] == IR_STATE_REPEAT))
    {
      found = true;
      dataReadCursor = cur;
      break;
    }

    cur++;
  }


  if (!found)
  {
    disableSending(); //No more commands found. Sleep.
  }
  else
  {
    commandTicksLeft = TICKS_PER_COMMAND;
    setNextState();
  }
}

void IRSenderClass::setNextState()
{
  state = dataBuffer[dataReadCursor];
  
  switch (state)
  {
      case IR_STATE_NONE:
      case IR_STATE_WAIT:
        markTicksLeft = 0;
        totalTicksLeft = commandTicksLeft;
      break;

      case IR_STATE_HEADER:
        markTicksLeft = TICKS_MARK_HEADER;
        totalTicksLeft = TICKS_TOTAL_HEADER;        
      break;

      case IR_STATE_ONE:
        markTicksLeft = TICKS_MARK_ONE;
        totalTicksLeft = TICKS_TOTAL_ONE;
      break;
    
      case IR_STATE_ZERO:
        markTicksLeft = TICKS_MARK_ZERO;
        totalTicksLeft = TICKS_TOTAL_ZERO;
      break;

      case IR_STATE_TAIL:
        markTicksLeft = TICKS_MARK_TAIL;
        totalTicksLeft = TICKS_TOTAL_TAIL;
      break;

      case IR_STATE_REPEAT:
        markTicksLeft = TICKS_MARK_REPEAT;
        totalTicksLeft = TICKS_TOTAL_REPEAT;
      break;
      
  }

  if (markTicksLeft > 0) {
    ENABLE_PWM;
  }
  else
  {
    DISABLE_PWM;
  }

  dataReadCursor++;

/*
  Serial.println("Data Buffer is now:");
  for (uint16_t j = 0; j < 100; j++)
  {
    if (j == dataReadCursor) Serial.print("R"); else Serial.print(" ");
    if (j == dataWriteCursor) Serial.print("W"); else Serial.print(" ");
    Serial.print(dataBuffer[j]);
  }
  Serial.println();
  */

}

void IRSenderClass::tick()
{
  if (commandTicksLeft == 0) //This command is finished, OR we're starting a brand new transmit:
  {
    DISABLE_PWM;
    setCommand(); //Load new command if found, or disable timer if not found.
  }
  else 
  {
    commandTicksLeft--;
    if (totalTicksLeft > 0) //This state still has ticks left:
    {
      if (markTicksLeft > 0) //This state still has mark ticks left:
      {
        markTicksLeft--;
        if (markTicksLeft == 0)
        {
          DISABLE_PWM;
        }
      }
  
      totalTicksLeft--;
      if (totalTicksLeft == 0)
      {
        setNextState();
      }
    }  
  }
  
}

ISR(TIMER2_COMPA_vect)          // timer compare interrupt service routine
{
  IRSender.tick();
  TCNT2  = 0;
}

/**
 * 
 * Remote Codes

POWER:
Address: 0x6CD2
Command: 0xCB


VOL UP:
Address: 0x6DD2
Command: 0x2

VOL DOWN:
Address: 0x6DD2
Command: 0x3


MUTE:
Address: 0x6DD2
Command: 0x5

CYCLE DIM BRIGHTNESS:
Address: 0x6DD2
Command: 0x95

SWAP DISPLAY:
Address: 0x6CD2
Command: 0x55

PLAYBACK: MOVIE/TV
Address: 0xACD2
Command: 0xD0

PLAYBACK: MUSIC
Address: 0xACD2
Command: 0xD1

PLAYBACK: GAME
Address: 0xACD2
Command: 0xD2

PLAYBACK: STEREO
Address: 0x6CD2
Command: 0x4C


Repeat last command:
Address: 0xFFFF
Command: 0x0


*/
