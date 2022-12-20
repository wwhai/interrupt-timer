#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <setjmp.h>
//
#define ALWAYS_INLINE static inline __attribute__((always_inline))
//
#define MAX_TASKS 2        // 2 tasks
#define MAX_TASK_SLICE 100 // 100ms
#define NEW_TASKS(N, TASKS...) MicroTask MicroTasks[N] = {TASKS};
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
  jmp_buf stack;                 // 栈空间
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
  task.valid = 1;
  task.state = READY;
  task.time_slice = MAX_TASK_SLICE;
  task.switched = 0;
  task.func = func;
  return task;
}
//
CPU_STATE s;
// 实现主进程任务的函数
void f0(void *args)
{
}
static MicroTask MainTask = NewTask(f0);
// 实现任务的函数

void f1(void *args)
{
  setjmp(MainTask.stack);
  memcpy(&s, MainTask.stack, sizeof(CPU_STATE));
  Serial.print("#F1# RETURN_ADDR: ");
  Serial.println(s.RETURN_ADDR);
  Serial.println("function-1 SP:");
  Serial.print(SP);
  Serial.println("function-1 be called");
  Serial.println("function-1 sleep 5000 BEGIN");
  int t = 0;
  while (t <= 10)
  {
    delay(1000);
    t += 1;
    Serial.print("function-0 sleep:");
    Serial.println(t);
  }

  Serial.println("function-0 sleep 5000 END");
}
void f2(void *args)
{
  setjmp(MainTask.stack);
  memcpy(&s, MainTask.stack, sizeof(CPU_STATE));
  Serial.print("#F2# RETURN_ADDR: ");
  Serial.println(s.RETURN_ADDR);
  Serial.println("function-2 SP:");
  Serial.print(SP);
  Serial.println("function-2 be called");
}
/// @brief 系统进程表
NEW_TASKS(MAX_TASKS, NewTask(f1), NewTask(f2));
// 保存上下文
void SaveCtx(MicroTask *task)
{
  Serial.print("SaveCtx:");
  Serial.println(task->id);
}
// 移动IP
void MoveIPToNext(MicroTask *task)
{
  Serial.println("MoveIPToNext");
}

// 中断计时器
ISR(TIMER1_COMPA_vect)
{
  setjmp(MainTask.stack);
  memcpy(&s, MainTask.stack, sizeof(CPU_STATE));
  Serial.print("#ISR# RETURN_ADDR: ");
  Serial.println(s.RETURN_ADDR);
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
          MicroTasks[i].time_slice -= 50; // 时间片一直减少
        }
      }
      else // 时间片用完了
      {
        Serial.print("#ISR# Task time slice approve:");
        Serial.print("[");
        Serial.print(i);
        Serial.print("]\n\r");
        SaveCtx(&MicroTasks[i]);
        MoveIPToNext(&MicroTasks[i]);
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
  for (size_t i = 0; i < MAX_TASKS; i++)
  {

    if (MicroTasks[i].valid)
    {
      switch (MicroTasks[i].state)
      {
      case READY:

        break;
      case RUNNING:

        break;
      case BLOCK:

        break;
      case STOP:

        break;

      default:
        break;
      }
      if (MicroTasks[i].state == READY)
      {
        Serial.print("Current RunTask BEGIN:");
        Serial.print("[");
        Serial.print(i);
        Serial.print("]\n\r");
        MicroTasks[i].state = RUNNING; // 开始执行
        MicroTasks[i].func(0);         // 执行
        MicroTasks[i].state = STOP;    // 执行结束
        MicroTasks[i].valid = 0;       // 执行完了就把任务给清了
        Serial.print("Current RunTask END:");
        Serial.print("[");
        Serial.print(i);
        Serial.print("]\n\r");
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
}
void loop()
{
  delay(5000);
  Serial.println("loop SP:");
  Serial.print(SP);
  Serial.println("<--[New CPU interval]->");
  // RunTask();
  f1(0);
  f2(0);
}
