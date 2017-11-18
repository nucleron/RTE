/******************************************************************************
*              This file contains IOM registry related stuff.                 *
*    You can use src/bsp/nuc-243/plc_iom_reg.c as reference implementation.   *
******************************************************************************/

/*****************************************************************************/
/* YAPLC includes */
#include <plc_config.h>
#include <plc_abi.h>
#include <plc_iom.h>

/*****************************************************************************/
/* Add project specific includes here. */

/*****************************************************************************/
/*
Declare your PLC driver interfaces here.
*/

/*
Every hardware driver which should be accessible by softPLC 
must be declared here.

Use 
PLC_IOM_METH_DECLS(plc_something);
for declarations.
*/

/*****************************************************************************/
/*
This is PLC IO manager registry array. 

It is used by PLC IO manager, see plc_iom.c/h for details.

Every hardware driver which should be accessible by softPLC 
must be registered here.
*/
const plc_io_metods_t plc_iom_registry[] =
{
    /*
    Use PLC_IOM_RECORD(plc_something) here to register your PLC drivers.
    */
};

/*****************************************************************************/
/* Must be declared after plc_iom_registry */
PLC_IOM_REG_SZ_DECL;

/*****************************************************************************/
/*
This function is heavily used by PLC IO manager. 

It takes the variable location protocol ID and returns corresponding 
"plc_iom_registry" index.

See plc_iom.c/h for details.
*/
uint8_t mid_from_pid(uint16_t proto)
{
    switch (proto)
    {
    /*Insert your code here.*/

    /*Don't delete this case!*/
    default:
        return PLC_IOM_MID_ERROR;
    }
    return PLC_IOM_MID_ERROR;/*Don't delete this line!*/
}
