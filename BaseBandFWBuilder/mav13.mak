
##############################################################################
# Definitions
##############################################################################

ifeq ($(STACK_SRCROOT),)
  APRICA_SRCROOT := $(RC_EMBEDDEDPROJECT_DIR)/Mav13BaseBandFW_built
  BBCFG_SRCROOT  := $(RC_EMBEDDEDPROJECT_DIR)/bbcfg
  VINYL_SRCROOT  := $(RC_EMBEDDEDPROJECT_DIR)/VinylFirmware
else
  APRICA_SRCROOT := $(STACK_SRCROOT)
  BBCFG_SRCROOT  := $(STACK_SRCROOT)
  VINYL_SRCROOT  := $(STACK_SRCROOT)
endif

mav13_check:=$(if $(wildcard $(APRICA_SRCROOT)),,disabled)
mav13_prefix:=Mav13
mav13_variants:=Release Debug POS
mav13_latest_files:=Release=Mav13-Latest.txt Debug=Mav13-Latest_Debug.txt POS=Mav13-Latest.txt
mav13_firmware_dirs:=Release=$(APRICA_SRCROOT)/$(FIRMWARE_PATH2) Debug=$(APRICA_SRCROOT)/$(FIRMWARE_PATH2) POS=$(APRICA_SRCROOT)/$(FIRMWARE_PATH2)
mav13_files:=acdb.mbn apps.mbn dsp3.mbn	mba.mbn qdsp6sw.mbn rpm.mbn sbl1.mbn tz.mbn wdt.mbn restoresbl1.mbn bbcfg.mbn Info.plist
mav13_bbcfg_files:=Mav13BaseBandCFG
mav13_bbcfg_dirs:=$(BBCFG_SRCROOT)
mav13_vinyl_files:=vinyl
mav13_vinyl_dirs:=$(VINYL_SRCROOT)
