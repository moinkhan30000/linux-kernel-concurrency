
CC       := gcc-12
CFLAGS   := -Wall -g
LDFLAGS  := -lrt


all: findmax module

findmax: findmax.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

findmax.o: findmax.c
	$(CC) $(CFLAGS) -c $< -o $@


obj-m += processinfo.o


KDIR := /lib/modules/$(shell uname -r)/build


PWD := $(shell pwd)

module:
	$(MAKE) -C $(KDIR) M=$(PWD) KBUILD_CFLAGS_REMOVE="-ftrivial-auto-var-init=zero" modules

# -------------------------------
# 3) Clean Targets
# -------------------------------
clean:
	rm -f findmax findmax.o
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# -------------------------------
# 4) Run the findmax Executable
# -------------------------------
run:
	./findmax $(ARGS)
