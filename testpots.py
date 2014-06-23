
import usb

dev = usb.core.find(idVendor=0x0456,idProduct=0xcee2)
dev.set_interface_altsetting(0,1)
dev.ctrl_transfer(0x40|0x80, 0x1B, 0x5f7f, ord("a"), 4)
