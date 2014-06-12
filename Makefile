flash: cleanLocal all
	sudo /usr/local/bin/bossac -e -w -v -b device_example_flash.bin

cleanLocal:
	rm -f *bin *elf *hex *lss *map *sym

MAKEFILE_PATH = Makefile.sam.in
include $(MAKEFILE_PATH)
