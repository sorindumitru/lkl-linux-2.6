#ifndef _ASM_LKL_SOCKET_H
#define _ASM_LKL_SOCKET_H

#include <x86/include/asm/socket.h>

/*
 * FIXME: is there a better way?
 */

#ifdef __KERNEL__
#define ARCH_HAS_SOCKET_TYPES
enum sock_type {
	SOCK_STREAM	= 1,
	SOCK_DGRAM	= 2,
	SOCK_RAW	= 3,
	SOCK_RDM	= 4,
	SOCK_SEQPACKET	= 5,
	SOCK_DCCP	= 6,
	SOCK_PACKET	= 10,
};
#define SOCK_MAX (SOCK_PACKET + 1)

#define SOCK_TYPE_MASK 0xf

/* Flags for socket, socketpair, accept4 */
#define SOCK_CLOEXEC	O_CLOEXEC
#ifndef SOCK_NONBLOCK
#define SOCK_NONBLOCK	O_NONBLOCK
#endif

#else
#define	SOCK_STREAM 	1
#define	SOCK_DGRAM	2
#define	SOCK_RAW	3
#define	SOCK_RDM	4
#define	SOCK_SEQPACKET	5
#define	SOCK_DCCP	6
#define	SOCK_PACKET	10
#endif

#endif
