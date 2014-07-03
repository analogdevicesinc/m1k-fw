import usb,sys,math
import time
from pylab import *

def getVolts(conf):
	dev.ctrl_transfer(0xc0, 0xad, 0, conf, 2)
	z = [dev.ctrl_transfer(0x40|0x80, 0xAD, 0, 0,2) for x in range(5)][1::]
	vs = [(x[1] << 8 | x[0]) * 4.096/(2**16-1) for x in z]
	return vs

def setDAC(x):
	dev.ctrl_transfer(0x40|0x80, 0x3D, x, 0, 1)

# magic number by nibble
# update, unipolar
# end at in3, full bw
# int 4.096 ref, scan
# scan no temp, do not readback conf

def grabShot(uid):
	data = dev.ctrl_transfer(0xc0,0xa4,0,2048,4096)
	voltages = [(hb << 8 | lb) * 4.096/(2**16-1) for hb,lb in zip(data[::2], data[::1])]
	times = [t*(0.010152/2048) for t in range(2048)]
	plot(times,voltages,'.')
	xlabel("time(s)")
	ylabel("amplitude(v)")
	title(uid)
	savefig(uid+"+timeseries.png")
	clf()
	semilogy(fftfreq(2048, 0.010152/2048), fft(voltages),'.')
	title(uid)
	xlabel("freq(Hz)")
	ylabel("amplitude")
	savefig(uid+"+fft.png")
	clf()
	f = open(uid+"+tsdata.csv", "wb")
	for t,v in zip(times, voltages):
		f.write(str(t)+","+str(v)+"\r\n")
	f.flush()
	f.close()

if __name__ == "__main__":
	dev = usb.core.find(idVendor=0x0456,idProduct=0xcee2)
	dev.set_interface_altsetting(0,1)

	print getVolts(0xf038)
	for x in range(2**6):
		v = x*2**10
		uid = str(int(time.time()*1000000))+"+dac="+hex(v)
		setDAC(v)
		time.sleep(0.01)
		grabShot(uid)

