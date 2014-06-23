
import usb

dev = usb.core.find(idVendor=0x0456,idProduct=0xcee2)
dev.set_interface_altsetting(0,1)
for i in range(255):
	if dev.ctrl_transfer(0x40|0x80, 0x5C, 0, i, 4)[0]:
		print hex(i >> 1)
