

/* For test only, we pretend we have a laaaarge controller */

MEMORY
{
	rom (rx)    : ORIGIN = 0x08001000, LENGTH = 2048K
	ram (rwx)   : ORIGIN = 0x20000000, LENGTH = 256K
}


/* Include the common ld script from libopenstm32. */
INCLUDE cortex-m-generic.ld
