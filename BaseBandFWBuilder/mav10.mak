
##############################################################################
# Definitions
##############################################################################

ifeq ($(STACK_SRCROOT),)
  LUDLOW_SRCROOT := $(RC_EMBEDDEDPROJECT_DIR)/Mav10BaseBandFW_built
  BBCFG_SRCROOT  := $(RC_EMBEDDEDPROJECT_DIR)/bbcfg
else
  LUDLOW_SRCROOT := $(STACK_SRCROOT)
  BBCFG_SRCROOT  := $(STACK_SRCROOT)
endif

mav10_check:=$(if $(wildcard $(LUDLOW_SRCROOT)),,disabled)
mav10_prefix:=Mav10
mav10_variants:=Release Debug POS
mav10_latest_files:=Release=Mav10-Latest.txt Debug=Mav10-Latest_Debug.txt POS=Mav10-Latest.txt
mav10_firmware_dirs:=Release=$(LUDLOW_SRCROOT)/$(FIRMWARE_PATH2) Debug=$(LUDLOW_SRCROOT)/$(FIRMWARE_PATH2) POS=$(LUDLOW_SRCROOT)/$(FIRMWARE_PATH2)
mav10_files:=acdb.mbn apps.mbn dsp3.mbn	mba.mbn qdsp6sw.mbn rpm.mbn sbl1.mbn tz.mbn wdt.mbn restoresbl1.mbn bbcfg.mbn Info.plist
mav10_bbcfg_files:=Mav10BaseBandCFG
mav10_bbcfg_dirs:=$(BBCFG_SRCROOT)
