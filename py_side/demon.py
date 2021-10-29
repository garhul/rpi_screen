#!/usr/bin/env python
import time
import pigpio
import os
import psutil
import argparse
import vcgencmd
import math

#define BSC_FIFO_SIZE 256
temp = 0

parser = argparse.ArgumentParser(description='i2c health status monitor')
parser.add_argument('addr', type=int, nargs='?',
                    help='i2c address')

args = parser.parse_args()
I2C_ADDR = args.addr or 0x45

print("...Starting i2c health monitor at i2c addr: 0x{:02X}...".format(I2C_ADDR))

def getHostname():
  return "{}\n".format(os.uname()[1])

def getIp():
  return "{}\n".format(psutil.net_if_addrs()['wlan0'][0].address)

def getCpuLoad():
  return "{}\n".format(psutil.getloadavg()[0])

def getTemp():
  return last_temp

def getUptime():
  total = time.time() - psutil.boot_time()
  rest = 0
  
  d = math.floor(total / (24 * 3600))
  rest = total - (d * 24 * 3600)

  h = math.floor(rest / 3600)
  rest = rest - (h * 3600)

  m = math.floor(rest / 60)  
  s = rest - (m * 60)

  return "{:.0f}d {:.0f}h {:.0f}m {:.0f}s\n".format(d,h,m,s)

def getMemory():
  return "{}\n".format(psutil.virtual_memory().percent)
  

def i2c(id, tick):
    global pi

    status, count, data = pi.bsc_i2c(I2C_ADDR)

    if data:
      if data[0] == ord('I'):
        pi.bsc_i2c(I2C_ADDR,getIp())
        print(getIp())
      
      elif data[0] == ord('C'):
        pi.bsc_i2c(I2C_ADDR,getCpuLoad())
        print(getCpuLoad())
      
      elif data[0] == ord('H'):
        pi.bsc_i2c(I2C_ADDR,getHostname())
        print(getHostname())

      elif data[0] == ord('T'):
        temp = getTemp()
        pi.bsc_i2c(I2C_ADDR, temp)
        print(temp)

      elif data[0] == ord('M'):
        pi.bsc_i2c(I2C_ADDR, getMemory());
        print(getMemory())

      elif data[0] == ord('U'):
        pi.bsc_i2c(I2C_ADDR, getUptime());
        print(getUptime())

pi = pigpio.pi()
if not pi.connected:
    print("Pigpio not connected, exiting")
    exit()


def shutdown():
   os.system("poweroff")

def restart():
   os.system("reboot")

last_temp = "{}\n".format(vcgencmd.Vcgencmd().measure_temp())

# Respond to BSC slave activity
e = pi.event_callback(pigpio.EVENT_BSC, i2c)
pi.bsc_i2c(I2C_ADDR) # Configure BSC as I2C slave
time.sleep(2) #time to be running
status, count, data = pi.bsc_i2c(I2C_ADDR)
print (data)


print(getCpuLoad())
print(getMemory())
print(getTemp())
print(getIp())
print(getUptime())

# loop forever
while True:  
  last_temp = "{}\n".format(vcgencmd.Vcgencmd().measure_temp())
  time.sleep(60)

e.cancel()
pi.bsc_i2c(0) # Disable BSC peripheral
pi.stop()