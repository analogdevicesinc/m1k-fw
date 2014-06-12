flash: cleanLocal all
	sudo /usr/local/bin/bossac -e -w -v -b helium.bin

cleanLocal:
	rm -f *bin *elf *hex *lss *map *sym

MAKEFILE_PATH = Makefile.sam.in
include $(MAKEFILE_PATH)
