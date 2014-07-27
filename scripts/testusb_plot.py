import math
import numpy as np
from scipy import io as sio
from pylab import *

target = []
voltages_a = []
current_a = []

import fileinput
for line in fileinput.input():
  [o, v, i] = [float(x.strip()) for x in line.split(',')]
  target.append(o)
  voltages_a.append(v)
  current_a.append(i)

plot(voltages_a, '.', label='a')
plot(current_a, '.', label='b')
plot(target, '.', label='dac')
xlabel('time (samples)')
ylabel('amplitude (bits)')
sio.savemat("smu.mat", {"v": voltages_a, "i": current_a, "setpoint": target})
legend(loc='best')
figure()
semilogy(fftfreq(len(voltages_a), 2e-05), fft(voltages_a), '.')
semilogy(fftfreq(len(target), 2e-05), fft(target), '.')
savefig("svmi-fft.png")
show()
#sio.savemat("timing.mat", {"microframes":counts})