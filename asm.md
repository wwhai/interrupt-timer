```cpp
#include <Arduino.h>

volatile bool shouldExecuteFunction = false;
volatile uint16_t savedSP;
volatile uint8_t savedSREG;

void myFunction() {
  // 这是你的函数代码
  Serial.println("My Function");
}

void setup() {
  Serial.begin(9600);

  // 设置定时器中断，每2秒触发一次
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1 = 0;
  OCR1A = 31250;  // 2秒钟的定时
  TCCR1B |= (1 << WGM12);
  TCCR1B |= (1 << CS12) | (1 << CS10);  // 设置预分频器为 1024
  TIMSK1 |= (1 << OCIE1A);  // 开启定时器中断
  interrupts();  // 启用中断
}

void loop() {
  // 主循环中的代码
  if (shouldExecuteFunction) {
    // 保存函数上下文
    asm volatile(
        "push r0\n\t"
        "in r0, 0x3F\n\t"
        "cli\n\t"
        "push r0\n\t"
        "push r1\n\t"
        "clr r1\n\t"
        "push r2\n\t"
        "push r3\n\t"
        "push r4\n\t"
        "push r5\n\t"
        "push r6\n\t"
        "push r7\n\t"
        "push r8\n\t"
        "push r9\n\t"
        "push r10\n\t"
        "push r11\n\t"
        "push r12\n\t"
        "push r13\n\t"
        "push r14\n\t"
        "push r15\n\t"
        "push r16\n\t"
        "push r17\n\t"
        "push r28\n\t"
        "push r29\n\t"
        "in %A0, 0x3D\n\t"
        "in %B0, 0x3E\n\t"
        : "=r" (savedSP)
    );
    savedSREG = SREG;
    // 执行函数
    myFunction();
    // 恢复函数上下文
    asm volatile(
        "out 0x3D, %A0\n\t"
        "out 0x3E, %B0\n\t"
        : : "r" (savedSP)
    );
    SREG = savedSREG;
    asm volatile(
        "pop r29\n\t"
        "pop r28\n\t"
        "pop r17\n\t"
        "pop r16\n\t"
        "pop r15\n\t"
        "pop r14\n\t"
        "pop r13\n\t"
        "pop r12\n\t"
        "pop r11\n\t"
        "pop r10\n\t"
        "pop r9\n\t"
        "pop r8\n\t"
        "pop r7\n\t"
        "pop r6\n\t"
        "pop r5\n\t"
        "pop r4\n\t"
        "pop r3\n\t"
        "pop r2\n\t"
        "pop r1\n\t"
        "pop r0\n\t"
        "out 0x3F, r0\n\t"
        "pop r0\n\t"
    );
    shouldExecuteFunction = false;
  }
}

// 定时器中断服务程序
ISR(TIMER1_COMPA_vect) {
  shouldExecuteFunction = true;
}


```