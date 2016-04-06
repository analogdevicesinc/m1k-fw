# Copyright 2014 Ian Daniher, Analog Devices
# Licensed under GPLv3
# This is fully functional sam-ba client capable of loading a binary executable onto the internal flash of a sam3u processor.
# This code is not impacted by any of the bugs in the ROM

from __future__ import print_function

import bitstring
import glob
import io
import os
import sys
import time
import usb

# make sure we're using >=pyusb-1
if usb.version_info[0] < 1:
    print("pyusb-1 or newer is required")
    sys.exit(1)

# look for m1k
m1k = usb.core.find(idVendor=0x064b, idProduct=0x784c)
if m1k is not None:
    print("m1k device found, forcing into command mode")
    m1k.ctrl_transfer(0x40, 0xBB)
    # wait for the device to be re-enumerated
    time.sleep(1)

# look for m1k in programming mode
dev = usb.core.find(idVendor=0x03eb, idProduct=0x6124)
if dev is None:
    print(
        "no device found, make sure an m1k is plugged in and "
        "if necessary force it into command mode")
    sys.exit(1)

try:
    dev.detach_kernel_driver(0)
    dev.detach_kernel_driver(1)
except:
    pass

try:
    dev.set_configuration(1)
except usb.core.USBError:
    print('error configuring device, trying unplugging and plugging it back in')
    sys.exit(1)
regBase = 0x400e0800
flashBase = 0x80000
offset = 0

def getStr():
    return ''.join(map(chr, dev.read(0x82, 512, 1)))

def putStr(x):
    return dev.write(0x01, x, 1)

print("please wait, flashing device firmware")

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

try:
    firmware_file = sys.argv[1]
except IndexError:
    # fallback to current dir
    firmware_file = './m1000.bin'

if not os.path.exists(firmware_file):
    print("firmware file doesn't exist: {}".format(firmware_file))
    sys.exit(1)

# read in firmware file
raw = io.open(firmware_file, mode='rb').read()
raw += b'\x00'*(256-len(raw)%256)
fw = bitstring.ConstBitStream(bytes=raw)

# write each word
page = 0
for pos in range(0, int(fw.length/8), 4):
    fw.bytepos = pos
    addr = hex(flashBase+pos).lstrip("0x").rstrip("L").zfill(8)
    data = hex(fw.peek("<L")).lstrip("0x").rstrip("L").zfill(8)
    cmd = ("W"+addr+","+data+"#").upper()
    try:
        putStr(cmd)
        getStr()
        getStr()
    except:
        print('error at ' + cmd)
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
        page_status = getStr().strip()
        if not page_status or int(page_status, 16) != 1:
            print('error writing page {}'.format(page))
            sys.exit(1)
        page += 1

# disable SAM-BA
putStr("W400E0804,5A00010B#")
getStr()
getStr()

# jump to flash
putStr("G00000000#")
getStr()

print("successfully updated firmware, please unplug and replug the device to finish the process")
