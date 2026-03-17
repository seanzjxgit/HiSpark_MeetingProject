#ifndef RT_CONFIG_H__
#define RT_CONFIG_H__

/* RT-Thread Configuration */

/* RT-Thread Kernel */

#define RT_NAME_MAX 20
#define RT_CPUS_NR 1
#define RT_ALIGN_SIZE 8
#define RT_THREAD_PRIORITY_MAX 32
#define RT_TICK_PER_SECOND 100
#define RT_USING_OVERFLOW_CHECK
#define RT_USING_HOOK
#define RT_HOOK_USING_FUNC_PTR
#define RT_USING_IDLE_HOOK
#define RT_IDLE_HOOK_LIST_SIZE 4
#define IDLE_THREAD_STACK_SIZE 256

/* kservice optimization */

#define RT_USING_DEBUG
#define RT_DEBUGING_COLOR
#define RT_DEBUGING_CONTEXT

/* Inter-Thread communication */

#define RT_USING_SEMAPHORE
#define RT_USING_MUTEX
#define RT_USING_EVENT
#define RT_USING_MAILBOX
#define RT_USING_MESSAGEQUEUE

/* Memory Management */

#define RT_USING_MEMHEAP
#define RT_USING_MEMPOOL
#define RT_USING_SMALL_MEM
#define RT_USING_SMALL_MEM_AS_HEAP
#define RT_USING_HEAP
#define RT_USING_DEVICE
#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE 128
#define RT_BACKTRACE_LEVEL_MAX_NR 32
#define RT_USING_HW_ATOMIC
#define RT_USING_TINY_FFS
#define RT_VER_NUM 0x50100

/* DFS: device virtual file system */

#define RT_USING_DFS

/* Device Drivers */

#define RT_UNAMED_PIPE_NUMBER 64
#define RT_USING_SERIAL
#define RT_SERIAL_USING_DMA
#define RT_SERIAL_RB_BUFSZ 64
#define RT_USING_PIN

/* Network */

#define IP_SOF_BROADCAST 1
#define IP_SOF_BROADCAST_RECV 1

/* rt_vsnprintf options */
 
#define RT_KLIBC_USING_LIBC_VSNPRINTF
 
/* rt_vsscanf options */
 
#define RT_KLIBC_USING_LIBC_VSSCANF

/* Utilities */
#define LOG_TAG              "example"
#define RT_USING_ULOG
#define ULOG_OUTPUT_LVL 7

#endif
