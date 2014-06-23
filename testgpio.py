import usb,sys

dev = usb.core.find(idVendor=0x0456,idProduct=0xcee2)
dev.set_interface_altsetting(0,1)
print dev.ctrl_transfer(0x40|0x80, 0x0E, int(sys.argv[1]), 0, 4) # 32 + 6 -> IDX_PB6 = 38
print dev.ctrl_transfer(0x40|0x80, 0xEE, int(sys.argv[1]), 0, 4) # 32 + 6 -> IDX_PB6 = 38
