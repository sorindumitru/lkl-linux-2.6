LOCAL_CFLAGS = $(EXTRA_CFLAGS) $(CFLAGS) $(LINUXINCLUDE) $(LKLENV_CFLAGS) $(def_arch)

$(obj)/%.o: $(srctree)/$(src)/%.c
	mkdir -p `dirname $@`
	$(CROSS_COMPILE)gcc $(LOCAL_CFLAGS) -c -o $@ $^

_ENV=$(if $(CONFIG_LKL_ENV_POSIX),posix)
_ENV+=$(if $(CONFIG_LKL_ENV_NT),nt)
_ENV+=$(if $(CONFIG_LKL_ENV_NTK),ntk)
_ENV+=$(if $(CONFIG_LKL_ENV_APR),apr)
_ENV+=$(if $(CONFIG_LKL_ENV_LINUX),linux)
ENV=$(strip $(_ENV))


DRV=$(if $(CONFIG_LKL_DRV_DISK),disk) $(if $(CONFIG_LKL_DRV_ETH),eth) $(if $(CONFIG_LKL_DRV_ETH),eth_tun) 
DRV_OBJS=$(patsubst %,$(obj)/drivers/$(ENV)/%.o,$(DRV))

LIB=$(shell ls $(srctree)/$(src)/lib/*.c)
LIB_OBJS=$(patsubst $(srctree)/$(src)/lib/%.c,$(obj)/lib/%.o,$(LIB))

lkl.a: $(patsubst %,$(obj)/%.o,$(ENV)) $(DRV_OBJS) $(LIB_OBJS)
	$(CROSS_COMPILE)ar -rc $@ $^


