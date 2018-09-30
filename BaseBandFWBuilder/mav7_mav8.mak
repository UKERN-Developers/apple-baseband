
##############################################################################
# Definitions
##############################################################################

ifeq ($(STACK_SRCROOT),)
  ALPENZOO_SRCROOT := $(RC_EMBEDDEDPROJECT_DIR)/Mav7Mav8BaseBandFW_built
else
  ALPENZOO_SRCROOT := $(STACK_SRCROOT)
endif

mav7_mav8_check:=$(if $(wildcard $(ALPENZOO_SRCROOT)),,disabled)
mav7_mav8_prefix:=Mav7Mav8
mav7_mav8_variants:=Release Debug POS
mav7_mav8_latest_files:=Release=Mav7Mav8-Latest.txt Debug=Mav7Mav8-Latest_Debug.txt POS=Mav7Mav8-Latest.txt
mav7_mav8_firmware_dirs:=Release=$(ALPENZOO_SRCROOT)/$(FIRMWARE_PATH2) Debug=$(ALPENZOO_SRCROOT)/$(FIRMWARE_PATH2) POS=$(ALPENZOO_SRCROOT)/$(FIRMWARE_PATH2)
mav7_mav8_files:=apps.mbn dsp1.mbn dsp2.mbn dsp3.mbn rpm.mbn sbl1.mbn sbl2.mbn restoresbl1.mbn 

