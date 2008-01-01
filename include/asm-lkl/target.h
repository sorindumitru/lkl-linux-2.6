#ifndef _ASM_LKL_TARGET_H
#define _ASM_LKL_TARGET_H

#ifdef __GNUC__
#ifdef i386
#define BITS_PER_LONG 32
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN
#endif
#define mb() barrier()

#define ELF_CLASS	ELFCLASS32

#if defined(__ELF__)
#define TARGET_FORMAT "elf32-i386"
#define TARGET_ARCH   "i386"
#elif defined(__MINGW32__)
#define TARGET_FORMAT "pe-i386"
#define TARGET_ARCH   "i386"
#else
#error unknown target
#endif

#endif
#endif

#endif
