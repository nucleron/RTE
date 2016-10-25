/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of NOSL,
 * see License.txt for details.
 */

#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include <iec_std_lib.h>

#include <plc_app_default.h>
#include <plc_tick.h>
#include <plc_rtc.h>

#include <plc_config.h>

static unsigned long __tick = 0;
#define LOG_BUFFER_SIZE (1<<9) /*512bytes*/
//#define LOG_BUFFER_ATTRS __attribute__ ((section(".plc_log_buf_sec")))

static void PLC_GetTime( IEC_TIME *curent_time )
{
    curent_time->tv_nsec = 0;
    curent_time->tv_sec = 0;
}

static long long AtomicCompareExchange64(long long* atomicvar,long long compared, long long exchange)
{
    /* No need for real atomic op on LPC,
     * no possible preemption between debug and PLC */
    long long res = *atomicvar;
    if(res == compared)
    {
        *atomicvar = exchange;
    }
    return res;
}

#ifndef LOG_BUFFER_SIZE
#define LOG_BUFFER_SIZE (1<<14) /*16Ko*/
#endif
#ifndef LOG_BUFFER_ATTRS
#define LOG_BUFFER_ATTRS
#endif

#define LOG_BUFFER_MASK (LOG_BUFFER_SIZE-1)

static char LogBuff[LOG_LEVELS][LOG_BUFFER_SIZE] LOG_BUFFER_ATTRS;
void inline copy_to_log(uint8_t level, uint32_t buffpos, void* buf, uint32_t size)
{
    if(buffpos + size < LOG_BUFFER_SIZE)
    {
        memcpy(&LogBuff[level][buffpos], buf, size);
    }
    else
    {
        uint32_t remaining = LOG_BUFFER_SIZE - buffpos;
        memcpy(&LogBuff[level][buffpos], buf, remaining);
        memcpy(LogBuff[level], (char*)buf + remaining, size - remaining);
    }
}
void inline copy_from_log(uint8_t level, uint32_t buffpos, void* buf, uint32_t size)
{
    if(buffpos + size < LOG_BUFFER_SIZE)
    {
        memcpy(buf, &LogBuff[level][buffpos], size);
    }
    else
    {
        uint32_t remaining = LOG_BUFFER_SIZE - buffpos;
        memcpy(buf, &LogBuff[level][buffpos], remaining);
        memcpy((char*)buf + remaining, LogBuff[level], size - remaining);
    }
}

/* Log buffer structure

 |<-Tail1.msgsize->|<-sizeof(mTail)->|<--Tail2.msgsize-->|<-sizeof(mTail)->|...
 |  Message1 Body  |      Tail1      |   Message2 Body   |      Tail2      |

*/
typedef struct
{
    uint32_t msgidx;
    uint32_t msgsize;
    unsigned long tick;
    IEC_TIME time;
} mTail;

/* Log cursor : 64b
   |63 ... 32|31 ... 0|
   | Message | Buffer |
   | counter | Index  | */
static uint64_t LogCursor[LOG_LEVELS] LOG_BUFFER_ATTRS = {0x0,0x0,0x0,0x0};

static void ResetLogCount(void)
{
    uint8_t level;
    for(level=0; level<LOG_LEVELS; level++)
    {
        LogCursor[level] = 0;
    }
}

/* Store one log message of give size */
static int LogMessage(uint8_t level, char* buf, uint32_t size)
{
    if(size < LOG_BUFFER_SIZE - sizeof(mTail))
    {
        uint32_t buffpos;
        uint64_t new_cursor, old_cursor;

        mTail tail;
        tail.msgsize = size;
        tail.tick = __tick;
        PLC_GetTime(&tail.time);

        /* We cannot increment both msg index and string pointer
           in a single atomic operation but we can detect having been interrupted.
           So we can try with atomic compare and swap in a loop until operation
           succeeds non interrupted */
        do
        {
            old_cursor = LogCursor[level];
            buffpos = (uint32_t)old_cursor;
            tail.msgidx = (old_cursor >> 32);
            new_cursor = ((uint64_t)(tail.msgidx + 1)<<32)
                         | (uint64_t)((buffpos + size + sizeof(mTail)) & LOG_BUFFER_MASK);
        }
        while(AtomicCompareExchange64(
                    (long long*)&LogCursor[level],
                    (long long)old_cursor,
                    (long long)new_cursor)!=(long long)old_cursor);

        copy_to_log(level, buffpos, buf, size);
        copy_to_log(level, (buffpos + size) & LOG_BUFFER_MASK, &tail, sizeof(mTail));

        return 1; /* Success */
    }
    else
    {
        char mstr[] = "Logging error : message too big";
        LogMessage(LOG_CRITICAL, mstr, sizeof(mstr));
    }
    return 0;
}

static uint32_t GetLogCount(uint8_t level)
{
    return (uint64_t)LogCursor[level] >> 32;
}

/* Return message size and content */
static uint32_t GetLogMessage(uint8_t level, uint32_t msgidx, char* buf, uint32_t max_size, uint32_t* tick, uint32_t* tv_sec, uint32_t* tv_nsec)
{
    uint64_t cursor = LogCursor[level];
    if(cursor)
    {
        /* seach cursor */
        uint32_t stailpos = (uint32_t)cursor;
        uint32_t smsgidx;
        mTail tail;
        tail.msgidx = cursor >> 32;
        tail.msgsize = 0;

        /* Message search loop */
        do
        {
            smsgidx = tail.msgidx;
            stailpos = (stailpos - sizeof(mTail) - tail.msgsize ) & LOG_BUFFER_MASK;
            copy_from_log(level, stailpos, &tail, sizeof(mTail));
        }
        while((tail.msgidx == smsgidx - 1) && (tail.msgidx > msgidx));

        if(tail.msgidx == msgidx)
        {
            uint32_t sbuffpos = (stailpos - tail.msgsize ) & LOG_BUFFER_MASK;
            uint32_t totalsize = tail.msgsize;
            *tick = tail.tick;
            *tv_sec = tail.time.tv_sec;
            *tv_nsec = tail.time.tv_nsec;
            copy_from_log(level, sbuffpos, buf,
                          totalsize > max_size ? max_size : totalsize);
            return totalsize;
        }
    }
    return 0;
}

void plc_app_default_init(void)
{
    int i;
    for(i=0; i<LOG_LEVELS; i++)
    {
        LogCursor[i] = 0;
        memset(&LogBuff[i][0], 0, LOG_BUFFER_SIZE);
    }
}

static int startPLC(int argc,char **argv)
{
    (void)argc;
    (void)argv;

    plc_tick_setup( 0, 1000000ull );

    return 0;
}
static int stopPLC()
{
    return 0;
}
static void runPLC(void)
{
    return;
}

static void resumeDebug(void)
{
    return;
}
static void suspendDebug(int disable)
{
    return;
}

static void FreeDebugData(void)
{
    return;
}

const uint32_t default_len = 0;
static int GetDebugData(unsigned long *tick, unsigned long *size, void **buffer)
{
    *tick = 0;
    *size = 0;
    *buffer = (void *)&default_len;
    return 0;
}
static void ResetDebugVariables(void)
{
    return;
}
static void RegisterDebugVariable(int idx, void* force)
{
    (void)idx;
    (void)force;
    return;
}

const plc_app_abi_t plc_app_default =
{
    .id   = "DEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEFDEADBEEF",

    .start = startPLC,
    .stop  = stopPLC,
    .run   = runPLC,

    .dbg_resume    = resumeDebug,
    .dbg_suspend   = suspendDebug,

    .dbg_data_get  = GetDebugData,
    .dbg_data_free = FreeDebugData,

    .dbg_vars_reset   = ResetDebugVariables,
    .dbg_var_register = RegisterDebugVariable,

    .log_cnt_get   = GetLogCount,
    .log_msg_get   = GetLogMessage,
    .log_cnt_reset = ResetLogCount,
    .log_msg_post  = LogMessage
};
