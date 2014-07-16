import usb
import math


dev = usb.core.find(idVendor=0x0456,idProduct=0xcee2)
dev.set_interface_altsetting(0,1)

dacVals = [int(math.sin(math.pi*2.0*x/256.0)*(2**15-1)+(2**15)) for x in range(256)]
outPacket = '\xf0\x38\x00\x00'*4

# A
outPacket += ''.join(['\x00'+ chr((dacVals[y]>>8)&0xFF)+chr(dacVals[y]&0xFF)+'\x00' for y in range(256)])
# B
outPacket += ''.join(['\x00'+ chr((dacVals[y]>>8)&0xFF)+chr(dacVals[y]&0xFF)+'\x00' for y in range(256)])

dev.get_active_configuration()[(0,1)][0].read(10280)

counts = []
data = []
for x in range(1024):
	dev.get_active_configuration()[(0,1)][1].write(outPacket)
	data_a = dev.get_active_configuration()[(0,1)][0].read(1026)
	counts += [data_a[1] * 256 + data_a[0]]
	data_a = data_a[2::]
	dev.get_active_configuration()[(0,1)][1].write(outPacket)
	data_b = dev.get_active_configuration()[(0,1)][0].read(1026)
	counts += [data_b[1] * 256 + data_b[0]]
	data_b = data_b[2::]
	data += data_a[:512]
	data += data_b[:512]

voltagesa = [(hb << 8 | lb) * 4.096/(2**16-1) for hb,lb in zip(data[::2], data[::1])][::1]
from pylab import *
plot(voltagesa, '.')
plot([(2.50*dacVal) / 2**16 for dacVal in dacVals], '.')
figure()
plot(counts, '.')
show()

print "hw version:", ''.join(map(chr, dev.ctrl_transfer(0x40|0x80, 0x0, 0, 0, 32)))
print "fw version:", ''.join(map(chr, dev.ctrl_transfer(0x40|0x80, 0x0, 0, 1, 32)))
# jump to bootloader
#dev.ctrl_transfer(0x40|0x80, 0xBB, 0, 0, 1)
