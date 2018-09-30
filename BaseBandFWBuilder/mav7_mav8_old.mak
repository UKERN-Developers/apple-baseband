
##############################################################################
# Definitions
##############################################################################

ifeq ($(STACK_SRCROOT),)
  ALPENZOO_SRCROOT := $(RC_EMBEDDEDPROJECT_DIR)/Mav7Mav8BaseBandFW
  ALPENZOO_SRCROOT_DEBUG := $(RC_EMBEDDEDPROJECT_DIR)/Mav7Mav8BaseBandFW_Debug
else
  ALPENZOO_SRCROOT := $(STACK_SRCROOT)
  ALPENZOO_SRCROOT_DEBUG := $(STACK_SRCROOT)
endif

mav7_mav8_old_check:=$(if $(call mav7_mav8_check),,disabled)
mav7_mav8_old_prefix:=Mav7Mav8
mav7_mav8_old_variants:=Release Debug POS
mav7_mav8_old_latest_files:=Release=Mav7Mav8-Latest.txt Debug=Mav7Mav8-Latest_Debug.txt POS=Mav7Mav8-Latest.txt
mav7_mav8_old_firmware_dirs:=Release=$(ALPENZOO_SRCROOT)/$(FIRMWARE_PATH2) Debug=$(ALPENZOO_SRCROOT_DEBUG)/$(FIRMWARE_PATH2) POS=$(ALPENZOO_SRCROOT)/$(FIRMWARE_PATH2)
mav7_mav8_old_files:=apps.mbn dsp1.mbn dsp2.mbn dsp3.mbn rpm.mbn sbl1.mbn sbl2.mbn restoresbl1.mbn 

