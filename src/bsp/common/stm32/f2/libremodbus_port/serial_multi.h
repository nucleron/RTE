#ifndef SERIAL_MULTI_H
#define SERIAL_MULTI_H

#include <stdint.h>
#include <stdbool.h>


#include <mbconfig.h>
#include <mb_types.h>

typedef uint8_t BOOL;

typedef unsigned char UCHAR;
typedef char CHAR;

typedef uint16_t USHORT;
typedef int16_t SHORT;

typedef uint32_t ULONG;
typedef int32_t LONG;

typedef uint32_t DWORD;

#if MB_ASCII_ENABLED == 1
#define	BUF_SIZE	513     /* must hold a complete ASCII frame. */
#else
#define	BUF_SIZE	256     /* must hold a complete RTU frame. */
#endif

typedef struct
{
    uint32_t  uartnum;
    bool     txen;

	//timer
	DWORD           dwTimeOut;
	BOOL            bTimeoutEnable;
	DWORD           dwTimeLast;

	DWORD           defaultTimeout;

	//events
	eMBEventType eQueuedEvent;
	BOOL     xEventInQueue;

	void* parent;

}MBSerialInstance;


#define CALLBACK_ARG void* transport
#define CALLBACK_ARG_VOID

#define CALLER_ARG MBInstance* caller


#define MULTIPORT_SERIAL_ARG MBSerialInstance* inst,
#define MULTIPORT_SERIAL_ARG_VOID MBSerialInstance* inst

#endif
