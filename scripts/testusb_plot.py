import math
import numpy as np
from scipy import io as sio
from pylab import *

target = []
voltages_a = []
currents_a = []
voltages_b = []
currents_b = []

import fileinput
for line in fileinput.input():
  [o, va, ia, vb, ib] = [float(x.strip()) for x in line.split(',')]
  target.append(o)
  voltages_a.append(va)
  currents_a.append(ia)
  voltages_b.append(vb)
  currents_b.append(ib)

plot(voltages_a, '.', label='va')
plot(currents_a, '.', label='ia')
plot(voltages_b, '.', label='vb')
plot(currents_b, '.', label='ib')
plot(target, '.', label='dac')
xlabel('time (samples)')
ylabel('amplitude (bits)')
sio.savemat("smu.mat", {"v": voltages_a, "i": voltages_b, "setpoint": target})
legend(loc='best')
figure()
semilogy(fftfreq(len(voltages_a), 2e-05), fft(voltages_a), '.')
semilogy(fftfreq(len(voltages_b), 2e-05), fft(voltages_b), '.')
semilogy(fftfreq(len(target), 2e-05), fft(target), '.')
savefig("svmi-fft.png")
show()
