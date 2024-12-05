import RPi_I2C_driver 
import time 
mylcd = RPi_I2C_driver.lcd() 
mylcd.lcd_display_string("Hello", 1) 
mylcd.lcd_display_string("Ato-Planet", 2) 
time.sleep(3) 
mylcd.lcd_clear() 
time.sleep(1) 
mylcd.backlight(0)