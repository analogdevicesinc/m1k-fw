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

voltages_a = voltages_a[512::]
voltages_b = voltages_b[512::]
target = target[512::]

plot(voltages_a, '.', label='va')
plot(voltages_b, '.', label='vb')
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
