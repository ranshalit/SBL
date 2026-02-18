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


Build summary (exact flow I used)

Apply source fix (remove ConfigureCgbcUarts() call in CgInitializationLib.c).
Install missing tools: sudo apt-get update && sudo apt-get install -y nasm acpica-tools
Ensure BaseTools wrappers are Linux-executable + LF line endings (this repo had CRLF):
chmod +x /mnt/storage/sleamboot/SBL/slimbootloader_04-Dec-2023_SBL0005/slimbootloader/BaseTools/BinWrappers/PosixLike/*
find /mnt/storage/sleamboot/SBL/slimbootloader_04-Dec-2023_SBL0005/slimbootloader/BaseTools/BinWrappers/PosixLike -type f -exec sed -i 's/\r$//' {} +
Use existing keys dir (or generate if missing):
python [GenerateKeys.py](http://_vscodecontentref_/2) -k .../slimbootloader/Keys
Build release SBL with key path set:
SBL_KEY_DIR=/mnt/storage/sleamboot/SBL/slimbootloader_04-Dec-2023_SBL0005/slimbootloader/Keys python [BuildLoader.py](http://_vscodecontentref_/3) build tgl -r
Stitch full flash image from base IFWI + new SBL:
python [StitchLoader.py](http://_vscodecontentref_/4) -i [BVTOR908.bin](http://_vscodecontentref_/5) -s [SlimBootloader.bin](http://_vscodecontentref_/6) -o /mnt/storage/sleamboot/SBL/slimbootloader_04-Dec-2023_SBL0005/slimbootloader/sbl_tgl_Cg_TCTO_rs232fix.bin
Verify artifact/hash:
stat -c '%n %y %s' .../sbl_tgl_Cg_TCTO_rs232fix.bin
sha256sum .../sbl_tgl_Cg_TCTO_rs232fix.bin


embedded spi BIOS showed version BCT0424, hence we used BCTO files.

# Reference
https://wiki.congatec.com/wiki/External_BIOS_Update_(AN07)#Onboard_BIOS_Flash_Update_Procedure 

https://dri.freedesktop.org/docs/drm/driver-api/mtd/intel-spi.html 

https://github.com/CE1CECL/IntelCSTools 
