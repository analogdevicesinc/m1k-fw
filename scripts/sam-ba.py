# Copyright 2014 Ian Daniher, Analog Devices
# Licensed under GPLv3
# This is fully functional sam-ba client capable of loading a binary executable onto the internal flash of a sam3u processor.
# This code is not impacted by any of the bugs in the ROM

import bitstring
import glob
import time
import usb

print "please wait..."

raw = open("./m1000.bin").read()
raw += '\x00'*(256-len(raw)%256)

fw = bitstring.ConstBitStream(bytes=raw)

dev = usb.core.find(idVendor=0x03eb,idProduct=0x6124)

try:
	dev.detach_kernel_driver(0)
	dev.detach_kernel_driver(1)
except:
	pass

dev.set_configuration(1)
regBase = 0x400e0800
flashBase = 0x80000
offset = 0

def getStr():
	return ''.join(map(chr, dev.read(0x82, 512, 1)))

def putStr(x):
	return dev.write(0x01, x, 1)

# erase flash
putStr("W400E0804,5A000005#")
time.sleep(0.1)
getStr()
# check if flash is erased
putStr("w400E0808,4#")
time.sleep(0.01)
getStr()
getStr()
getStr()

page = 0

# write each word
for pos in xrange(0,fw.length/8,4):
	fw.bytepos = pos
	addr = hex(flashBase+pos).lstrip("0x").rstrip("L").zfill(8)
	data = hex(fw.peek("<L")).lstrip("0x").rstrip("L").zfill(8)
	cmd = ("W"+addr+","+data+"#").upper()
	try:
		putStr(cmd)
		getStr()
		getStr()
	except:
		print 'error at', cmd
		quit()
	# if at end of page
	if pos & 0xFC == 0xFC:
		# write page
		cmd = "W400E0804,5A00"+hex(page).lstrip("0x").zfill(2)+"03#"
		putStr(cmd)
		time.sleep(0.01)
		getStr()
		getStr()
		# check that page is written
		putStr("w400E0808,4#")
		time.sleep(0.01)
		getStr()
		getStr()
		assert int(getStr().strip(), 16) == 1
		page += 1


# disable SAM-BA
putStr("W400E0804,5A00010B#")
getStr()
getStr()

# jump to flash
putStr("G00000000#")
getStr()

print "good to go!"
