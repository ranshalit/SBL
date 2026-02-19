PROJECT_INC = -I. -I.. -I../.. -I../cgutlcmn
PROJECT_LIB = -lcgos -lm -L./
C_source = cgutlcmd.c 
C_sourcep = bcprgcmd.c biosmodules.c biosupdate.c boardinfo.c firmwareupdate.c panelconfig.c ../cgutlcmn/bcprgcmn.c ../cgutlcmn/biosflsh.c ../cgutlcmn/cgepi.c ../cgutlcmn/cginfo.c ../cgutlcmn/cgmpfa.c ../cgutlcmn/cgutlcmn.c ../cgutlcmn/dmstobin.c
OPT = -Wall -Wno-multichar
DEF = -D"CONGA" -D"LINUX"

default: libcgutlp.o
	gcc  $(C_source) -o cgutlcmd libcgutlp.o $(OPT) $(DEF) $(PROJECT_INC) $(PROJECT_LIB)

libcgutlp.o:
	gcc -Wl,-r -no-pie -nostdlib $(C_sourcep) -o libcgutlp.o $(OPT) $(DEF) $(PROJECT_INC) 

clean:
	rm -f cgutlcmd *.so *.o

cleanall: clean

