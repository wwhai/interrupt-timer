#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>

asm(".section .text\r\n"
    ".global AVR_RETI\r\n"
    "AVR_RETI:\r\n"
    "\t\tRETI\r\n");
//
extern "C" void AVR_RETI();
//
volatile bool scheduing = false;
//
void Task()
{
  Serial.println("DELAY 3000 BEGIN.");

  for (size_t i = 1000; i < 3000; i++)
  {
    Serial.print("[");
    Serial.print(i);
    Serial.print("] ");
  }
  Serial.println();
  Serial.println("DELAY 3000 END.");
}
//
//
//
ISR(TIMER1_COMPA_vect)
{
  Serial.println("**** AVR_RETI BEGIN.");
  AVR_RETI();
  Serial.println("**** AVR_RETI END.");
  if (!scheduing)
  {
    Serial.println("**** AVR_RETI scheduing.");
    scheduing = true;
    //-----------------------------------------
    Task();
    //-----------------------------------------
    scheduing = false;
  }
  Serial.println("**** AVR_RETI no scheduing.");
}
void setInterrupt()
{
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 65535; // 1秒
  TCCR1B = (1 << WGM12) | (1 << CS12);
  TIMSK1 = (1 << OCIE1A);
}
void setup()
{
  Serial.begin(9600);
  Serial.println("TIMER1 Setup BEGIN.");
  noInterrupts();
  setInterrupt();
  interrupts();
  Serial.println("TIMER1 Setup Finished.");
}
void loop()
{
}
