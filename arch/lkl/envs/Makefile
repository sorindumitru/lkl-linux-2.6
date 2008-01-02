CFLAGS=-Wall $(LINUXINCLUDE) $(EXTRA_CFLAGS)

$(obj)/%.o: $(srctree)/$(src)/%.c
	mkdir -p `dirname $@`
	$(CROSS_COMPILE)gcc $(CFLAGS) -c -o $@ $^ 

_ENV=$(if $(CONFIG_LKL_ENV_POSIX),posix)
_ENV+=$(if $(CONFIG_LKL_ENV_NT),nt)
_ENV+=$(if $(CONFIG_LKL_ENV_NTK),ntk)
_ENV+=$(if $(CONFIG_LKL_ENV_APR),apr)
_ENV+=$(if $(CONFIG_LKL_ENV_LINUX),linux)
ENV=$(strip $(_ENV))


DRV=$(if $(CONFIG_LKL_DRV_DISK),disk) $(if $(CONFIG_LKL_DRV_ETH),eth) 
DRV_OBJS=$(patsubst %,$(obj)/drivers/$(ENV)/%.o,$(DRV))


env.a: $(patsubst %,$(obj)/%.o,$(ENV)) $(DRV_OBJS) $(obj)/lib/*.o
	$(CROSS_COMPILE)ar -rc $@ $^
	$(CROSS_COMPILE)ranlib $@

