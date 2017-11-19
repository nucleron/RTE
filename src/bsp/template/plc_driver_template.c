/******************************************************************************
*                 This file contains PLC driver template.                     *
*                    See src/plc_iom.c/h for details.                         *
******************************************************************************/
#include <plc_iom.h>

/*****************************************************************************/
#define LOCAL_PROTO rename_me_please_

/*---------------------------------------------------------------------------*/
/* This function initiates PLC hardware at device reset. */
void PLC_IOM_LOCAL_INIT(void)
{
}

/*---------------------------------------------------------------------------*/
/* This function checks PLC hardware after init. */
bool PLC_IOM_LOCAL_TEST_HW(void)
{
    /*If hardware is OK return true, else return false.*/
    return true; /*Hardware is OK by default.*/
}

/*---------------------------------------------------------------------------*/
/* This function checks PLC variable locations. */
bool PLC_IOM_LOCAL_CHECK(uint16_t i)
{
    (void)i; /*Location index in softPLC location table.*/
    /*Return true if lication is valid or else return false.*/
    return false; /*Check failed by default.*/
}

/*---------------------------------------------------------------------------*/
void PLC_IOM_LOCAL_BEGIN(uint16_t i)
{
    (void)i; /*Location index in softPLC location table.*/
}

/*---------------------------------------------------------------------------*/
void PLC_IOM_LOCAL_END(uint16_t i)
{
    (void)i; /*Location index in softPLC location table.*/
}

/*---------------------------------------------------------------------------*/
void PLC_IOM_LOCAL_START(void)
{
    
}

/*---------------------------------------------------------------------------*/
uint32_t PLC_IOM_LOCAL_SCHED(uint16_t i, uint32_t tick)
{
    (void)i; /*Location index in softPLC location table.*/
    return 0;
}

/*---------------------------------------------------------------------------*/
void PLC_IOM_LOCAL_POLL(uint32_t tick)
{
}

/*---------------------------------------------------------------------------*/
void PLC_IOM_LOCAL_STOP(void)
{
}

/*---------------------------------------------------------------------------*/
uint32_t PLC_IOM_LOCAL_WEIGTH(uint16_t i)
{
    (void)i; /*Location index in softPLC location table.*/
    return PLC_APP->l_tab[i]->a_data[0];
}

/*---------------------------------------------------------------------------*/
uint32_t PLC_IOM_LOCAL_GET(uint16_t i)
{
    (void)i; /*Location index in softPLC location table.*/
    return 0;
}

/*---------------------------------------------------------------------------*/
uint32_t PLC_IOM_LOCAL_SET(uint16_t i)
{
    (void)i; /*Location index in softPLC location table.*/
    return 0;
}

/*---------------------------------------------------------------------------*/
#undef LOCAL_PROTO
