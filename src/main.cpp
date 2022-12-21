#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <setjmp.h>
jmp_buf f1buf;
jmp_buf f2buf;
jmp_buf f3buf;
void f1()
{
  Serial.println("f1");
  if (setjmp(f1buf))
  {
    Serial.println("setjmp f1");
    delay(3000);
    longjmp(f2buf, 1);
  }
}
void f2()
{
  Serial.println("f2");
  if (setjmp(f2buf))
  {
    Serial.println("setjmp f2");
    delay(3000);
    longjmp(f3buf, 1);
  }
}
void f3()
{
  Serial.println("f3");
  if (setjmp(f3buf))
  {
    Serial.println("setjmp f3");
    delay(3000);
    longjmp(f1buf, 1);
  }
}

void setup()
{
  Serial.begin(9600);
  f1();
  f2();
  f3();
}

void loop()
{
  longjmp(f1buf, 1);
}
