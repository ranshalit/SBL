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

# build manually
SBL_KEY_DIR=/mnt/storage/sleamboot/SBL/slimbootloader_04-Dec-2023_SBL0005/slimbootloader/Keys python BuildLoader.py build tgl -r

python3 Platform/TigerlakeBoardPkg/Script/StitchLoader.py -i BVTOR908.bin -s Outputs/tgl/SlimBootloader.bin -o sbl_tgl_Cg_TCTL.bin


# Build summary (exact flow I used)

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

# tools
2 options for cgutilcmd
For Linux full installation, you need a matching pair: the utility plus the correct CGOS driver stack.

Recommended modern full install:
cgutlcmd_linux_1.7.3.0.zip
CGOS_DIRECT_Lx_R3.1.4.zip
Legacy full install (only if you stay on older utility/source):
cgutillx.tar (or your existing cgutillx source)
cgoslx-x64-1.03.032_vmalloc.tar.xz for kernel 5.8+
cgoslx-x64-1.03.032.tar.xz for older kernels

# folders
backup - contain bin for the bios

# build files
started with Tal's SBL0005 from congatec (fot tc570), but it had only stdout in serial, i.e. rx was not supported. fixed get and rebuilt to image:
./slimbootloader_04-Dec-2023_SBL0005/slimbootloader/sbl_tgl_Cg_TCTO_rs232fix.bin

# Reference
https://wiki.congatec.com/wiki/External_BIOS_Update_(AN07)#Onboard_BIOS_Flash_Update_Procedure 

https://dri.freedesktop.org/docs/drm/driver-api/mtd/intel-spi.html 

https://github.com/CE1CECL/IntelCSTools 

https://wiki.congatec.com/wiki/Congatec_System_Utility_-_CGUTILis the cgutil embedded in the bios