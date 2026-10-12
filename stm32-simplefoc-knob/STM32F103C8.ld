ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx) : ORIGIN = 0x08000000, LENGTH = 64K
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 20K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
  .isr_vector : { . = ALIGN(4); KEEP(*(.isr_vector)) . = ALIGN(4); } > FLASH
  .text : {
    . = ALIGN(4);
    *(.text*) *(.rodata*) *(.glue_7) *(.glue_7t)
    KEEP(*(.init)) KEEP(*(.fini))
    . = ALIGN(4);
    _etext = .;
  } > FLASH
  .ARM.exidx : { *(.ARM.exidx*) } > FLASH
  _sidata = LOADADDR(.data);
  .data : { . = ALIGN(4); _sdata = .; *(.data*) . = ALIGN(4); _edata = .; } > RAM AT > FLASH
  .bss (NOLOAD) : { . = ALIGN(4); _sbss = .; *(.bss*) *(COMMON) . = ALIGN(4); _ebss = .; } > RAM
  . = ALIGN(8);
  _end = .;
  PROVIDE(end = .);
  ASSERT(_ebss <= ORIGIN(RAM) + LENGTH(RAM) - 0x400, "RAM overflow: stack reservation unavailable")
  ASSERT(_etext <= ORIGIN(FLASH) + LENGTH(FLASH), "FLASH overflow")
}
