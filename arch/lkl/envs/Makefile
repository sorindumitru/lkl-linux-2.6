CFLAGS=-Wall $(LINUXINCLUDE)
apr_CFLAGS=-I/usr/include/apr-1.0 -D_LARGEFILE64_SOURCE 

$(obj)/%.o: $(srctree)/$(src)/%.c
	mkdir -p `dirname $@`
	$(CROSS_COMPILE)gcc $(CFLAGS) -c -o $@ $^ $($(patsubst %.c,%,$^)_CFLAGS)

_ENV=$(if $(CONFIG_LKL_ENV_POSIX),posix)
_ENV+=$(if $(CONFIG_LKL_ENV_NT),nt)
_ENV+=$(if $(CONFIG_LKL_ENV_NTK),ntk)
_ENV+=$(if $(CONFIG_LKL_ENV_APR),apr)
_ENV+=$(if $(CONFIG_LKL_ENV_LINUX),linux)
ENV=$(strip $(_ENV))


DRV=$(if $(CONFIG_LKL_DRV_DISK),disk) $(if $(CONFIG_LKL_DRV_ETH),eth) 
DRV_OBJS=$(patsubst %,$(obj)/drivers/$(ENV)/%.o,$(DRV))


env.a: $(patsubst %,$(obj)/%.o,$(ENV)) $(DRV_OBJS)
	$(CROSS_COMPILE)ar -rc $@ $^
	$(CROSS_COMPILE)ranlib $@
	echo $(DRV_OBJS) x$(ENV)x
