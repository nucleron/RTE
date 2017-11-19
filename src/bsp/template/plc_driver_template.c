/******************************************************************************
*                 This file contains PLC driver template.                     *
*                    See src/plc_iom.c/h for details.                         *
******************************************************************************/
#include <plc_iom.h>

/*****************************************************************************/
/*
DON'T RENAME FUNCTIONS IN THIS FILE!!!

Driver method names are generated at compile time.

Just define the following macro.
*/
#define LOCAL_PROTO define_me_please_

/*---------------------------------------------------------------------------*/
/* This function initiates PLC hardware at device reset. */
void PLC_IOM_LOCAL_INIT(void)
{
    /*Insert your code here.*/
}

/*---------------------------------------------------------------------------*/
/* This function checks PLC hardware after init. */
bool PLC_IOM_LOCAL_TEST_HW(void)
{
    /*If hardware is OK return true, else return false.*/

    /*Insert your code here.*/

    return true; /*Hardware is OK by default.*/
}

/*---------------------------------------------------------------------------*/
/* This function checks PLC variable locations. */
bool PLC_IOM_LOCAL_CHECK(uint16_t i)
{
    (void)i; /*Location index in softPLC location table.*/

    /*Return true if lication is valid or else return false.*/

    /*Insert your code here.*/

    return false; /*Check failed by default.*/
}

/*---------------------------------------------------------------------------*/
/*
These two functions are called when locations are checked and sorted 
to indicate "begin" and "end" IDs of "inputs", "memories", "outputs".

These functions are called three times for "inputs", "memories" and "outputs"
respectively.
*/

void PLC_IOM_LOCAL_BEGIN(uint16_t i)
{
    (void)i; /*Location index in softPLC location table.*/

    /*Insert your code here.*/
}

void PLC_IOM_LOCAL_END(uint16_t i)
{
    (void)i; /*Location index in softPLC location table.*/

    /*Insert your code here.*/
}

/*---------------------------------------------------------------------------*/
/*This function is called when we start softPLC.*/
void PLC_IOM_LOCAL_START(void)
{
    /*Insert your code here.*/
}

/*---------------------------------------------------------------------------*/
/*This function is called to schedule location specific actions.*/
uint32_t PLC_IOM_LOCAL_SCHED(uint16_t i, uint32_t tick)
{
    (void)i; /*Location index in softPLC location table.*/

    /*Insert your code here.*/

    return 0;
}

/*---------------------------------------------------------------------------*/
/* 
This function is called to schedule/do driver specific activities 
asynchronously with softPLC.
*/
void PLC_IOM_LOCAL_POLL(uint32_t tick)
{
    /*Insert your code here.*/
}

/*---------------------------------------------------------------------------*/
/*
This function is called when softPLC is stoped, it should be used to set 
PLCs outputs to safe state.
*/
void PLC_IOM_LOCAL_STOP(void)
{
    /*Insert your code here.*/
}

/*---------------------------------------------------------------------------*/
/*
This function is called when location is weighted for protocol specific sort.
*/
uint32_t PLC_IOM_LOCAL_WEIGTH(uint16_t i)
{
    (void)i; /*Location index in softPLC location table.*/
    return PLC_APP->l_tab[i]->a_data[0];
}

/*---------------------------------------------------------------------------*/
/*
This function is called when we get driver variables and transfer them to
located variables of softPLC.
*/
uint32_t PLC_IOM_LOCAL_GET(uint16_t i)
{
    
    (void)i; /*Location index in softPLC location table.*/
    return 0;
}

/*---------------------------------------------------------------------------*/
/*
This function is called when we get softPLC located variables and transfer them to
driver variables.
*/
uint32_t PLC_IOM_LOCAL_SET(uint16_t i)
{
    (void)i; /*Location index in softPLC location table.*/
    return 0;
}

/*---------------------------------------------------------------------------*/
#undef LOCAL_PROTO
