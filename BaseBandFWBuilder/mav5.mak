
##############################################################################
# Definitions
##############################################################################

ifeq ($(STACK_SRCROOT),)
  BEARTOOTH_SRCROOT := $(RC_EMBEDDEDPROJECT_DIR)/Mav5BaseBandFW_built
else
  BEARTOOTH_SRCROOT := $(STACK_SRCROOT)
endif

mav5_prefix:=Mav5
mav5_variants:=Release Debug POS
mav5_latest_files:=Release=Mav5-Latest.txt Debug=Mav5-Latest_Debug.txt POS=Mav5-Latest.txt
mav5_firmware_dirs:=Release=$(BEARTOOTH_SRCROOT)/$(FIRMWARE_PATH2) Debug=$(BEARTOOTH_SRCROOT)/$(FIRMWARE_PATH2) POS=$(BEARTOOTH_SRCROOT)/$(FIRMWARE_PATH2)
mav5_files:=apps.mbn dsp1.mbn dsp2.mbn dsp3.mbn rpm.mbn sbl1.mbn sbl2.mbn restoresbl1.mbn 

