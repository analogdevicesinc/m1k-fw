import usb
d = usb.core.find(idVendor=0x59e3,idProduct=0xf000)
d.set_interface_altsetting(0,1)
d.get_active_configuration()[(0,1)][1].write([10]*1023)
print d.get_active_configuration()[(0,1)].bNumEndpoints
x = d.get_active_configuration()[(0,1)][0].read(1024)
print x[0], len(x)
