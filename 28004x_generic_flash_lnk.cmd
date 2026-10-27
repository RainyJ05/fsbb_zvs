
MEMORY
{
PAGE 0 :
   /* BEGIN is used for the "boot to Flash" bootloader mode   */

   BEGIN           	: origin = 0x080000, length = 0x000002
   RAMM0           	: origin = 0x0000F6, length = 0x00030A

   RAMLS0          	: origin = 0x008000, length = 0x000800
   RAMLS1          	: origin = 0x008800, length = 0x000800
   RAMLS2      		: origin = 0x009000, length = 0x000800
   RAMLS3      		: origin = 0x009800, length = 0x000800
   RAMLS4      		: origin = 0x00A000, length = 0x000800
   RESET           	: origin = 0x3FFFC0, length = 0x000002

   /* Flash sectors */
   /* BANK 0 */
   FLASH_BANK0_SEC0  : origin = 0x080002, length = 0x000FFE	/* on-chip Flash */
   FLASH_BANK0_SEC1  : origin = 0x081000, length = 0x001000	/* on-chip Flash */
   FLASH_BANK0_SEC2  : origin = 0x082000, length = 0x001000	/* on-chip Flash */
   FLASH_BANK0_SEC3  : origin = 0x083000, length = 0x001000	/* on-chip Flash */
   FLASH_BANK0_SEC4  : origin = 0x084000, length = 0x001000	/* on-chip Flash */
   FLASH_BANK0_SEC5  : origin = 0x085000, length = 0x001000	/* on-chip Flash */
   /* Large ZVS table: Bank0 SEC6-15 and Bank1, with the final 0x10 words reserved. */
   FLASH_LUT         : origin = 0x086000, length = 0x00019FF0

   /* BANK 1 */
   /* FLASH_LUT above owns the remaining Flash address range. */

//   FLASH_BANK1_SEC15_RSVD : origin = 0x09FFF0, length = 0x000010  /* Reserve and do not use for code as per the errata advisory "Memory: Prefetching Beyond Valid Memory" */

PAGE 1 :

   BOOT_RSVD       : origin = 0x000002, length = 0x0000F1     /* Part of M0, BOOT rom will use this for stack */
   RAMM1           : origin = 0x000400, length = 0x0003F8     /* on-chip RAM block M1 */
//   RAMM1_RSVD      : origin = 0x0007F8, length = 0x000008     /* Reserve and do not use for code as per the errata advisory "Memory: Prefetching Beyond Valid Memory" */

   RAMLS5      : origin = 0x00A800, length = 0x000800
   RAMLS6      : origin = 0x00B000, length = 0x000800
   RAMLS7      : origin = 0x00B800, length = 0x000800

   RAMGS0      : origin = 0x00C000, length = 0x002000
   RAMGS1      : origin = 0x00E000, length = 0x002000
   RAMGS2      : origin = 0x010000, length = 0x002000
   RAMGS3      : origin = 0x012000, length = 0x001FF8
//   RAMGS3_RSVD : origin = 0x013FF8, length = 0x000008     /* Reserve and do not use for code as per the errata advisory "Memory: Prefetching Beyond Valid Memory" */
}


SECTIONS
{
   codestart        : > BEGIN,     PAGE = 0, ALIGN(4)
   .text            : >> FLASH_BANK0_SEC2 | FLASH_BANK0_SEC3 | FLASH_BANK0_SEC5,   PAGE = 0, ALIGN(4)
   .cinit           : > FLASH_BANK0_SEC1,     PAGE = 0, ALIGN(4)
   .switch          : > FLASH_BANK0_SEC1,     PAGE = 0, ALIGN(4)
   .reset           : > RESET,     PAGE = 0, TYPE = DSECT /* not used, */

   .stack           : > RAMM1,     PAGE = 1

#if defined(__TI_EABI__)
   .init_array      : > FLASH_BANK0_SEC1,       PAGE = 0,       ALIGN(4)
   .bss             : > RAMLS5,       PAGE = 1
   .bss:output      : > RAMLS3,       PAGE = 0
   .bss:cio         : > RAMLS0,       PAGE = 0
   .data            : > RAMLS5,       PAGE = 1
   .sysmem          : > RAMLS5,       PAGE = 1
   /* Initalized sections go in Flash */
   .const           : > FLASH_BANK0_SEC4,       PAGE = 0,       ALIGN(4)
#else
   .pinit           : > FLASH_BANK0_SEC1,       PAGE = 0,       ALIGN(4)
   .ebss            : > RAMLS5,       PAGE = 1
   .esysmem         : > RAMLS5,       PAGE = 1
   .cio             : > RAMLS0,       PAGE = 0
   .econst          : > FLASH_BANK0_SEC4,    PAGE = 0, ALIGN(4)
 #endif

   ZVS_T2_TABLE     : > FLASH_LUT,              PAGE = 0,       ALIGN(4)

   ramgs0           : > RAMGS0,    PAGE = 1
   ramgs1           : > RAMGS1,    PAGE = 1

 
#if defined(__TI_EABI__) 
   .TI.ramfunc      : LOAD = FLASH_BANK0_SEC1,
                      RUN = RAMLS0,
                      LOAD_START(RamfuncsLoadStart),
                      LOAD_SIZE(RamfuncsLoadSize),
                      LOAD_END(RamfuncsLoadEnd),
                      RUN_START(RamfuncsRunStart),
                      RUN_SIZE(RamfuncsRunSize),
                      RUN_END(RamfuncsRunEnd),
                      PAGE = 0, ALIGN(4)
#else					  
   .TI.ramfunc      : LOAD = FLASH_BANK0_SEC1,
                      RUN = RAMLS0,
                      LOAD_START(_RamfuncsLoadStart),
                      LOAD_SIZE(_RamfuncsLoadSize),
                      LOAD_END(_RamfuncsLoadEnd),
                      RUN_START(_RamfuncsRunStart),
                      RUN_SIZE(_RamfuncsRunSize),
                      RUN_END(_RamfuncsRunEnd),
                      PAGE = 0, ALIGN(4)
#endif

}

/*
//===========================================================================
// End of file.
//===========================================================================
*/
