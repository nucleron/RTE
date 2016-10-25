#ifndef _PLC_AOUT_H_
#define _PLC_AOUT_H_

#define PLC_DAC_SYNC_MSK    0x0060

// данные для DAC, 0...4095
uint16_t plc_aout_dataA;
uint16_t plc_aout_dataB;

void _plc_aout_init(void);
void _plc_aout_dac_poll(void);

#endif /* _PLC_AOUT_H_ */
