flash: cleanLocal all
	- python -c "import usb;usb.core.find(idVendor=0x0456,idProduct=0xcee2).ctrl_transfer(0x40|0x80, 0xBB, 0, 0, 1)"
	- sudo python ./scripts/sam-ba.py

cleanLocal:
	rm -f *bin *elf *hex *lss *map *sym *o *d

HW_VERSION=0
GIT_VERSION=$(shell git describe --always --dirty='*')

include ../asf/sam/utils/make/Makefile.sam.in

cflags-gnu-y += -D'HW_VERSION=$(HW_VERSION)'
cflags-gnu-y += -D'FW_VERSION=$(GIT_VERSION)'

