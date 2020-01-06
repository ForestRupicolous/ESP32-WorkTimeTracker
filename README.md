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
   




https://github.com/loboris/MicroPython_ESP32_psRAM_LoBo

tft.init(tft.M5STACK, width=160, height=80, rst_pin=18, mosi=15, miso=32, clk=13, cs=5, dc=23)
lcd = lcd.TFT()
lcd.init(lcd.M5STACK, width=160, height=80, speed=40000000, rst_pin=18, 
         miso=32, mosi=15, clk=13, cs=5, dc=23, bgr=True,invrot=3, 
         expwm=machine.PWM(32, duty=0, timer=1))
lcd.setBrightness(30)
lcd.clear()
lcd.setColor(0xCCCCCC)



