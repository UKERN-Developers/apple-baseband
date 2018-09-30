
##############################################################################
# Internal rules
##############################################################################

ifeq ($(STACK_SRCROOT),)
  BEARTOOTH_SRCROOT := $(RC_EMBEDDEDPROJECT_DIR)/Mav5BaseBandFW_Debug
else
  BEARTOOTH_SRCROOT := $(STACK_SRCROOT)
endif

eur4debug_zip = zip
eur4debug_unzip = unzip
eur4debug_firmware_file_base = Mav5

eur4debug_firmware_path := $(BEARTOOTH_SRCROOT)/$(FIRMWARE_PATH2)

eur4debug_firmware_info_file := $(eur4debug_firmware_path)/Mav5-Latest_Debug.txt
eur4debug_firmware_desired_file := $(eur4debug_firmware_path)/$(notdir $(shell cat $(eur4debug_firmware_info_file) | tr '\' '/' ))

eur4debug_firmware_source_all := $(wildcard $(eur4debug_firmware_path)/Mav5-[0-9]*.[0-9]*.*_DEBUG.zip)

eur4debug_firmware_source := $(filter $(eur4debug_firmware_desired_file),$(eur4debug_firmware_source_all))
eur4debug_firmware_file := $(subst _DEBUG,,$(basename $(notdir $(eur4debug_firmware_source))))
eur4debug_firmware_version := $(patsubst Mav5-%,%,$(eur4debug_firmware_file))

eur4debug_intermediate_dir := $(intermediate_dir)/eur4debug
eur4debug_intermediate_files := apps.mbn dsp1.mbn dsp2.mbn dsp3.mbn rpm.mbn sbl1.mbn sbl2.mbn restoresbl1.mbn
eur4debug_intermediate_paths := $(addprefix $(eur4debug_intermediate_dir)/,$(eur4debug_intermediate_files))

eur4debug_file_type := Debug
eur4debug_firmware_img := $(output_img_dir)/$(eur4debug_firmware_file).$(eur4debug_file_type).bbfw
eur4debug_firmware_img_copy := $(eur4debug_firmware_file_base).$(eur4debug_file_type).bbfw

##############################################################################
# Things to tweak during bringup 
##############################################################################

eur4debug_firmware_path_bringup := $(DSTROOT)/$(FIRMWARE_PATH)/Baseband/Mav5

# Rule for generating plist, comment out to disable generation
eur4debug_firmware_plist := $(output_img_dir)/$(eur4debug_firmware_file).$(eur4debug_file_type).plist
eur4debug_firmware_plist_copy := $(eur4debug_firmware_file_base).$(eur4debug_file_type).plist

# Rule for generating a bringup bbfw, comment out AFTER personalization is done
eur4debug_firmware_img_bringup_install := $(eur4debug_firmware_path_bringup)/$(eur4debug_firmware_file).$(eur4debug_file_type).bbfw

ifneq ($(filter $(action), build install),)
  ifeq ($(eur4debug_firmware_source),)
    $(error unable to locate compatible firmware file in $(eur4debug_firmware_path))
  endif
endif

##############################################################################
# Build rules
#############################################################################

.PHONY: eur4_build
eur4debug_build: eur4debug_firmware

.PHONY: eur4debug_clean
eur4debug_clean:
	rm -rf $(OUTPUT_DIR)
	find . \( -name '.DS_Store' \) -exec rm -rf {} \;

.PHONY: eur4_install
eur4debug_install: eur4debug_build
	@echo "**** Installing $@ ****"
	ditto $(eur4debug_firmware_plist) $(eur4debug_firmware_img) $(DSTROOT)/$(FIRMWARE_PATH_RESTORE)/
	ditto $(eur4debug_firmware_plist) $(DSTROOT)/$(FIRMWARE_PATH_RESTORE_UPDATE)/$(eur4debug_firmware_plist_copy)
	ditto $(eur4debug_firmware_img) $(DSTROOT)/$(FIRMWARE_PATH_RESTORE_UPDATE)/$(eur4debug_firmware_img_copy)

.PHONY: eur4_firmware
eur4debug_firmware: $(eur4debug_firmware_img)
eur4debug_firmware: $(eur4debug_firmware_plist)

.PHONY: $(eur4debug_firmware_img_bringup_install)
$(eur4debug_firmware_img_bringup_install): $(eur4debug_firmware_img)
	@mkdir -p $(@D)
	@echo "**** Copying and Unpacking $@ for Bringup ****"
	ditto $(eur4debug_firmware_img) $(eur4debug_firmware_img_bringup_install)
	$(eur4debug_unzip) -o $(eur4debug_firmware_img) -d $(eur4debug_firmware_path_bringup)
	chmod go-w $(eur4debug_firmware_path_bringup)/*

$(eur4debug_firmware_img): $(eur4debug_intermediate_paths)
	@mkdir -p $(@D)
	@echo "**** Creating $@ with $(eur4debug_intermediate_files) ****"
	$(eur4debug_zip) -0j $@ $(eur4debug_intermediate_paths)

$(eur4debug_firmware_plist): $(eur4debug_intermediate_dir) $(eur4debug_firmware_img)
	@echo "**** Measuring $(eur4debug_firmware_source) to $@ ****"
	$(measure_bbfw) -i $(eur4debug_firmware_source) -o $@
	plutil -convert binary1 $@

$(eur4debug_intermediate_paths): $(eur4debug_firmware_source) $(eur4debug_intermediate_dir)

$(eur4debug_intermediate_dir): $(eur4debug_firmware_source)
	@mkdir -p $@
	@echo "**** Extracting $(notdir $(eur4debug_firmware_source)) to $(eur4debug_intermediate_dir) ****"
	$(eur4debug_unzip) -o $(eur4debug_firmware_source) -d $(eur4debug_intermediate_dir)

