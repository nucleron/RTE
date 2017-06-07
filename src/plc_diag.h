#ifndef _PLC_DIAG_H_
#define _PLC_DIAG_H_

#define PLC_DIAG_ERR_HSE 0x001
#define PLC_DIAG_ERR_LSE 0x002
#define PLC_DIAG_ERR_APP_INFO 0x004  //User programm warning
#define PLC_DIAG_ERR_APP_WARN 0x008  //User programm warning

#define PLC_DIAG_ERR_OTHER_CRIT 0xFFFFFFF0 //Critical errors

#define PLC_DIAG_ERR_APP_CRIT 0x010  //User programm abort
#define PLC_DIAG_ERR_DEADLINE 0x020  //Deadline violation detected
#define PLC_DIAG_ERR_INVALID  0x040  //App code check error
#define PLC_DIAG_ERR_LOCATION 0x080  //location check efrror
#define PLC_DIAG_ERR_HW_OTHER 0x100  //Faailed HW

extern volatile uint32_t plc_diag_status;

#endif // _PLC_DIAG_H_
