#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

export TARGET		:=	$(shell basename $(CURDIR))
export TOPDIR		:=	$(CURDIR)

export MAKE	:=	make
export FWPACK 	:=	$(CURDIR)/tools/fwpack

APPTITLE := wupSNES
APPVERSION := 01000000


.PHONY: herpderp

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all: herpderp tools/fwpack $(TARGET).fw

#---------------------------------------------------------------------------------
tools/fwpack:
	@echo making fwpack...
	@gcc ./tools/fwpack.c -o ./tools/fwpack

$(TARGET).fw	:	wifi/wifi.bin arm9/arm9.bin
	$(FWPACK) version=$(APPVERSION) title=$(APPTITLE) LVC_=arm9/arm9.bin WIFI=wifi/wifi.bin ROM_=rom.smc $@
	
wifi/wifi.bin:
	$(MAKE) -C wifi
	
#---------------------------------------------------------------------------------
arm9/arm9.bin:
	$(MAKE) -C arm9

# TODO figure out a nicer way to do this
herpderp:
	rm -f arm9/arm9.elf arm9/arm9.bin wifi/wifi.elf wifi/wifi.bin

#---------------------------------------------------------------------------------
clean:
	$(MAKE) -C wifi clean
	$(MAKE) -C arm9 clean
	rm -f $(TARGET).fw
