#ifndef _PLC_DIAG_H_
#define _PLC_DIAG_H_

#define PLC_DIAG_ERR_HSE 0x1
#define PLC_DIAG_ERR_LSE 0x2

#define PLC_DIAG_ERR_APP_WARN 0x4  //User programm warning
#define PLC_DIAG_ERR_APP_CRIT 0x8  //User programm abort

#define PLC_DIAG_ERR_DEADLINE 0x1  //Deadline violation detected
#define PLC_DIAG_ERR_INVALID  0x20 //App code check error
#define PLC_DIAG_ERR_LOCATION 0x40 //location check efrror
#define PLC_DIAG_ERR_HW_OTHER 0x80 //Faailed HW

#define PLC_DIAG_ERR_OTHER_CRIT 0xF8 //Critical errors

extern volatile uint32_t plc_diag_status;

#endif // _PLC_DIAG_H_
