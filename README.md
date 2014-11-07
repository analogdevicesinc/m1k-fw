This is the firmware for the microcontroller on the M1K.

Unless you are an advanced user, you are probably looking for the [desktop software]().



Build instructions:

With `arm-none-eabi-gcc` version 4.7 or later installed, run the following commands:

* `git clone --recursive https://github.com/itdaniher/ubuild.git` - this command will take awhile to complete as it fetches the Atmel Software Framework.
* `sudo make flash` - this command will build the firmware image and attempt to load it onto any attached SAM3U devices with either SAMBA or M1K vendor IDs.

`make flash` also requires Python 2.7 or later and PyUSB for the script to load the firmware on the target device.

Unplug and replug the attached device to try out the new firmware.

