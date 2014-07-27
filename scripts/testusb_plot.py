import math
import numpy as np
from scipy import io as sio
from pylab import *

target = []
voltages_a = []
voltages_b = []

import fileinput
for line in fileinput.input():
  [o, v, i] = [float(x.strip()) for x in line.split(',')]
  target.append(o)
  voltages_a.append(v)
  voltages_b.append(i)

target = target[1::2]
current_a = voltages_a[1::2]
voltages_a = voltages_a[0::2]
current_b = voltages_b[1::2]
voltages_b = voltages_b[0::2]

plot(voltages_a, '.', label='va')
plot(current_a, '.', label='ia')
plot(voltages_b, '.', label='vb')
plot(current_b, '.', label='ib')
plot(target, '.', label='dac')
xlabel('time (samples)')
ylabel('amplitude (bits)')
sio.savemat("smu.mat", {"v": voltages_a, "i": current_a, "setpoint": target})
legend(loc='best')
figure()
semilogy(fftfreq(len(voltages_a), 2e-05), fft(voltages_a), '.')
semilogy(fftfreq(len(current_a), 2e-05), fft(current_a), '.')
semilogy(fftfreq(len(target), 2e-05), fft(target), '.')
savefig("svmi-fft.png")
show()
#sio.savemat("timing.mat", {"microframes":counts})
