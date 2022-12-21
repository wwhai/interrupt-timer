#include <Arduino.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <setjmp.h>
//
#define ALWAYS_INLINE static inline __attribute__((always_inline))
//
volatile bool scheduing = false;
//
#define MAX_TASKS 3        // 2 tasks
#define MAX_TASK_SLICE 100 // 100ms
#define NEW_TASKS(TASKS...) \
  MicroTask MicroTasks[] = {TASKS};
static jmp_buf os_context;
// _JBLEN  23
typedef unsigned char Byte;
typedef struct
{
  Byte r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15, r16, r17; // call-saved registers
  Byte r28, r29;                                                               // frame pointer
  Byte SP_H, SP_L;                                                             // stack pointer
  Byte STATUS;                                                                 // status register
  Byte RETURN_ADDR;                                                            // return address (PC)
} CPU_STATE;

//
enum MicroTaskState
{
  RUNNING = 0, // 正在运行
  READY,       // 准备好了
  BLOCK,       // 被挂起来
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
//-----------------------------
// 实现主进程任务的函数
//-----------------------------
void f0(void *args)
{
  Serial.println("##[Task0]## Task0 BEGIN");
}

// 实现任务的函数
void f1(void *args)
{
  int acc = 0;
  while (true)
  {
    delay(100);
    Serial.print("[");
    Serial.print(acc++);
    Serial.print("]");
    Serial.print("●\r\n");
  }
}
void f2(void *args)
{
  for (int i = 0; i <= 1000; i++)
  {
    {
      delay(20);
      Serial.print("[");
      Serial.print(i);
      Serial.print("]");
      Serial.print("◆\r\n");
    }
  }
}
// 当前任务表数量
static MicroTask currentTask;
/// @brief 系统进程表
/*----------------*/ NEW_TASKS(NewTask(f0), NewTask(f1), NewTask(f2)); /*----------------*/
uint8_t valid_count = sizeof(MicroTasks) / sizeof(MicroTask);
// 保存上下文
void TaskYield(void)
{
  // 如果是第一次表示此时该上下文被保存
  // AddTaskCtx(currentTask.stack)
  if (!setjmp(currentTask.stack)) // 可以理解为第一次加载任务到Task调度器中
  {
    longjmp(os_context, 1); // 加载任务以后, 控制权交给Task管理器
  }
}

asm(".section .text\r\n"
    ".global AVR_RETI\r\n"
    "AVR_RETI:\r\n"
    "\t\tRETI\r\n");
// 这个函数的神奇之处就是打破了C语言的函数堆栈，牺牲一个空函数换来任务函数的堆栈
// 也是实时调度的核心。
extern "C" void AVR_RETI();
/// 中断入口
/*----------------*/ ISR(TIMER1_COMPA_vect) /*----------------*/
{
  if (currentTask.id < 0) // 表示没有任务
  {
    return;
  }
  AVR_RETI();
  Serial.print("# ISR TIMER1 #");
  //-----------------------------------------
  for (size_t i = 0; i < MAX_TASKS; i++)
  {
    if (MicroTasks[i].valid)
    {
      if (MicroTasks[i].time_slice > 0) // 还有时间片
      {
        if (MicroTasks[i].state == RUNNING)
        {
          MicroTasks[i].time_slice -= 10; // 时间片一直减少
        }
      }
      else // 时间片用完了
      {
        if (!scheduing) // 防止重复调度
        {
          Serial.println("**** AVR_RETI scheduing.");
          MicroTasks[i].time_slice = MAX_TASK_SLICE;
          MicroTasks[i].state = READY;
          scheduing = true; // 开始调度
          TaskYield();
        }
      }
    }
  }
}

/// @brief 运行任务
/// @param task
/// @param args
void RunTask()
{

  for (size_t i = 0; i < MAX_TASKS; i++)
  {

    if (setjmp(os_context)) // 理解为加载了个进程进来进行调度
    {
      scheduing = false;
      continue; // TODO 问题出在这里了， 导致上下文没有恢复成功
    }
    if (MicroTasks[i].valid)
    {
      switch (MicroTasks[i].state)
      {
      case READY:
      {
        Serial.print("Current RunTask... BEGIN:");
        Serial.print("[");
        Serial.print(i);
        Serial.print("]\n\r");
        currentTask.id = i;            // 标记当前执行的是哪个函数
        MicroTasks[i].state = RUNNING; // 开始执行
        MicroTasks[i].func(0);         // 执行
        MicroTasks[i].state = STOP;    // 执行结束
        MicroTasks[i].valid = false;   // 执行完了就把任务给清了
        valid_count--;
        Serial.print("Current RunTask... END:");
        Serial.print("[");
        Serial.print(i);
        Serial.print("]\n\r");
      }
      break;
      case RUNNING:
        longjmp(MicroTasks[i].stack, 1);
        break;
      case BLOCK:
        MicroTasks[i].time--;
        if (MicroTasks[i].time == 0)
        {
          if (MicroTasks[i].valid)
          {
            MicroTasks[i].state = READY;
            longjmp(MicroTasks[i].stack, 1);
          }
        }
        break;
      case STOP:
        MicroTasks[i].valid = false;
        break;
      default:
        TaskYield();
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
  currentTask.priority = 0; // 优先级0
  currentTask.id = -1;      // 当前任务未被调用
}
void loop()
{
  Serial.println("<--[New CPU interval]-->");
  RunTask();
}
