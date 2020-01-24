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

tft.text(10, tft.CENTER, 'Hello World' , tft.GREEN)
tft.text(tft.CENTER, 0, 'Hello World' , tft.RED)

tft.deinit()

tft.clear()
tft.text(tft.CENTER, tft.CENTER, mpu.get_dir() , tft.GREEN)

# IMU
acc = mpu.acc
import math
vec = math.sqrt(sum([x*x for x in mpu.acc])) <1.1

vec = 0
for i in range (0, 10):
    vec = vec + math.sqrt(sum([x*x for x in mpu.acc]))
vec/10

if vec < 1.1:

if -1.1 < mpu.acc[0] < -0.9:
    print('x')
elif 0.9 < mpu.acc[0] < 1.1:
    print('-x')


# Upload 
ampy -p /dev/ttyUSB0 put /home/martinkorinek/projekte/WorkTimeTracker/M5StickC_MicroPython/MicroPython_BUILD/components/micropython/esp32/modules/mpu6886.py

>>> help(tft)

object TFT   (160x120, Type=M5STACK, Ready: yes, Color mode: 24-bit, Clk=10000000 Hz, RdClk=1000000 Hz, Touch: no)

Pins  (miso=32, mosi=15, clk=13, cs=5, dc=23, reset=18, backlight=-1) is of type TFT

  init -- <function>

  deinit -- <function>

  pixel -- <function>

  line -- <function>

  lineByAngle -- <function>

  triangle -- <function>

  circle -- <function>

  ellipse -- <function>

  arc -- <function>

  polygon -- <function>

  rect -- <function>

  roundrect -- <function>

  clear -- <function>

  fill -- <function>

  clearwin -- <function>

  font -- <function>

  fontSize -- <function>

  text -- <function>

  orient -- <function>

  textWidth -- <function>

  textClear -- <function>

  attrib7seg -- <function>

  image -- <function>

  compileFont -- <function>

  hsb2rgb -- <function>

  setwin -- <function>

  resetwin -- <function>

  savewin -- <function>

  restorewin -- <function>

  screensize -- <function>

  winsize -- <function>

  get_fg -- <function>

  get_bg -- <function>

  set_fg -- <function>

  set_bg -- <function>

  text_x -- <function>

  text_y -- <function>

  setColor -- <function>

  print -- <function>

  println -- <function>

  setRotation -- <function>

  setTextColor -- <function>

  setCursor -- <function>

  getCursor -- <function>

  fillScreen -- <function>

  drawPixel -- <function>

  drawLine -- <function>

  drawRect -- <function>

  fillRect -- <function>

  drawCircle -- <function>

  fillCircle -- <function>

  drawTriangle -- <function>

  fillTriangle -- <function>

  drawRoundRect -- <function>

  fillRoundRect -- <function>

  setBrightness -- <function>

  backlight -- <function>

  qrcode -- <function>

  tft_setspeed -- <function>

  tft_select -- <function>

  tft_deselect -- <function>

  tft_writecmd -- <function>

  tft_writecmddata -- <function>

  tft_readcmd -- <function>

  M5STACK -- 6

  CENTER -- -9003

  RIGHT -- -9004

  BOTTOM -- -9004

  LASTX -- 7000

  LASTY -- 8000

  PORTRAIT -- 0

  LANDSCAPE -- 1

  PORTRAIT_FLIP -- 2

  LANDSCAPE_FLIP -- 3

  FONT_Default -- 0

  FONT_DejaVu18 -- 1

  FONT_DejaVu24 -- 2

  FONT_Ubuntu -- 3

  FONT_Comic -- 4

  FONT_Minya -- 5

  FONT_Tooney -- 6

  FONT_Small -- 7

  FONT_DefaultSmall -- 8

  FONT_7seg -- 9

  BLACK -- 0

  NAVY -- 128

  DARKGREEN -- 32768

  DARKCYAN -- 32896

  MAROON -- 8388608

  PURPLE -- 8388736

  OLIVE -- 8421376

  LIGHTGREY -- 12632256

  DARKGREY -- 8421504

  BLUE -- 255

  GREEN -- 65280

  CYAN -- 65535

  RED -- 16515072

  MAGENTA -- 16515327

  YELLOW -- 16579584

  WHITE -- 16579836

  ORANGE -- 16557056

  GREENYELLOW -- 11336748

  PINK -- 16564426

  COLOR_BITS16 -- 16

  COLOR_BITS24 -- 24

  JPG -- 1

  BMP -- 2

  HSPI -- 1

  VSPI -- 2