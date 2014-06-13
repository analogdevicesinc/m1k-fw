flash: cleanLocal all
	sudo /usr/local/bin/bossac -e -w -v -b helium.bin

cleanLocal:
	rm -f *bin *elf *hex *lss *map *sym *o *d

HW_PRODUCT=ADI Helium
HW_VERSION=0
GIT_VERSION=$(shell git describe --always --dirty='*')

MAKEFILE_PATH = Makefile.sam.in
include $(MAKEFILE_PATH)

cflags-gnu-y += -D'HW_PRODUCT=$(HW_PRODUCT)'
cflags-gnu-y += -D'HW_VERSION=$(HW_VERSION)'
cflags-gnu-y += -D'FW_VERSION=$(GIT_VERSION)'

