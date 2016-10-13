#include <plc_config.h>
#include <iec_std_lib.h>
#include <dbnc_flt.h>

/*
 * System menu settings
 */
#define SYS_MENU_READONLY (1<<5) | (1<<6) | (1<<7) | (1<<9) | (1<<12) | (1<<13) | (1<<14) | (1<<17) | (1<<18) | (1<<19) | (1<<20) | (1<<21)
#define SYS_MENU_ENABLED (1<<0) | (1<<1) | (1<<2) | (1<<3) | (1<<4) | (1<<5) | (1<<6) | (1<<7) | (1<<19) | (1<<20)
#define SYS_MENU_DESIGNATOR "0123456789abcdefghjlpr"


const uint32_t menu_readonly = SYS_MENU_READONLY;
const uint32_t sys_menu_enabled = SYS_MENU_ENABLED;
const uint8_t item_designator[] ={SYS_MENU_DESIGNATOR};

extern uint16_t menu_tmp;
extern uint8_t hmi_brightness;

extern uint32_t mb_baudrate;
extern uint8_t mb_slave_addr;
extern dbnc_flt_t in_flt[];

extern uint16_t digital_out;
extern uint8_t in_chnl;

#define MIN(x,y) (((x)>(y))?(y):(x))
void menu_sys_save_par(uint8_t num)
{
    tm now;
    if(num>=0 && num <= 2)
        plc_rtc_dt_get(&now);
    switch(num)
    {
    case 0: //YYYY
        now.tm_year = menu_tmp;
        plc_rtc_dt_set(&now);
        break;
    case 1: //MM.DD
        now.tm_day = menu_tmp%100;
        now.tm_mon = menu_tmp/100;
        plc_rtc_dt_set(&now);
        break;
    case 2: //HH.MM
        now.tm_min = menu_tmp%100;
        now.tm_hour = menu_tmp/100;
        plc_rtc_dt_set(&now);
        break;
    case 3: //hmi display brightness
        hmi_brightness = MIN(menu_tmp,7);
        plc_backup_save_brightness(hmi_brightness);
        break;
    case 4:
        in_chnl = MIN(menu_tmp,(PLC_DI_NUM-1));
        break;
    }
}

uint16_t menu_sys_get_par(uint8_t num)
{
    tm now;
    if(num>=0 && num <= 2)
        plc_rtc_dt_get(&now);
    switch(num)
    {
    case 0: //YYYY
        return now.tm_year;
    case 1: //MM.DD
        return now.tm_day+now.tm_mon*100;
    case 2: //HH.MM
        return now.tm_min + now.tm_hour*100;
    case 3: //hmi display brightness
        return hmi_brightness;
    case 4:
        return in_chnl;
    case 5:
        return in_flt[in_chnl].thr_on;
    case 6:
        return in_flt[in_chnl].thr_off;
    case 7:
        return digital_out;

    //modbus baudrate
    case 19:
        return mb_baudrate/100;
    case 20:
        return mb_slave_addr;

    default:
        return 0; //dummy
    }
}
