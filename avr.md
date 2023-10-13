```cpp
#include <avr/io.h>
#include <avr/interrupt.h>

// 声明一个结构体用于保存函数上下文
typedef struct {
  uint8_t r0;
  uint8_t r1;
  uint8_t r2;
  uint8_t r3;
  uint8_t r4;
  uint8_t r5;
  uint8_t r6;
  uint8_t r7;
  uint8_t r8;
  uint8_t r9;
  uint8_t r10;
  uint8_t r11;
  uint8_t r12;
  uint8_t r13;
  uint8_t r14;
  uint8_t r15;
  uint8_t r16;
  uint8_t r17;
  uint8_t r18;
  uint8_t r19;
  uint8_t r20;
  uint8_t r21;
  uint8_t r22;
  uint8_t r23;
  uint8_t r24;
  uint8_t r25;
  uint8_t r26;
  uint8_t r27;
  uint8_t r28;
  uint8_t r29;
  uint8_t r30;
  uint8_t r31;
  uint8_t sreg;
} Context;

Context savedContext;

// 中断服务程序
ISR(TIMER1_COMPA_vect) {
  // 保存上下文
  asm volatile (
    "push r0\n"
    "push r1\n"
    "push r2\n"
    "push r3\n"
    "push r4\n"
    "push r5\n"
    "push r6\n"
    "push r7\n"
    "push r8\n"
    "push r9\n"
    "push r10\n"
    "push r11\n"
    "push r12\n"
    "push r13\n"
    "push r14\n"
    "push r15\n"
    "push r16\n"
    "push r17\n"
    "push r18\n"
    "push r19\n"
    "push r20\n"
    "push r21\n"
    "push r22\n"
    "push r23\n"
    "push r24\n"
    "push r25\n"
    "push r26\n"
    "push r27\n"
    "push r28\n"
    "push r29\n"
    "push r30\n"
    "push r31\n"
    "in %0, __SREG__\n"
    "cli\n"
    : "=r" (savedContext.sreg)
  );

  // 执行中断处理操作
  // ...

  // 恢复上下文
  asm volatile (
    "out __SREG__, %0\n"
    "pop r31\n"
    "pop r30\n"
    "pop r29\n"
    "pop r28\n"
    "pop r27\n"
    "pop r26\n"
    "pop r25\n"
    "pop r24\n"
    "pop r23\n"
    "pop r22\n"
    "pop r21\n"
    "pop r20\n"
    "pop r19\n"
    "pop r18\n"
    "pop r17\n"
    "pop r16\n"
    "pop r15\n"
    "pop r14\n"
    "pop r13\n"
    "pop r12\n"
    "pop r11\n"
    "pop r10\n"
    "pop r9\n"
    "pop r8\n"
    "pop r7\n"
    "pop r6\n"
    "pop r5\n"
    "pop r4\n"
    "pop r3\n"
    "pop r2\n"
    "pop r1\n"
    "pop r0\n"
  : : "r" (savedContext.sreg)
  );
}

int main() {
  // 初始化定时器
  // ...

  // 启用中断
  sei();

  while (1) {
    // 主程序循环
    // ...
  }

  return 0;
}

```