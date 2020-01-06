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

from machine import I2C
i2c = I2C(freq=400000, sda=21, scl=22) #Enable I2C
i2c.scan()  #Check for devices
    [52, 81, 104] #52 is the adress of AXP192

i2c.writeto_mem(52, 0x28, b'\xcc') #Set TFT and TFT_LED to 3.0V
i2c.readfrom_mem(52, 0x12, 1) #Read Byte 12
    b'\x13' # 0b10011 -> DCDC enabled but LDO3/2 disabled
i2c.writeto_mem(52, 0x12, b'\x1f') # enable LDO3/2 (TFT and TFT_LED)
   
i2c.writeto_mem(52, 0x12, b'\x13') #disable LDO3/2 (TFT and TFT_LED)



https://github.com/loboris/MicroPython_ESP32_psRAM_LoBo

import display
tft = display.TFT()
tft.init(tft.M5STACK, width=120, height=160, rst_pin=18, mosi=15, miso=32, clk=13, cs=5, dc=23, bgr=False, invrot=1, rot=tft.LANDSCAPE, hastouch=False)
tft.clear(tft.BLUE)
tft.text(tft.CENTER, tft.CENTER, 'Hello World' , tft.RED)






tft.clear(tft.RED)

tft.clear(tft.BLUE)

 
tft.text(0, 25, 'Hello World' , tft.RED)

tft.text(0, tft.CENTER, 'Hello World' , tft.RED)
tft.text(tft.CENTER, 0, 'Hello World' , tft.RED)

tft.deinit()

tft.setwin(10,10, 160,80)