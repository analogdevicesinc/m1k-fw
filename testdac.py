import usb,sys,math

dev = usb.core.find(idVendor=0x0456,idProduct=0xcee2)
dev.set_interface_altsetting(0,1)


for z in range(10):
	for v in [int(math.sin(math.pi*2.0*x/1024.0)*2**14+2**15+2**14)-2 for x in range(1024)]:
		dev.ctrl_transfer(0x40|0x80, 0x3D, v, 0, 1)

dev.ctrl_transfer(0x40|0x80, 0x3D, 0x7fff, 0, 1)
