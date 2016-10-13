/* Linker script for STM32F405RG Cortex-M4 ARM MCU */

/* Copyright (C)2013-2016, Philip Munts, President, Munts AM Corp.		*/
/*										*/
/* Redistribution and use in source and binary forms, with or without		*/
/* modification, are permitted provided that the following conditions are met:	*/
/*										*/
/* * Redistributions of source code must retain the above copyright notice,	*/
/*   this list of conditions and the following disclaimer.			*/
/*										*/
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"	*/
/* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE	*/
/* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE	*/
/* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE	*/
/* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR		*/
/* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF		*/
/* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS	*/
/* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN	*/
/* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)	*/
/* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE	*/
/* POSSIBILITY OF SUCH DAMAGE.							*/

MEMORY
{
  flash (rx)  : ORIGIN = 0x08008000, LENGTH = 992K
  ram (rwx)   : ORIGIN = 0x20002000, LENGTH = 120K
  ccm (rwx)   : ORIGIN = 0x10000000, LENGTH = 64K
}

__rom_start__	= ORIGIN(flash);
__rom_size__	= LENGTH(flash);
__ram_start__	= ORIGIN(ram);
__ram_size__	= LENGTH(ram);
__ram_end__	    = __ram_start__ + __ram_size__;
_plc_sstart	    = __ram_end__;		/* Top of RAM */

SECTIONS
{
  .text : {
  KEEP(*(.plc_app_abi_sec)) /* PLC APP ABI */
  KEEP(*(.plc_md5_sec))     /* PLC APP ABI */
  *(.text*)				    /* Program code */
  KEEP(*(.rodata*))			/* Read only data */

  . = ALIGN(4);
  _plc_pa_start = .;
  KEEP(*(.preinit_array))	    /* Preinitarray */
  _plc_pa_end = .;

  . = ALIGN(4);
  _plc_ia_start = .;
  KEEP (*(SORT(.init_array.*)))
  KEEP(*(.ctors))			    /*Construcrs*/
  _plc_ia_end = .;

  . = ALIGN(4);
  _plc_fia_start = .;
  KEEP(*(.fini_array))	        /*Destructors */
  KEEP (*(SORT(.fini_array.*)))
  _plc_fia_end = .;

  } >flash

  .data : ALIGN(16) {
  _plc_data_start = .;	    /* Used in PLC startup */
  *(.data*)				    /* Initialized data */
  _plc_data_end = .;	    /* Used in PLC startup */
  } >ram AT > flash
  _plc_data_loadaddr = LOADADDR(.data);

  .bss (NOLOAD) : ALIGN(16) {
  *(.bss*)				    /* Uninitialized data */
  *(COMMON)				    /* Common data */
  _plc_bss_end = .;			/* Used in PLC startup */
  } >ram

  /* C++ exception unwinding stuff--needed by some toolchains */

  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } >flash

  __exidx_start = .;
  .ARM.exidx : { *(.ARM.exidx* .gnu.linkonce.armexidx.*) } >flash
  __exidx_end = .;


  ._plc_check :
  {
  KEEP(*(.plc_check_sec))     /* PLC APP ABI */
  } > flash
}

/* Firmware entry point */

ENTRY(fake_start)

