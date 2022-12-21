#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <setjmp.h>
//
#define ALWAYS_INLINE static inline __attribute__((always_inline))
//
volatile bool scheduing = false;
//
#define MAX_TASKS 2        // 2 tasks
#define MAX_TASK_SLICE 100 // 100ms
#define NEW_TASKS(N, TASKS...) \
  MicroTask MicroTasks[N] = {TASKS};

// _JBLEN  23
typedef unsigned char Byte;
typedef struct CPU_STATE
{
  Byte r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17; // call-saved registers
  Byte r28, r29;                                                               // frame pointer
  Byte SP_H, SP_L;                                                             // stack pointer
  Byte STATUS;                                                                 // status register
  Byte RETURN_ADDR;                                                            // return address (PC)
};

//
enum MicroTaskState
{
  RUNNING = 0, // 正在运行
  READY,       // 准备好了
  BLOCK,       // 超时被挂起来
  STOP         // 已经结束
};
// 任务函数
typedef void (*MicroTaskFunc)(void *args);
// 任务
typedef struct MicroTask
{
  uint8_t id;                    // ID
  volatile uint8_t valid;        // 是否是有效任务
  volatile MicroTaskState state; // 状态 see@MicroTaskState
  uint8_t priority;              // 任务的优先级 0-16
  jmp_buf stack;                 // 栈空间，大小是24个字节!
  volatile uint8_t time_slice;   // 分配给任务的时间片，默认一个任务时间片为100毫秒
  volatile uint64_t time;        // 假设给sleep了，这里表示sleep的时间
  uint8_t switched;              // 是否已经被保存了上下文
  MicroTaskFunc func;            // 任务执行函数

} MicroTask;
/// @brief 每个函数的堆栈
jmp_buf stacks[MAX_TASKS];
/// @brief 当前执行的任务
static MicroTask currentTask;
//
// 系统进程表
//
static uint8_t TaskId = 0;
MicroTask NewTask(MicroTaskFunc func)
{
  MicroTask task;
  task.id = TaskId++;
  task.valid = true;
  task.state = READY;
  task.time_slice = MAX_TASK_SLICE;
  task.switched = false;
  task.func = func;
  return task;
}

// 实现主进程任务的函数
void f0(void *args)
{
  Serial.println("##[OS]## MainTask");
}
static MicroTask MainTask = NewTask(f0);

// 实现任务的函数
void f1(void *args)
{
  Serial.println("##[TASK]## Task0");
}
void f2(void *args)
{
  Serial.println("##[TASK]## Task1");
}
// 当前任务表数量
uint8_t valid_count = 3;
/// @brief 系统进程表
NEW_TASKS(MAX_TASKS, NewTask(f1), NewTask(f2));
// 保存上下文
void TaskYield()
{
  // 先切到0号进程
  longjmp(MainTask.stack, 1);
  // 然后再从0号进程跳转到目标
  // longjmp(Next.stack, 1);
}

asm(".section .text\r\n"
    ".global AVR_RETI\r\n"
    "AVR_RETI:\r\n"
    "\t\tRETI\r\n");
// 这个函数的神奇之处就是打破了C语言的函数堆栈，牺牲一个空函数换来任务函数的堆栈
// 也是实时调度的核心。
extern "C" void AVR_RETI();
/// 中断入口

ISR(TIMER1_COMPA_vect)
{
  Serial.println("#ISR# AVR_RETI BEFORE");
  AVR_RETI();
  Serial.println("#ISR# AVR_RETI AFTER");
  for (size_t i = 0; i < MAX_TASKS; i++)
  {
    if (MicroTasks[i].valid)
    {
      if (MicroTasks[i].time_slice > 0) // 还有时间片
      {
        if (MicroTasks[i].state == RUNNING)
        {
          Serial.print("#ISR# Task time slice DERCEASE");
          Serial.print("[");
          Serial.print(i);
          Serial.print("] => ");
          Serial.println(MicroTasks[i].time_slice);
          MicroTasks[i].time_slice -= 10; // 时间片一直减少
        }
      }
      else // 时间片用完了
      {
        Serial.print("#ISR# Task time slice approve:");
        Serial.print("[");
        Serial.print(i);
        Serial.print("]\n\r");
        TaskYield();
      }
    }
  }
}
/// @brief 重置任务
/// @param task
void ResetTaskTimeSlice(MicroTask *task)
{
  task->time_slice = MAX_TASK_SLICE; // 重新设置时间片
  task->state = READY;               // 重新设置状态为就绪
}
/// @brief 运行任务
/// @param task
/// @param args
void RunTask()
{
  if (valid_count == 1)
  {
    MainTask.func(0);
    return;
  }
  for (size_t i = 0; i < MAX_TASKS; i++)
  {

    if (MicroTasks[i].valid)
    {
      switch (MicroTasks[i].state)
      {
      case READY:
      {
        Serial.print("Current RunTask BEGIN:");
        Serial.print("[");
        Serial.print(i);
        Serial.print("]\n\r");
        MicroTasks[i].state = RUNNING;                // 开始执行
        MicroTasks[i].func((void *)MicroTasks[i].id); // 执行
        MicroTasks[i].state = STOP;                   // 执行结束
        MicroTasks[i].valid = false;                  // 执行完了就把任务给清了
        valid_count--;
        Serial.print("Current RunTask END:");
        Serial.print("[");
        Serial.print(i);
        Serial.print("]\n\r");
      }
      break;
      case RUNNING:
        break;
      case BLOCK:
        MicroTasks[i].time--;
        if (MicroTasks[i].time == 0)
        {
          if (MicroTasks[i].valid)
          {
            MicroTasks[i].state = READY;
          }
        }
        break;
      case STOP:
        MicroTasks[i].valid = false;
        break;
      default:
        break;
      }
    }
  }
}

void setup()
{
  Serial.begin(9600);
  Serial.println("TIMER1 Setup BEGIN.");
  cli();
  TCCR1A = 0;
  TCCR1B = 0;
  OCR1A = 0xF424; // 1秒
  TCCR1B = (1 << WGM12) | (1 << CS12);
  TIMSK1 = (1 << OCIE1A);
  sei();
  Serial.println("TIMER1 Setup Finished.");
  MainTask.priority = 0;
  setjmp(MainTask.stack); // 初始化到主进程中
}
void loop()
{
  delay(5000); // 频率放慢一点有利于观察输出
  Serial.println("<--[New CPU interval]-->");
  RunTask();
}
