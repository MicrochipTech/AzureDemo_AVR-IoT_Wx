/* Default linker script, for normal executables */

/**************************************************************************************
/* Cortus APS3/APS3B Standard Linker Script for executables.
 **************************************************************************************/

/* Provide default definitions of various SIZE symbols if needed.
   These should normally be defined on the link command line with
        --defsym _STACK_SIZE=xx
   or linked in as part of the program (see memory.c example). */

_STACK_SIZE          = DEFINED(_STACK_SIZE)          ? _STACK_SIZE          :  64K;
_HEAP_SIZE           = DEFINED(_HEAP_SIZE)           ? _HEAP_SIZE           :  64K;
_PROGRAM_MEMORY_SIZE = DEFINED(_PROGRAM_MEMORY_SIZE) ? _PROGRAM_MEMORY_SIZE : 512K;
_DATA_MEMORY_SIZE    = DEFINED(_DATA_MEMORY_SIZE)    ? _DATA_MEMORY_SIZE    : 512K;


/********************************************************************************
 * Nothing should ever need changing below this point.
 ********************************************************************************/

MEMORY
{
  app_memory (!rx) :  org = 0x60000000, len = 64K
};

ENTRY(app_init)

PHDRS
{
   text PT_LOAD ;
   data PT_LOAD ;
   bss  PT_LOAD ;
};

SECTIONS
{
  .note.gnu.build-id :  { *(.note.gnu.build-id)}
  .hash                   : { *(.hash)}
  .dynsym                 : { *(.dynsym)}
  .dynstr                 : { *(.dynstr)}
  .gnu.version            : { *(.gnu.version)}
  .gnu.version_d          : { *(.gnu.version_d)}
  .gnu.version_r          : { *(.gnu.version_r)}

  .rel.init               : { *(.rel.init)}
  .rela.init              : { *(.rela.init)}
  .rel.text               : { *(.rel.text .rel.text.* .rel.gnu.linkonce.t.*)}
  .rela.text              : { *(.rela.text .rela.text.* .rela.gnu.linkonce.t.*)}
  .rel.fini               : { *(.rel.fini)}
  .rela.fini              : { *(.rela.fini)}
  .rel.rodata             : { *(.rel.rodata .rel.rodata.* .rel.gnu.linkonce.r.*)}
  .rela.rodata            : { *(.rela.rodata .rela.rodata.* .rela.gnu.linkonce.r.*)}
  .rel.srodata            : { *(.rel.srodata .rel.srodata.* .rel.gnu.linkonce.sr.*)}
  .rela.srodata           : { *(.rela.srodata .rela.srodata.* .rela.gnu.linkonce.sr.*)}
  .rel.data.rel.ro        : { *(.rel.data.rel.ro* .rel.gnu.linkonce.d.rel.ro.*)}
  .rela.data.rel.ro       : { *(.rela.data.rel.ro* .rela.gnu.linkonce.d.rel.ro.*)}
  .rel.data               : { *(.rel.data .rel.data.* .rel.gnu.linkonce.d.*)}
  .rela.data              : { *(.rela.data .rela.data.* .rela.gnu.linkonce.d.*)}
  .rel.ctors              : { *(.rel.ctors)}
  .rela.ctors             : { *(.rela.ctors)}
  .rel.dtors              : { *(.rel.dtors)}
  .rela.dtors             : { *(.rela.dtors)}
  .rel.bss                : { *(.rel.bss .rel.bss.* .rel.gnu.linkonce.b.*)}
  .rela.bss               : { *(.rela.bss .rela.bss.* .rela.gnu.linkonce.b.*)}
  .rel.data               : { *(.rel.data .rel.data.* .rel.gnu.linkonce.b.*)}
  .rela.data              : { *(.rela.data .rela.bss.* .rela.gnu.linkonce.b.*)}
  .rel.sbss               : { *(.rel.sbss .rel.sbss.* .rel.gnu.linkonce.sb.*)}
  .rela.sbss              : { *(.rela.sbss .rela.sbss.* .rela.gnu.linkonce.sb.*)}
  .rel.sdata              : { *(.rel.sdata .rel.sdata.* .rel.gnu.linkonce.s.*)}
  .rela.sdata             : { *(.rela.sdata .rela.bss.* .rela.gnu.linkonce.s.*)}
  .rel.vectors            : { *(.rel.vectors .rel.vectors.*)}
  .rela.vectors           : { *(.rela.vectors .rela.vectors.*)}

  .text   :
  {
    PROVIDE(_program_memory_start = .);
    /* Start of normal text section */
    PROVIDE(_text = .);
    KEEP (*(.init .init.*));
    *(.text .text.* .libgcc .gnu.linkonce.t.*);
    KEEP (*(.fini .fini.*));
    . = ALIGN(4);
    PROVIDE(_rodata = .);
    *(.rodata);
    *(SORT_BY_ALIGNMENT(.rodata.*));
    *(.gnu.linkonce.r.*);
    *(.gcc_except_table);
    *(SORT_BY_ALIGNMENT(.gcc_except_table.*));
    . = ALIGN(4);
    _erodata = .;
    PROVIDE(_etext = .);
  }  > app_memory :text

  .data    :
  {
    PROVIDE(_data_memory_start = .);
    PROVIDE(_sdata = .);
    *(.sdata);
    *(SORT_BY_ALIGNMENT(.sdata.*));
    *(SORT_BY_ALIGNMENT(.gnu.linkonce.s.*));
    . = ALIGN(4);
    PROVIDE(_esdata = .);
    PROVIDE(_srodata = .);
    *(.srodata);
    *(SORT_BY_ALIGNMENT(.srodata.*));
    *(.gnu.linkonce.sr.*);
    . = ALIGN(4);
    _esrodata = .;
        
    PROVIDE(__CTOR_LIST__ = .);
    KEEP(*(.ctors));
    PROVIDE(__CTOR_END__ = .);
    /* There is no real point in providing support for global destructors
       on an embedded system since main is not supposed to return */
    PROVIDE(__DTOR_LIST__ = .);
    KEEP(*(.dtors));
    PROVIDE(__DTOR_END__ = .);
    PROVIDE(_data = .);
    *(.data);
    *(SORT_BY_ALIGNMENT(.data.*));
    *(SORT_BY_ALIGNMENT(.patchtable.*));
    *(SORT_BY_ALIGNMENT(.gnu.linkonce.d.*));
    . = ALIGN(4);
    PROVIDE(_edata = .);
  }  > app_memory :data

  .bss   :
  {
    PROVIDE(_sbss = .);
    *(.sbss);
    *(.scommon);
    *(SORT_BY_ALIGNMENT(.sbss.*));
    *(SORT_BY_ALIGNMENT(.scommon.*));
    *(SORT_BY_ALIGNMENT(.gnu.linkonce.sb.*));
    . = ALIGN(4);
    PROVIDE(_esbss = .);
  
    PROVIDE(_bss = .);
    *(.bss);
    *(SORT_BY_ALIGNMENT(.bss.*));
    *(SORT_BY_ALIGNMENT(.gnu.linkonce.b.*));
    *(SORT_BY_ALIGNMENT(COMMON));

    /*********************************************************************************
     * Heap
     *********************************************************************************/

    /* Put the heap into the bss segment so that it gets cleared on startup. */
    . = ALIGN(4);
    __heap_bottom = . ;
    . += _HEAP_SIZE ;
    __heap_top = .;

    /*********************************************************************************
     * Stack
     *********************************************************************************/

    /* Put the stack into the bss segment so that it gets cleared on startup.
       NB. This means that the crt0 must not keep anything on the stack!
       In practice, if it is compiled with optimisation this is the case */
    . = ALIGN(4);
    __stack_bottom = . ;
    . += _STACK_SIZE ;
    __stack_top = .;

    . = ALIGN(4);
    _ebss = .;
  }  > app_memory :bss
  _end = . ;
  PROVIDE (end = .);

  /* Global data not cleared or intialised after reset.  */
  .noinit   :
  {
     PROVIDE (__noinit_start = .) ;
    *(.noinit  .noinit.*)
     PROVIDE (__noinit_end = .) ;
     PROVIDE (_data_memory_end = .) ;
  }  > app_memory :bss

  /* Check that everything fits */
  _sdata_size = _esrodata;
  _program_memory_size = (SIZEOF(.text));
  /*_program_memory_size = _etext - _text;*/
  /*_data_memory_size = __noinit_end - _data_memory_start;*/
  _data_memory_size = (SIZEOF(.data)) + (SIZEOF(.bss));
  /*_dummy = ASSERT(_sdata_size          <= 0x10000,              "Run out of space in short data section");*/
  _dummy = ASSERT(_program_memory_size <= _PROGRAM_MEMORY_SIZE, "Run out of space in program memory ");
  _dummy = ASSERT(_data_memory_size    <= _DATA_MEMORY_SIZE,    "Run out of space in data memory");

  /* This assertion is to ensure that we do not have a mix of code using the Cortus
     heap allocation routines and non Cortus heap allocation */
  _dummy = ASSERT(!((DEFINED(aps_malloc) || DEFINED(aps_malloc_dbg)) && DEFINED(malloc)), "Warning: Using a mixture of newlib heap and Cortus heap");

  /* This assertion is to ensure that if we are using Cortus heap allocation routines, then
     we compiled all of the code with DEBUG_HEAP or all of the code without DEBUG_HEAP */
  _dummy = ASSERT(!(DEFINED(aps_malloc) && DEFINED(aps_malloc_dbg)), "Warning: Some but not all code compiled with DEBUG_HEAP defined");
  

  /* Stabs debugging sections.  */
  .stab            0 : { *(.stab) }
  .stabstr         0 : { *(.stabstr) }
  .stab.excl       0 : { *(.stab.excl) }
  .stab.exclstr    0 : { *(.stab.exclstr) }
  .stab.index      0 : { *(.stab.index) }
  .stab.indexstr   0 : { *(.stab.indexstr) }
  .comment         0 : { *(.comment) }
  /* DWARF debug sections. */
  .debug           0 : { *(.debug) }
  .line            0 : { *(.line) }
  .debug_srcinfo   0 : { *(.debug_srcinfo) }
  .debug_sfnames   0 : { *(.debug_sfnames) }
  .debug_aranges   0 : { *(.debug_aranges) }
  .debug_pubnames  0 : { *(.debug_pubnames) }
  .debug_info      0 : { *(.debug_info .gnu.linkonce.wi.*) }
  .debug_abbrev    0 : { *(.debug_abbrev) }
  .debug_line      0 : { *(.debug_line) }
  .debug_frame     0 : { *(.debug_frame) }
  .debug_str       0 : { *(.debug_str) }
  .debug_loc       0 : { *(.debug_loc) }
  .debug_macinfo   0 : { *(.debug_macinfo) }
  .debug_weaknames 0 : { *(.debug_weaknames) }
  .debug_funcnames 0 : { *(.debug_funcnames) }
  .debug_typenames 0 : { *(.debug_typenames) }
  .debug_varnames  0 : { *(.debug_varnames) }
  .debug_pubtypes  0 : { *(.debug_pubtypes) }
  .debug_ranges    0 : { *(.debug_ranges) }
  .gnu.attributes  0 : { KEEP (*(.gnu.attributes)) }
  /DISCARD/          : { *(.note.GNU-stack) *(.gnu_debuglink) }
}
