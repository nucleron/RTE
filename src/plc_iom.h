/*
 * Copyright Nucleron R&D LLC 2016
 *
 * This file is licensed under the terms of YAPL,
 * see License.txt for details.
 */

#ifndef _PLC_IOM_H_
#define _PLC_IOM_H_

typedef struct _plc_io_metods_t plc_io_metods_t;
struct _plc_io_metods_t
{
    void     (*init)   (void);                /* void     init    (void);                               */
    bool     (*test_hw)(void);                /* bool     test_hw (void);                               */
    bool     (*check)  (uint16_t);            /* bool     check   (uint16_t lid);                       */
    void     (*begin)  (uint16_t);            /* void     begin   (uint16_t lid);                       */
    void     (*end)    (uint16_t);            /* void     end     (uint16_t lid);                       */
    uint32_t (*sched)  (uint16_t, uint32_t);  /* uint32_t sched   (uint16_t lid, uint32_t tick);        */
    void     (*start)  (void);                /* bool     start   (void);                               */
    void     (*poll)   (uint32_t);            /* void     poll    (uint32_t tick);                      */
    void     (*stop)   (void);                /* bool     stop    (void);                               */
    uint32_t (*weigth) (uint16_t);            /* uint32_t weigth  (uint16_t lid);                       */
    uint32_t (*get)    (uint16_t);            /* uint32_t get     (uint16_t lid);                       */
    uint32_t (*set)    (uint16_t);            /* uint32_t set     (uint16_t lid);                       */
};

extern const plc_io_metods_t plc_iom_registry[];
extern const uint8_t plc_iom_reg_sz;

extern uint8_t mid_from_pid(uint16_t proto);
#define PLC_IOM_MID_ERROR (0xff)
// plc_iom_reg_sz must be declared after plc_iom_registry initiation
// as before initiation plc_iom_registry has inclomplete type
#define PLC_IOM_REG_SZ_DECL const uint8_t plc_iom_reg_sz = (uint8_t)(sizeof(plc_iom_registry)/sizeof(plc_io_metods_t))

#define PLC_IOM_CONCAT(a, b)       a##b
#define PLC_IOM_CONCAT2(a, b)      PLC_IOM_CONCAT(a, b)

#define PLC_IOM_RC_LINE(proto, method) .method = PLC_IOM_CONCAT2(proto, PLC_IOM_CONCAT(_, method))

#define PLC_IOM_RECORD(proto)        \
{                                    \
    PLC_IOM_RC_LINE(proto, init),     \
    PLC_IOM_RC_LINE(proto, test_hw),  \
    PLC_IOM_RC_LINE(proto, check),    \
    PLC_IOM_RC_LINE(proto, begin),    \
    PLC_IOM_RC_LINE(proto, end),      \
    PLC_IOM_RC_LINE(proto, sched),    \
    PLC_IOM_RC_LINE(proto, start),    \
    PLC_IOM_RC_LINE(proto, poll),     \
    PLC_IOM_RC_LINE(proto, stop),     \
    PLC_IOM_RC_LINE(proto, weigth),   \
    PLC_IOM_RC_LINE(proto, get),      \
    PLC_IOM_RC_LINE(proto, set),      \
}

#define PLC_IOM_LOCAL_INIT     PLC_IOM_CONCAT2(LOCAL_PROTO, _init)
#define PLC_IOM_LOCAL_TEST_HW  PLC_IOM_CONCAT2(LOCAL_PROTO, _test_hw)
#define PLC_IOM_LOCAL_CHECK    PLC_IOM_CONCAT2(LOCAL_PROTO, _check)
#define PLC_IOM_LOCAL_BEGIN    PLC_IOM_CONCAT2(LOCAL_PROTO, _begin)
#define PLC_IOM_LOCAL_END      PLC_IOM_CONCAT2(LOCAL_PROTO, _end)
#define PLC_IOM_LOCAL_SCHED    PLC_IOM_CONCAT2(LOCAL_PROTO, _sched)
#define PLC_IOM_LOCAL_START    PLC_IOM_CONCAT2(LOCAL_PROTO, _start)
#define PLC_IOM_LOCAL_POLL     PLC_IOM_CONCAT2(LOCAL_PROTO, _poll)
#define PLC_IOM_LOCAL_STOP     PLC_IOM_CONCAT2(LOCAL_PROTO, _stop)
#define PLC_IOM_LOCAL_WEIGTH   PLC_IOM_CONCAT2(LOCAL_PROTO, _weigth)
#define PLC_IOM_LOCAL_GET      PLC_IOM_CONCAT2(LOCAL_PROTO, _get)
#define PLC_IOM_LOCAL_SET      PLC_IOM_CONCAT2(LOCAL_PROTO, _set)

#define PLC_IOM_METH_DECLS(proto)                                             \
extern void     PLC_IOM_CONCAT2(proto, _init)   (void)                        ;\
extern bool     PLC_IOM_CONCAT2(proto, _test_hw)(void)                        ;\
extern bool     PLC_IOM_CONCAT2(proto, _check)  (uint16_t lid)                ;\
extern void     PLC_IOM_CONCAT2(proto, _begin)  (uint16_t lid)                ;\
extern void     PLC_IOM_CONCAT2(proto, _end)    (uint16_t lid)                ;\
extern uint32_t PLC_IOM_CONCAT2(proto, _sched)  (uint16_t lid, uint32_t tick) ;\
extern void     PLC_IOM_CONCAT2(proto, _start)  (void)                        ;\
extern void     PLC_IOM_CONCAT2(proto, _poll)   (uint32_t tick)               ;\
extern void     PLC_IOM_CONCAT2(proto, _stop)   (void)                        ;\
extern uint32_t PLC_IOM_CONCAT2(proto, _weigth) (uint16_t lid)                ;\
extern uint32_t PLC_IOM_CONCAT2(proto, _get)    (uint16_t lid)                ;\
extern uint32_t PLC_IOM_CONCAT2(proto, _set)    (uint16_t lid)

typedef struct _plc_iom_t plc_iom_t;

struct _plc_iom_t
{
    volatile uint32_t tick;    /*!<tick counter           */
    uint16_t m_begin;          /*!<Memory locations begin */
    uint16_t m_end;            /*!<Memory locations end   */
    volatile bool     tflg;    /*!<tick flag              */
};

extern plc_iom_t plc_iom; /*!<plc io manager*/

void plc_iom_init(void);
bool plc_iom_test_hw(void);
bool plc_iom_check_and_sort(void);

void plc_iom_tick(void);

void plc_iom_get(void);
void plc_iom_set(void);

void plc_iom_start(void);
void plc_iom_stop(void);
void plc_iom_poll(void);
void plc_iom_end(void);

void plc_iom_check_print(uint16_t i);
void plc_iom_errno_print(uint16_t errno);

extern const uint32_t plc_iom_err_sz_sz;
extern const char     plc_iom_err_sz[];

#define PLC_LOG_ERROR(a) plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)(a), sizeof(a))
#define PLC_LOG_WARN(a)  plc_curr_app->log_msg_post(LOG_WARNING,  (char *)(a), sizeof(a))
#define PLC_LOG_ERR_SZ() plc_curr_app->log_msg_post(LOG_CRITICAL, (char *)(plc_iom_err_sz), plc_iom_err_sz_sz)

#endif // _PLC_IOM_H_
