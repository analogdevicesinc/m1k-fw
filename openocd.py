# This is my "ghetto" helper script for erasing, flashing, and resetting with openocd.
import socket
import os, sys 
import time

c = socket.create_connection(('localhost', 4444))
print c.recv(10000)
c.send("reset halt\r\n")
time.sleep(0.01)
print c.recv(10000)
c.send("adapter_khz 1000\r\n")
time.sleep(0.01)
print c.recv(10000)
c.send("flash erase_sector 0 15\r\n")
time.sleep(0.01)
print c.recv(10000)
c.send("flash write_bank 0 "+os.path.abspath(os.path.curdir)+"/"+sys.argv[1]+" 0\r\n")
time.sleep(0.5)
print c.recv(10000)
c.send("reset run\r\n")
time.sleep(0.5)
print c.recv(10000)
c.close()
