#
# arch/v850/Makefile
#
#  Copyright (C) 2001,02,03,05  NEC Corporation
#  Copyright (C) 2001,02,03,05  Miles Bader <miles@gnu.org>
#
# This file is included by the global makefile so that you can add your own
# architecture-specific flags and dependencies. Remember to do have actions
# for "archclean" and "archdep" for cleaning up and making dependencies for
# this architecture
#
# This file is subject to the terms and conditions of the GNU General Public
# License.  See the file "COPYING" in the main directory of this archive
# for more details.
#

LDFLAGS_vmlinux := -r

arch_dir = arch/lkl

CFLAGS += -fno-builtin -g 

KALLSYMS += --symbol-prefix=$(shell echo | $(CROSS_COMPILE)cpp -dM | grep USER_LABEL_PREFIX | cut -f3 -d ' ')

KBUILD_DEFCONFIG := defconfig

core-y += $(arch_dir)/kernel/
lib-y  += $(arch_dir)/lib/
drivers-y += $(LKL_DRIVERS)/

