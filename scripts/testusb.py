import usb
import math
import numpy as np
from scipy import io as sio
_chunk = lambda l, x: [l[i:i+x] for i in xrange(0, len(l), x)]

dev = usb.core.find(idVendor=0x0456,idProduct=0xcee2)
dev.set_interface_altsetting(0,1)

# set pots for sane simv
dev.ctrl_transfer(0x40|0x80, 0x1B, 0x0707, ord("a"), 4)

sineSIMV = [2**15+2**8+int(math.sin(math.pi*2.0*x/(2**8))*(2**8-1)) for x in range(2**8)]
sineFast = [2**15+int(math.sin(math.pi*2.0*x/(2**8-1))*(2**14-1))for x in range(2**14)]
sineSlow = [2**15+int(math.sin(math.pi*2.0*x/(2**16-1))*(2**15-1)) for x in range(2**16)]
target = sineFast
dev.get_active_configuration()[(0,1)][0].read(10280)

counts = []
voltages_a = []
voltages_b = []

for chunk in _chunk(target, 256):
	# bipolar scanning
	outPacket = chr(0b10000001)+chr(0b11000000)
	outPacket += chr(0b11110001)+chr(0b11000000)
	outPacket += ''.join(['\x00'+ chr((chunk[y]>>8)&0xFF)+chr(chunk[y]&0xFF)+'\x00' for y in range(256)])
	outPacket += ''.join(['\x00'+ chr((chunk[y]>>8)&0xFF)+chr(chunk[y]&0xFF)+'\x00' for y in range(256)])
	dev.get_active_configuration()[(0,1)][1].write(outPacket, timeout=100)
	data = dev.get_active_configuration()[(0,1)][0].read(1026)
	counts += [data[1] * 256 + data[0]]
	voltages_a += [(hb << 8 | lb) + (2**15-1) if hb <= 0x7f else (hb << 8 | lb) - (2**15-1) for hb,lb in zip(data[2:514:2], data[3:514:2])]
	voltages_b += [(hb << 8 | lb) + (2**15-1) if hb <= 0x7f else (hb << 8 | lb) - (2**15-1) for hb,lb in zip(data[514:1026:2], data[515:1026:2])]
	#voltages_a += [(hb << 8 | lb) for hb,lb in zip(data[2:514:2], data[3:514:2])]#[0:256:2]
	#voltages_a += [(hb << 8 | lb) for hb,lb in zip(data[514:1026:2], data[515:1026:2])][0:256:2]
	#voltages_b += [(hb << 8 | lb) for hb,lb in zip(data[2:514:2], data[3:514:2])]#[1:256:2]
	#voltages_b += [(hb << 8 | lb) for hb,lb in zip(data[514:1026:2], data[515:1026:2])]#[1:256:2]
	#voltages_b += [(hb << 8 | lb) for hb,lb in zip(data[2:514:2], data[3:514:2])][0:256:2]
	#voltages_a += [(hb << 8 | lb) for hb,lb in zip(data[2:514:2], data[3:514:2])][1:256:2]
	print len(data), len(voltages_a), len(voltages_b)

target = target

from pylab import *

plot(voltages_a, '.', label='a')
plot(voltages_b, '.', label='b')
plot(target, '.', label='dac')
xlabel('time (samples)')
ylabel('amplitude (bits)')
sio.savemat("smu.mat", {"v": voltages_a, "i": voltages_b, "setpoint": target})
legend(loc='best')
figure()
semilogy(fftfreq(len(voltages_a), 2e-05), fft(voltages_a), '.')
semilogy(fftfreq(len(target), 2e-05), fft(target), '.')
savefig("svmi-fft.png")
show()
sio.savemat("timing.mat", {"microframes":counts})
print "hw version:", ''.join(map(chr, dev.ctrl_transfer(0x40|0x80, 0x0, 0, 0, 32)))
print "fw version:", ''.join(map(chr, dev.ctrl_transfer(0x40|0x80, 0x0, 0, 1, 32)))
#show()
# jump to bootloader
#dev.ctrl_transfer(0x40|0x80, 0xBB, 0, 0, 1)
