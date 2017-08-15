/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#ifndef _PLC_DBG_H_
#define _PLC_DBG_H_

//#include <iec_std_lib.h>
#include <iec_types_all.h>
#include <plc_abi.h>
#include <plc_iom.h>

#define DBG_FIFO_SIZE 128

typedef struct
{
    volatile unsigned short bytes;
    volatile unsigned short read;
    volatile unsigned short write;
    volatile unsigned char buf[DBG_FIFO_SIZE];
} dbg_fifo_t;

void     dbg_fifo_flush(dbg_fifo_t * fifo);
int dbg_fifo_write_byte(dbg_fifo_t * fifo, unsigned char d);
int      dbg_fifo_write(dbg_fifo_t * fifo, unsigned char *d, unsigned short n);
int  dbg_fifo_read_byte(dbg_fifo_t * fifo, unsigned char *d);
int       dbg_fifo_read(dbg_fifo_t * fifo, unsigned char *d, unsigned short n);


//Debug serial port access functions
extern int dbg_serial_write(unsigned char *d, unsigned short n);
extern int dbg_serial_read(unsigned char *d, unsigned short n);
extern void dbg_serial_init(void);

//Debug protocol handler
//States
#define GET_CMD          0
#define PUT_ACK          19
#define PARSE_CMD        20

#define GET_DATA_LEN     1
#define GET_VAR_IDX      2
#define GET_FORCE_LEN    3
#define GET_FORCE_DATA   4

#define GET_MSG_LEN      5
#define GET_MSG_LEVEL    6
#define GET_MSG_ID       7
#define PUT_MSG_LEN      8
#define PUT_MSG_TICK     9
#define PUT_MSG_SEC      10
#define PUT_MSG_NSEC     11

#define GET_DEBUG_DATA   13
#define PUT_DEBUG_LEN    14
#define PUT_DEBUG_TICK   15

#define PUT_ID_LEN       16

#define PUT_LC_LEN       17

#define PUT_DATA         18

#define GET_RTC_DATA     21

//Commands
#define DBG_CMD_START    0x61
#define DBG_CMD_STOP     0x62
#define DBG_CMD_RESET    0x63

#define DBG_CMD_SET_V    0x64
#define DBG_CMD_GET_V    0x65

#define DBG_CMD_GET_ID   0x66

#define DBG_CMD_GET_LC    0x67
#define DBG_CMD_GET_LM    0x68
#define DBG_CMD_CLR_LM    0x69

#define DBG_CMD_IDLE      0x6a

#define DBG_CMD_SET_RTC   0x6b

//Timer threshold
#define PLC_DBG_TIMER_THR 500

typedef union
{
    unsigned long long align_header; //Align for biggest type
    unsigned long log_cnt[4];
    char log_msg[512];
    unsigned char data[512];
} tr_buf_t;

typedef struct
{
} get_id_t;

typedef struct
{
    int var_idx;
    unsigned char * var_idx_ptr;
} set_val_t;

typedef struct
{
    unsigned long tick;
} get_val_t;

typedef struct
{
    unsigned char level;
    unsigned long msg_id;
    unsigned long tick;
    unsigned long sec;
    unsigned long nsec;
} get_lm_t;

typedef struct
{
    unsigned char year;
    unsigned char mon;
    unsigned char day;
    unsigned char hour;
    unsigned char min;
    unsigned char sec;
} set_rtc_t;

typedef union
{
    get_id_t   get_id;
    set_val_t  set_val;
    get_val_t  get_val;
    get_lm_t   get_log_msg;
    set_rtc_t  set_rtc;
} tr_vars_t;

typedef struct
{
    int state;
    uint32_t timer;
    unsigned char cmd;

    unsigned char tmp_len;
    unsigned char * tmp;

    int data_len;
    unsigned char * data;

    void (*data_hook)(void);

    tr_vars_t tr;
    tr_buf_t tr_buf;
} plc_dbg_proto_t;

#define PLC_STATE_STOPED 0x55
#define PLC_STATE_STARTED 0xAA

//extern void plc_boot_mode_enter(void);
extern uint8_t plc_state;
extern bool plc_dbg_mode;

static inline void plc_app_start(void)
{
    plc_curr_app->start(0, 0);
    plc_iom_start();
    plc_state = PLC_STATE_STARTED;
}

static inline void plc_app_stop(void)
{
    plc_state = PLC_STATE_STOPED;
    plc_iom_stop();
    plc_curr_app->stop();
}

extern void dbg_init(void);
extern void dbg_handler(void);
#endif // _PLC_DBG_H_
