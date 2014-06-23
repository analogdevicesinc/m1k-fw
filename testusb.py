import usb

dev = usb.core.find(idVendor=0x0456,idProduct=0xcee2)
dev.set_interface_altsetting(0,1)
# bulk loopback
dev.get_active_configuration()[(0,1)][1].write([10]*511)
x = dev.get_active_configuration()[(0,1)][0].read(1024)
print "got val:", x[0], "ct:", len(x)
print "hw version:", ''.join(map(chr, dev.ctrl_transfer(0x40|0x80, 0x0, 0, 0, 32)))
print "fw version:", ''.join(map(chr, dev.ctrl_transfer(0x40|0x80, 0x0, 0, 1, 32)))
# jump to bootloader
#dev.ctrl_transfer(0x40|0x80, 0xBB, 0, 0, 1)
