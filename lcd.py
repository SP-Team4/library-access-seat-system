import RPi_I2C_driver 
import time 

def lcd_execute(str1, str2):
    mylcd = RPi_I2C_driver.lcd() 

    mylcd.lcd_display_string(str1, 1) 
    mylcd.lcd_display_string(str2, 2) 
    
    time.sleep(10) 
    
    mylcd.lcd_clear() 
    time.sleep(1) 
    mylcd.backlight(0)

    return 0

def main():
    lcd_execute("ajou", "library")