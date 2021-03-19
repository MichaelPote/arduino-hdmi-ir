/*
 * HDMI CEC Decode and IR Remote output.
 * Michael Pote - March 2021
*/

#define IR_PIN 9 //Arduino UNO Timer 1 (OC1A) PWM is on Pin 9.

#include <CEClient.h>
#include "IR.h"

#define CEC_PHYSICAL_ADDRESS    0x2000
#define CEC_INPUT_PIN           2
#define CEC_OUTPUT_PIN          3

#define BTN_VOL_UP 1
#define BTN_VOL_DOWN 2
#define BTN_MUTE 3
#define BTN_POWER 4
#define BTN_REPEAT 99

bool powerOn = false;
bool muteOn = false;
bool buttonPressed = false;
bool repeatUntilRelease = false;
uint8_t repeatsLeft = 0;
bool firstPress = false;
uint8_t buttonCommand = 0;

uint16_t timer = 0;


void sendRemoteCommand(uint16_t aAddress, uint8_t aCommand)
{
  IRSender.send(aAddress, aCommand);
}

void sendRemoteRepeat()
{
  IRSender.sendRepeat();
}

void pressButton(int btn, bool holdDown)
{
  Serial.print("PRESSING BUTTON ");
  Serial.println(btn);
  
  buttonPressed = true;
  buttonCommand = btn;
  repeatUntilRelease = holdDown;
  firstPress = true;
  repeatsLeft = 4;
  timer = 0;
}

void onReceiveCEC(int source, int dest, unsigned char* data, int count)
{
  /**
  Serial.print("Got CEC: Source: ");
  Serial.print(source, HEX);
  Serial.print(" (");
  Serial.print(source);
  Serial.print(") Dest: ");
  Serial.print(dest, HEX);
  Serial.print(" (");
  Serial.print(dest);
  Serial.print(") Len: ");
  Serial.print(count);
  if (count > 0)
  {
    Serial.print(" 1stData: ");
    Serial.print(data[0], HEX);
  }

  Serial.println();
  **/
  
  if (count == 0) { return;}
  if (source != 0) { return; }
  if (dest != 5 && dest != 15) { return; }

  
  
  switch (data[0])
  {
    case 0x36: //TV going to standby
        pressButton(BTN_POWER, false);
        powerOn = false;
    break;
    
    case 0x46: //Give audio name
    case 0x83: //Give audio physical address
      if (!powerOn)
      {
        pressButton(BTN_POWER, false);
        powerOn = true;
      }
    break;


    case 0x45: //Button released
      buttonPressed = false;
    break;

    case 0x44: //Button pressed
         if (count > 1)
         {
          if (data[1] == 0x41) //VOLUME UP
          {
            pressButton(BTN_VOL_UP, true);
          }
          else if (data[1] == 0x42) // VOLUME DOWN
          {
            pressButton(BTN_VOL_DOWN, true);
          }
          else if (data[1] == 0x43) //MUTE
          {
            pressButton(BTN_MUTE, false);
            muteOn = !muteOn;
          }
         }
    break;

    
  }
}


// create a CEC client
CEClient ceclient(CEC_PHYSICAL_ADDRESS, CEC_INPUT_PIN, CEC_OUTPUT_PIN);


void setup() {

    Serial.begin(115200);

    // initialize the client with the AUDIO device type
    ceclient.begin(CEC_LogicalDevice::CDT_AUDIO_SYSTEM);

    // enable promiscuous mode (read all messages on the bus)
    ceclient.setPromiscuous(true);

    // enable monitor mode (do not transmit)
    ceclient.setMonitorMode(true);
    ceclient.onReceiveCallback(onReceiveCEC);
    Serial.println("HDMI Monitoring started.");
    IRSender.begin(IR_PIN);
    Serial.println("IR Sender started.");

    digitalWrite(IR_PIN, false);
}



void loop() {

    // run the client
    ceclient.run();
    
    if ((timer == 0) && (buttonPressed) && (buttonCommand > 0)) //It's time to do an update
    { 
      Serial.print("Sending command! firstPress? ");
      Serial.print((firstPress ? "YES" : "NO"));
      Serial.print(" - ");
      
      
      if (firstPress)
      {
       
        switch (buttonCommand)
        {
          case BTN_POWER:
            Serial.print("POWER");
            sendRemoteCommand(0x6CD2, 0xCB);
          break;
          case BTN_VOL_UP:
            Serial.print("VOLUME UP");
            sendRemoteCommand(0x6DD2, 0x2);
          break;
          case BTN_VOL_DOWN:
            Serial.print("VOLUME DOWN");
            sendRemoteCommand(0x6DD2, 0x3);
          break;
          case BTN_MUTE:
            Serial.print("VOLUME MUTE");
            sendRemoteCommand(0x6DD2, 0x5);
          break;
        }
      }
      else
      {
        if (repeatsLeft > 0)
        {
          Serial.print("REPEAT...");
          //Repeat last command.
          sendRemoteRepeat();
          repeatsLeft--;
        }
        else
        {
          Serial.print("END");
          buttonPressed = false;
          repeatsLeft = 0;
          buttonCommand = 0;
        }
      }

      if (!repeatUntilRelease)
      {
        Serial.print(" and done.");
        buttonPressed = false;
        repeatsLeft = 0;
        buttonCommand = 0;
      }
      
      Serial.println("");     
    }

    timer++;   
    if (timer == 4000) timer = 0; 

}
