#ifndef _PLC_HMI_H_
#define _PLC_HMI_H_

#define PLC_HMI_BRI_LIM 6
#define HMI_DIGITS   6
#define HMI_NBUTTONS 3

typedef struct
{
    uint32_t shift;
    uint32_t msk;
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
    PLC_HMI_EMPTY,    //Yes, empty is read only!
    PLC_HMI_END
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
    char     (*poll)(uint32_t, char);       //Poll hook
    uint8_t  psize;
};

struct _plc_hmi_t
{
    plc_hmi_dm_t * mdl; //Current data model
    uint32_t tmp;             //Temp var for edit mode
    uint32_t delta;           //Delta in edit mode
    uint8_t cursor;           //Cursor in edit mode
    uint8_t cur_par;          //Current param;
    uint8_t state;            //HMI state
    char buf[HMI_DIGITS];
    bool cur_show;
};

#define PLC_HMI_STATE_VIEW 0
#define PLC_HMI_STATE_EDIT 1

void plc_hmi_kb_init(void);
char plc_hmi_kb_poll(uint32_t tick);

void plc_hmi_vout_init(void);
void plc_hmi_vout_poll(void);

void _plc_hmi_init(void);
void _plc_hmi_poll(uint32_t tick);

#endif // _PLC_HMI_H_
