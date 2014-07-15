import usb
import math


dev = usb.core.find(idVendor=0x0456,idProduct=0xcee2)
dev.set_interface_altsetting(0,1)

dacVals = [int(math.sin(math.pi*2.0*x/256.0)*(2**15-1.0)+2**15) for x in range(256)]
outPacket = '\xf0\x38\x00\x00'*4
outPacket += ''.join(['\x00'+ chr((dacVals[y]>>8)&0xFF)+chr(dacVals[y]&0xFF)+'\x00' for y in range(256)])
data = dev.get_active_configuration()[(0,1)][0].read(8192)
outPacket += ''.join(['\x00'+ chr((dacVals[y]>>8)&0xFF)+chr(dacVals[y]&0xFF)+'\x00' for y in range(256)])
data += dev.get_active_configuration()[(0,1)][0].read(8192)
dev.get_active_configuration()[(0,1)][1].write(outPacket)
dev.get_active_configuration()[(0,1)][1].write(outPacket)
voltagesa = [(hb << 8 | lb) * 4.096/(2**16-1) for hb,lb in zip(data[::2], data[::1])][::2]
voltagesb = [(hb << 8 | lb) * 4.096/(2**16-1) for hb,lb in zip(data[::2], data[::1])][1::2]
from pylab import *
plot(voltagesa, '.')
plot(voltagesb, '.')
show()
#print data
print "got val:", x, "ct:", len(x)
print "hw version:", ''.join(map(chr, dev.ctrl_transfer(0x40|0x80, 0x0, 0, 0, 32)))
print "fw version:", ''.join(map(chr, dev.ctrl_transfer(0x40|0x80, 0x0, 0, 1, 32)))
# jump to bootloader
#dev.ctrl_transfer(0x40|0x80, 0xBB, 0, 0, 1)
