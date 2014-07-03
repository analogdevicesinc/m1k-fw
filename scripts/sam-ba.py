# Copyright 2014 Ian Daniher, Analog Devices
# Licensed under GPLv3
# This is fully functional sam-ba client capable of loading a binary executable onto the internal flash of a sam3u processor.
# This code is not affected by any of the bugs in the ROM

import serial
import bitstring
import glob
import time
import usb

print "please wait..."

raw = open("./helium.bin").read()
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
	return ''.join(map(chr, dev.read(0x82, 512, 1, 100)))

def putStr(x):
	return dev.write(0x01, x, 1, 100)

# erase flash
cmd = "W400E0804,5A000005#"
putStr(cmd)
time.sleep(0.1)
getStr()
cmd = "w400E0808,4#"
putStr(cmd)
time.sleep(0.01)
getStr()
getStr()
getStr()

page = 0

for pos in xrange(0,fw.length/8,4):
	fw.bytepos = pos
	addr = hex(flashBase+pos).lstrip("0x").rstrip("L").zfill(8)
	data = hex(fw.peek("<L")).lstrip("0x").zfill(8)
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
		cmd = "W400E0804,5A00"+hex(page).lstrip("0x").zfill(2)+"03#"
		putStr(cmd)
		time.sleep(0.01)
		getStr()
		getStr()
		cmd = "w400E0808,4#"
		putStr(cmd)
		time.sleep(0.01)
		getStr()
		getStr()
		getStr()
		page += 1


cmd = "W400E0804,5A00010B#",

putStr(cmd)

putStr("G00080000#")

print "good to go!"
