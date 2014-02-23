LTTNG_MODULES_DIR ?= $(src)/../../lttng/lttng-modules

obj-m := lttng_benchmark.o lttng_benchmark_probe.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
ccflags-y := -I$(src) -I$(LTTNG_MODULES_DIR)
KBUILD_EXTRA_SYMBOLS += $(LTTNG_MODULES_DIR)/Module.symvers 

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
