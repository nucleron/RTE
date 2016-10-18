#ifndef _PLC_HMI_H_
#define _PLC_HMI_H_

#define HMI_DIGITS   6
#define HMI_NBUTTONS 3

typedef struct
{
    uint8_t shift;
    uint8_t msk;
}
plc_hmi_led_rec;

typedef enum
{
    PLC_HMI_NOT_USED = 0,
    PLC_HMI_BOOL_OO,
    PLC_HMI_BOOL_TF,
    PLC_HMI_HEX,
    PLC_HMI_UINT,
    PLC_HMI_MMDD,
    PLC_HMI_HHMM,
    PLC_HMI_RO_START,  //Read only types start
    PLC_HMI_RO_BOOL_OO,
    PLC_HMI_RO_BOOL_TF,
    PLC_HMI_RO_HEX,
    PLC_HMI_RO_UINT,
    PLC_HMI_RO_MMDD,
    PLC_HMI_RO_HHMM,
    PLC_HMI_RO_END
}plc_hmi_par_t; //HMI parameter types

typedef struct _plc_hmi_t plc_hmi_t;//HMI
typedef struct _plc_hmi_dm_t plc_hmi_dm_t;//Data model (MVC pattern is used)

struct _plc_hmi_dm_t
{
    uint32_t leds;                              //Led satate
    plc_hmi_par_t * ptype;
    uint16_t (*par_get)(uint8_t);           //parameter get
    uint16_t (*par_chk)(uint8_t, uint16_t); //parameter check
    void     (*par_set)(uint8_t, uint16_t); //parameter set
    uint16_t (*poll)(void);              //Poll hook
    bool psize;
};

struct _plc_hmi_t
{
    uint8_t cur_par; //Current param;
    uint8_t state;
    uint16_t tmp;
    const plc_hmi_dm_t * mdl; //Current data mode
    uint8_t cursor;
    char buf[HMI_DIGITS];
    bool cur_show;
};

#define PLC_HMI_STATE_VIEW 0
#define PLC_HMI_STATE_EDIT 1

void plc_hmi_vout_poll(void);

#endif // _PLC_HMI_H_
