# ESP32-WorkTimeTracker
Track time usage in multiple projects by turning a M5StickC to different sides.

https://docs.m5stack.com/#/en/core/m5stickc

##
hel Schematic:
https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/schematic/Core/M5StickC/20191118__StickC_A04_3110_Schematic_Rebuild_PinMap.pdf

## micropython
DISPLAY hängt am PMIC AXP192! Muss erst eingeschaltet werden und LED Backlight hängt auch am PMIC AXP192
From other Stcik https://github.com/sipeed/MaixPy/blob/master/components/boards/m5stick/src/m5stick.c

## Arduino Lib for AXP192:
https://github.com/m5stack/M5StickC/blob/master/src/AXP192.cpp


Idea: Duplicate I2C commands in Python:
Adress from code: 0x34 = 52

Enable
## PMIC
from machine import I2C
i2c = I2C(freq=400000, sda=21, scl=22) #Enable I2C
i2c.scan()  #Check for devices
    [52, 81, 104] #52 is the adress of AXP192

i2c.writeto_mem(52, 0x28, b'\xcc') #Set TFT and TFT_LED to 3.0V
i2c.readfrom_mem(52, 0x12, 1) #Read Byte 12
    b'\x13' # 0b10011 -> DCDC enabled but LDO3/2 disabled
i2c.writeto_mem(52, 0x12, b'\x1f') # enable LDO3/2 (TFT and TFT_LED)
   
i2c.writeto_mem(52, 0x12, b'\x13') #disable LDO3/2 (TFT and TFT_LED)
i2c.writeto_mem(52, 0x28, b'\x9c') #Dimm display


https://github.com/loboris/MicroPython_ESP32_psRAM_LoBo

## MPU6886
#Address from cpp code is 0x68 = 104
#define MPU6886_ADDRESS           0x68 
#define MPU6886_WHOAMI            0x75
#define MPU6886_ACCEL_INTEL_CTRL  0x69
#define MPU6886_SMPLRT_DIV        0x19
#define MPU6886_INT_PIN_CFG       0x37
#define MPU6886_INT_ENABLE        0x38
#define MPU6886_ACCEL_XOUT_H      0x3B
#define MPU6886_ACCEL_XOUT_L      0x3C
#define MPU6886_ACCEL_YOUT_H      0x3D
#define MPU6886_ACCEL_YOUT_L      0x3E
#define MPU6886_ACCEL_ZOUT_H      0x3F
#define MPU6886_ACCEL_ZOUT_L      0x40

#define MPU6886_TEMP_OUT_H        0x41
#define MPU6886_TEMP_OUT_L        0x42

#define MPU6886_GYRO_XOUT_H       0x43
#define MPU6886_GYRO_XOUT_L       0x44
#define MPU6886_GYRO_YOUT_H       0x45
#define MPU6886_GYRO_YOUT_L       0x46
#define MPU6886_GYRO_ZOUT_H       0x47
#define MPU6886_GYRO_ZOUT_L       0x48

#define MPU6886_USER_CTRL         0x6A
#define MPU6886_PWR_MGMT_1        0x6B
#define MPU6886_PWR_MGMT_2        0x6C
#define MPU6886_CONFIG            0x1A
#define MPU6886_GYRO_CONFIG       0x1B
#define MPU6886_ACCEL_CONFIG      0x1C
#define MPU6886_ACCEL_CONFIG2     0x1D
#define MPU6886_FIFO_EN           0x23

from mpu6886 import MPU6886
mpu = MPU6886(i2c, accel_sf=MPU6886.SF_G)
mpu.gyro


i2c.readfrom_mem(104, 0x75, 2) #whoAmI should return 0x19
i2c.writeto_mem(104, 0x6B, b'\x00') #pwr mgmt
#wait(10)
i2c.writeto_mem(104, 0x6B, b'\x80') #pwr mgmt
#wait(10)
i2c.writeto_mem(104, 0x6B, b'\x01') #pwr mgmt

i2c.writeto_mem(104, 0x6B, b'\x00') #pwr mgmt
i2c.readfrom_mem(104, 0x3B, 8) #Read temperature from MPU6886



i2c.writeto_mem(104, 0x28, b'\xcc')


## DISPLAY
import display
tft = display.TFT()
tft.init(tft.M5STACK, width=120, height=160, rst_pin=18, mosi=15, miso=32, clk=13, cs=5, dc=23, bgr=False, invrot=1, rot=tft.LANDSCAPE_FLIP, hastouch=False) #
tft.clear(tft.BLUE)
tft.text(tft.CENTER, tft.CENTER, 'Hello World' , tft.RED)


for x in range(0,120,10):
    tft.line(x+1, x+26, x+15, x+26, tft.GREEN)

#display goes from 1,26 to 
tft.rect(1, 26, 159, 80, tft.RED, tft.GREEN)


tft.setwin(1,26, 160,80)
 
tft.clear(tft.RED)

tft.clear(tft.BLUE)

 
tft.text(0, 25, 'Hello World' , tft.RED)

tft.text(0, tft.CENTER, 'Hello World' , tft.RED)
tft.text(tft.CENTER, 0, 'Hello World' , tft.RED)

tft.deinit()

