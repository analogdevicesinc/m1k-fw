import usb,sys,math
import time

dev = usb.core.find(idVendor=0x0456,idProduct=0xcee2)
dev.set_interface_altsetting(0,1)

def getVolts(conf):
	dev.ctrl_transfer(0xc0, 0xad, 0, conf, 2)
	z = [dev.ctrl_transfer(0x40|0x80, 0xAD, 0, 0,2) for x in range(5)][1::]
	vs = [(x[1] << 8 | x[0]) * 4.096/(2**16-1) for x in z]
	return vs

# magic number by nibble
# update, unipolar
# end at in3, full bw
# int 4.096 ref, scan
# scan no temp, do not readback conf
print getVolts(0xf738)
