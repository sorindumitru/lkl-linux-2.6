#include <asm-generic/vmlinux.lds.h>
#include <asm/target.h>
#include <asm/thread_info.h>
        
OUTPUT_FORMAT(TARGET_FORMAT)
OUTPUT_ARCH(TARGET_ARCH)
ENTRY(start_kernel)
jiffies = jiffies_64;

SECTIONS
{
        /* Read-only sections, merged into text segment: */
        . = + SIZEOF_HEADERS;
        .interp : { *(.interp) }
        .hash          : { *(.hash)		}
        .gnu.hash      : { *(.gnu.hash)	}
        .dynsym        : { *(.dynsym)		}
        .dynstr        : { *(.dynstr)		}
        .rel.text      : { *(.rel.text)		}
        .rela.text     : { *(.rela.text) 	}
        .rel.data      : { *(.rel.data)		}
        .rela.data     : { *(.rela.data) 	}
        .rel.rodata    : { *(.rel.rodata) 	}
        .rela.rodata   : { *(.rela.rodata) 	}
        .rel.got       : { *(.rel.got)		}
        .rela.got      : { *(.rela.got)		}
        .rel.ctors     : { *(.rel.ctors)	}
        .rela.ctors    : { *(.rela.ctors)	}
        .rel.dtors     : { *(.rel.dtors)	}
        .rela.dtors    : { *(.rela.dtors)	}
        .rel.bss       : { *(.rel.bss)		}
        .rela.bss      : { *(.rela.bss)		}
        .rel.plt       : { *(.rel.plt)		}
        .rela.plt      : { *(.rela.plt)		}

        .plt : { *(.plt) }
        .text : {
                _text = .;
                _stext = . ;
                *(.text)
                SCHED_TEXT
                LOCK_TEXT
                *(.fixup)
                _etext = .;
                PROVIDE (etext = .);
        }


        RODATA

        .fini      : { *(.fini)    } =0
        .ctors     : { *(.ctors)   }
        .dtors     : { *(.dtors)   }

        .fixup     : { *(.fixup) }

        __ex_table : {
                __start___ex_table = .;
                *(__ex_table)
		__stop___ex_table = .;
	}

	__bug_table : {
		__start___bug_table = .;
		*(__bug_table)
		__stop___bug_table = .;
	}

        /* Read-write section, merged into data segment: */
        . = ALIGN(4096);
        .data    :
        {
                *(.data)
                CONSTRUCTORS
        }

        . = ALIGN(4096);
        __nosave_begin = .;
        .data_nosave : { *(.data.nosave) }
        . = ALIGN(4096);
        __nosave_end = .;

        . = ALIGN(32);
        .data.cacheline_aligned : { *(.data.cacheline_aligned) }

        _edata  =  .;
        PROVIDE (edata = .);

        . = ALIGN(4096);
        __init_begin = .;
        .init.text : {
                _sinittext = .;
                *(.init.text)
                _einittext = .;
        }
        /* .exit.text is discarded at runtime, not link time,
           to deal with references from __bug_table */
        .exit.text : { *(.exit.text) }
        .init.data : {
                *(.init.data);
        }

        . = ALIGN(16);


        .init.setup : { 
                __setup_start = .;
                *(.init.setup) 
                __setup_end = .;
        }


        .initcall.init : {
                __initcall_start = .;
                INITCALLS
               __initcall_end = .;           
        }

        .con_initcall.init : { 
                __con_initcall_start = .;
                *(.con_initcall.init) 
                __con_initcall_end = .;
        }

        SECURITY_INIT

        . = ALIGN(32);

        .data.percpu  : { 
                __per_cpu_start = .;
                *(.data.percpu) 
                __per_cpu_end = .;
        }


        . = ALIGN(4096);
        .init.ramfs : { 
                __initramfs_start = .;
                *(.init.ramfs) 
                __initramfs_end = .;
        }


        . = ALIGN(4096);
        __init_end = .;

        . = ALIGN(4096);
        _sextratext = .;
        _eextratext = .;

        .bss : {
                __bss_start = .;
                *(.sbss) *(.scommon)
                *(.dynbss)
                *(.bss)
                *(COMMON)
                __bss_stop = .;
        }


        _end = . ;
        PROVIDE (end = .);

        /* Sections to be discarded. */
        /DISCARD/ : {
                *(.exitcall.exit)
                *(.exit.data)
        }
}

