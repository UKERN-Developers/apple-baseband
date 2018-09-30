
##############################################################################
# Internal rules
##############################################################################

ifeq ($(STACK_SRCROOT),)
  BEARTOOTH_SRCROOT := $(RC_EMBEDDEDPROJECT_DIR)/Mav5BaseBandFW
else
  BEARTOOTH_SRCROOT := $(STACK_SRCROOT)
endif

eur4release_zip = zip
eur4release_unzip = unzip
eur4release_firmware_file_base = Mav5

eur4release_firmware_path := $(BEARTOOTH_SRCROOT)/$(FIRMWARE_PATH2)

eur4release_firmware_info_file := $(eur4release_firmware_path)/Mav5-Latest.txt
eur4release_firmware_desired_file := $(eur4release_firmware_path)/$(notdir $(shell cat $(eur4release_firmware_info_file) | tr '\' '/' ))

eur4release_firmware_source_all := $(wildcard $(eur4release_firmware_path)/Mav5-[0-9]*.[0-9]*.*.zip)

eur4release_firmware_source := $(filter $(eur4release_firmware_desired_file),$(eur4release_firmware_source_all))
eur4release_firmware_file := $(basename $(notdir $(eur4release_firmware_source)))
eur4release_firmware_version := $(patsubst Mav5-%,%,$(eur4release_firmware_file))

eur4release_intermediate_dir := $(intermediate_dir)/eur4release
eur4release_intermediate_files := apps.mbn dsp1.mbn dsp2.mbn dsp3.mbn rpm.mbn sbl1.mbn sbl2.mbn restoresbl1.mbn
eur4release_intermediate_paths := $(addprefix $(eur4release_intermediate_dir)/,$(eur4release_intermediate_files))

eur4release_variants := Release POS
eur4release_firmware_img := $(foreach eur4release_file_type, $(eur4release_variants), $(output_img_dir)/$(eur4release_firmware_file).$(eur4release_file_type).bbfw)
eur4release_firmware_img_copy := $(foreach eur4release_file_type, $(eur4release_variants), $(output_img_dir)/$(eur4release_firmware_file).$(eur4release_file_type).bbfw\ $(DSTROOT)/$(FIRMWARE_PATH_RESTORE_UPDATE)/$(eur4release_firmware_file_base).$(eur4release_file_type).bbfw)


##############################################################################
# Things to tweak during bringup 
##############################################################################

eur4release_firmware_path_bringup := $(DSTROOT)/$(FIRMWARE_PATH)/Baseband/Mav5

# Rule for generating plist, comment out to disable generation
eur4release_firmware_plist := $(foreach eur4release_file_type, $(eur4release_variants), $(output_img_dir)/$(eur4release_firmware_file).$(eur4release_file_type).plist)
eur4release_firmware_plist_copy := $(foreach eur4release_file_type, $(eur4release_variants), $(output_img_dir)/$(eur4release_firmware_file).$(eur4release_file_type).plist\ $(DSTROOT)/$(FIRMWARE_PATH_RESTORE_UPDATE)/$(eur4release_firmware_file_base).$(eur4release_file_type).plist)

# Rule for generating a bringup bbfw, comment out AFTER personalization is done
eur4release_firmware_img_bringup_release := $(output_img_dir)/$(eur4release_firmware_file).Release.bbfw
eur4release_firmware_img_bringup_pos := $(output_img_dir)/$(eur4release_firmware_file).POS.bbfw
eur4release_firmware_img_bringup_release_install := $(eur4release_firmware_path_bringup)/$(eur4release_firmware_file).Release.bbfw
eur4release_firmware_img_bringup_pos_install := $(eur4release_firmware_path_bringup)/$(eur4release_firmware_file).POS.bbfw

ifneq ($(filter $(action), build install),)
  ifeq ($(eur4release_firmware_source),)
    $(error unable to locate compatible firmware file in $(eur4release_firmware_path))
  endif
endif

##############################################################################
# Build rules
#############################################################################

.PHONY: eur4release_build
eur4release_build: eur4release_firmware

.PHONY: eur4release_clean
eur4release_clean:
	rm -rf $(OUTPUT_DIR)
	find . \( -name '.DS_Store' \) -exec rm -rf {} \;

.PHONY: eur4release_install
eur4release_install: eur4release_build
	@echo "**** Installing $@ ****"
	ditto $(eur4release_firmware_plist) $(eur4release_firmware_img) $(DSTROOT)/$(FIRMWARE_PATH_RESTORE)/
	for plist_cmd_map in $(eur4release_firmware_plist_copy) ; do \
                ditto $$plist_cmd_map ; \
        done
	for img_cmd_map in $(eur4release_firmware_img_copy) ; do \
                ditto $$img_cmd_map ; \
        done
        
.PHONY: eur4release_firmware
eur4release_firmware: $(eur4release_firmware_img)
eur4release_firmware: $(eur4release_firmware_plist)

.PHONY: $(eur4release_firmware_img_bringup_release_install)
$(eur4release_firmware_img_install): $(eur4release_firmware_img)
	@mkdir -p $(@D)
	@echo "**** Copying and Unpacking $@ for Bringup ****"
	ditto $(eur4release_firmware_img_bringup_release) $(eur4release_firmware_img_bringup_release_install)
	$(eur4release_unzip) -o $(eur4release_firmware_img_bringup_release) -d $(eur4release_firmware_path_bringup)
	chmod go-w $(eur4release_firmware_path_bringup)/*
	
.PHONY: $(eur4release_firmware_img_bringup_pos_install)
$(eur4release_firmware_img_install): $(eur4release_firmware_img)
	@mkdir -p $(@D)
	@echo "**** Copying and Unpacking $@ for Bringup ****"
	ditto $(eur4release_firmware_img_bringup_pos) $(eur4release_firmware_img_bringup_pos_install)
	$(eur4release_unzip) -o $(eur4release_firmware_img_bringup_pos) -d $(eur4release_firmware_path_bringup)
	chmod go-w $(eur4release_firmware_path_bringup)/*	

$(eur4release_firmware_img): $(eur4release_intermediate_paths)
	@mkdir -p $(@D)
	@echo "**** Creating $@ with $(eur4release_intermediate_files) ****"
	$(eur4release_zip) -0j $@ $(eur4release_intermediate_paths)

$(eur4release_firmware_plist): $(eur4release_intermediate_dir) $(eur4release_firmware_img)
	@echo "**** Measuring $(eur4release_firmware_source) to $@ ****"
	$(measure_bbfw) -i $(eur4release_firmware_source) -o $@
	plutil -convert binary1 $@

$(eur4release_intermediate_paths): $(eur4release_firmware_source) $(eur4release_intermediate_dir)

$(eur4release_intermediate_dir): $(eur4release_firmware_source)
	@mkdir -p $@
	@echo "**** Extracting $(notdir $(eur4release_firmware_source)) to $(eur4release_intermediate_dir) ****"
	$(eur4release_unzip) -o $(eur4release_firmware_source) -d $(eur4release_intermediate_dir)

