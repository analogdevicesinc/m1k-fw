This is the firmware for the microcontroller on the M1K.

Unless you are an advanced user, you should probably start at the [Table of Contents](https://wiki.analog.com/university/tools/m1k).

### Build instructions:

With `arm-none-eabi-gcc` version 4.7 or later installed, run the following commands:

* `git clone --recursive https://github.com/analogdevicesinc/m1k-fw.git` - this command will take awhile to complete as it fetches the Atmel Software Framework.
* `sudo make flash` - this command will build the firmware image and attempt to load it onto any attached SAM3U devices with either SAMBA or M1K vendor IDs.

`make flash` also requires Python 2.7 or later and PyUSB for the script to load the firmware on the target device.

Unplug and replug the attached device to try out the new firmware.

### Updating on Windows

All SAM parts have a slightly broken boot ROM which presents a serial interface that is capable of flashing a firmware image to the device in its raw state. 

Due to the brokenness inherent in the bootloader, the recommended tool for updating the firmware is [the Python script in this repository] which treats the USB device implementing the CDC-ACM USB device class as a raw vendor device and directly interacts without using the serial interface and triggering bugs. This is advantageous due to finer grained control over packet boundaries - if SAM-BA receives commands split across subsequent USB packets, an error condition will arise. Unfortunately, Windows does not offer the ability for userspace programs to detatch kernel drivers, so the device must be interacted with as a serial device.

[BOSSA](www.shumatech.com/web/products/bossa) is a third-party open-source cross-platform tool which implements the required serial protocol and works around the afforementioned bugs. Install the latest version from [SourceForge](http://sourceforge.net/projects/b-o-s-s-a/files/) and follow the instructions [in the documentation](http://www.shumatech.com/web/products/bossa) to flash the m1000.bin file included in this repository.

Before you use BOSSA, you must first manually trigger the bootloader on the ADALM1000. This can be done by shorting out the small jumper on the "top" of the board, by the labels for the PIO connector. The device should disconnect from USB and re-enumerate as a SAMBA bootloader device / USB modem.
