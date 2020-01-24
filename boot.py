# pylint: disable=import-error
import display
from machine import I2C
from mpu6886 import MPU6886
# pylint: enable=import-error

i2c = I2C(freq=400000, sda=21, scl=22) #Enable I2C
i2c.writeto_mem(52, 0x28, b'\x9c') #Dimm display
i2c.writeto_mem(52, 0x12, b'\x1f') # enable LDO3/2 (TFT and TFT_LED)

tft = display.TFT()
tft.init(tft.M5STACK, width=120, height=160, rst_pin=18, mosi=15, miso=32, clk=13, cs=5, dc=23, bgr=False, invrot=1, rot=tft.LANDSCAPE_FLIP, hastouch=False) #
tft.clear(tft.BLUE)
tft.text(tft.CENTER, tft.CENTER, 'Hello World', tft.RED)

mpu = MPU6886(i2c)
# mpu.gyro
# mpu.acc