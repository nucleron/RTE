#ifndef _PLC_AOUT_H_
#define _PLC_AOUT_H_

#define PLC_DAC_SYNC_MSK    0x0060

extern uint16_t plc_aout_dataA;
extern uint16_t plc_aout_dataB;

typedef union
{
    uint32_t reg;
    uint16_t val[2];
}aout_clb_t;

void _plc_aout_dac_poll(void);

#endif /* _PLC_AOUT_H_ */
