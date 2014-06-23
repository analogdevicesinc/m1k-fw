
import usb

dev = usb.core.find(idVendor=0x0456,idProduct=0xcee2)
dev.set_interface_altsetting(0,1)
dev.ctrl_transfer(0x40|0x80, 0x1B, 0x3040, ord("a"), 4)
dev.ctrl_transfer(0x40|0x80, 0x1B, 0x3040, ord("b"), 4)
