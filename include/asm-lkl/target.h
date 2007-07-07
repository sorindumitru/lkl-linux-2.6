#ifndef _ASM_LKL_TARGET_H
#define _ASM_LKL_TARGET_H

#ifdef __GNUC__
#ifdef i386

#define BITS_PER_LONG 32
#define LITTLE_ENDIAN
#define mb() barrier()

#define ELF_CLASS	ELFCLASS32

#define TARGET_FORMAT "elf32-i386"
#define TARGET_ARCH   "i386"

#endif
#endif


#endif
