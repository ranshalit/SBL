# general 
The project is to use Intel's congatec conga-TEVAL/COMe 3.0 with tc570 SOM and to flash program slimbootloader on eeprom, and then run with buildroot (or yocto) on disk.
A later phase will also try to do secured boot.

# configuration
using congatec conga-TEVAL/COMe 3.0 with tc570 SOM

**important must connect usb-rs232 adapter (not any ftdi (uart))
i.e. SW12 rs232 state (1-on, 2-off)
     SW13 1-off 2-off
     X53 pin3 (rx)- pin 3 (tx)in rs232 male connector
     X53 pin5 (tx) - pin 2 (rx) in rs232 male connector
     X53 pin9 (gnd) - pin 5 (gnd) rs232 male connector
# slimboot loader project



# Reference
https://wiki.congatec.com/wiki/External_BIOS_Update_(AN07)#Onboard_BIOS_Flash_Update_Procedure 

https://dri.freedesktop.org/docs/drm/driver-api/mtd/intel-spi.html 

https://github.com/CE1CECL/IntelCSTools 
