
##############################################################################
# Internal rules
##############################################################################

ifeq ($(STACK_SRCROOT),)
  ZULU_SRCROOT := $(RC_EMBEDDEDPROJECT_DIR)/Mav4BaseBandFW
else
  ZULU_SRCROOT := $(STACK_SRCROOT)
endif

eur3_zip = zip
eur3_unzip = unzip
eur3_firmware_file_base = Mav4

eur3_options_dir := $(traits_dir)/Mav4

eur3_firmware_path := $(ZULU_SRCROOT)/$(FIRMWARE_PATH)

eur3_firmware_info_file := $(eur3_firmware_path)/Zulu-Latest.txt
eur3_firmware_desired_file := $(eur3_firmware_path)/$(shell cat $(eur3_firmware_info_file))

#$(warning $(eur3_firmware_desired_file))

eur3_firmware_source_all := $(wildcard $(eur3_firmware_path)/Mav4-[0-9]*.[0-9]*.*.zip)

eur3_firmware_source := $(filter $(eur3_firmware_desired_file),$(eur3_firmware_source_all))
eur3_firmware_file := $(basename $(notdir $(eur3_firmware_source)))
eur3_firmware_version := $(patsubst Mav4-%,%,$(eur3_firmware_file))

eur3_intermediate_dir := $(intermediate_dir)/eur3
eur3_intermediate_files := amss.mbn osbl.mbn ENPRG.hex ENPRG.mbn dbl.mbn partition.mbn dsp1.mbn dsp2.mbn
eur3_intermediate_paths := $(addprefix $(eur3_intermediate_dir)/,$(eur3_intermediate_files))

eur3_file_variants := Release Debug POS
eur3_firmware_img := $(foreach eur3_file_type, $(eur3_file_variants), $(output_img_dir)/$(eur3_firmware_file).$(eur3_file_type).bbfw)
eur3_firmware_img_copy := $(foreach eur3_file_type, $(eur3_file_variants), $(output_img_dir)/$(eur3_firmware_file).$(eur3_file_type).bbfw\ $(DSTROOT)/$(FIRMWARE_PATH_RESTORE_UPDATE)/$(eur3_firmware_file_base).$(eur3_file_type).bbfw)
eur3_firmware_plist := $(foreach eur3_file_type, $(eur3_file_variants), $(output_img_dir)/$(eur3_firmware_file).$(eur3_file_type).plist)
eur3_firmware_plist_copy := $(foreach eur3_file_type, $(eur3_file_variants), $(output_img_dir)/$(eur3_firmware_file).$(eur3_file_type).plist\ $(DSTROOT)/$(FIRMWARE_PATH_RESTORE_UPDATE)/$(eur3_firmware_file_base).$(eur3_file_type).plist)

eur3_options := $(foreach eur3_file_type, $(eur3_file_variants), $(eur3_options_dir)/$(eur3_file_type)/$(options_file))

ifneq ($(filter $(action), build install),)
  ifeq ($(eur3_firmware_source),)
    $(error unable to locate compatible firmware file in $(eur3_firmware_path))
  endif
endif

##############################################################################
# Build rules
##############################################################################

.PHONY: eur3_build
eur3_build: eur3_firmware

.PHONY: eur3_clean
eur3_clean:
	rm -rf $(OUTPUT_DIR)
	find . \( -name '.DS_Store' \) -exec rm -rf {} \;

.PHONY: eur3_install
eur3_install: eur3_build
	@echo "**** Installing $@ ****"
	ditto $(eur3_firmware_plist) $(eur3_firmware_img) $(DSTROOT)/$(FIRMWARE_PATH_RESTORE)/
	for img_cmd_map in $(eur3_firmware_img_copy) ; do \
                ditto $$img_cmd_map ; \
        done
	for plist_cmd_map in $(eur3_firmware_plist_copy) ; do \
                ditto $$plist_cmd_map ; \
        done

.PHONY: eur3_firmware
eur3_firmware: $(eur3_firmware_img)
eur3_firmware: $(eur3_firmware_plist)

$(eur3_firmware_img): $(eur3_intermediate_paths) $(eur3_options)
	@mkdir -p $(@D)
	@echo "**** Creating $@ with $(eur3_intermediate_paths) $(eur3_options_dir)/$(subst .,,$(suffix $(basename $@)))/$(options_file) ****"
	$(eur3_zip) -0j $@ $(eur3_intermediate_paths) $(eur3_options_dir)/$(subst .,,$(suffix $(basename $@)))/$(options_file)

$(eur3_firmware_plist): $(eur3_intermediate_dir) $(eur3_firmware_img)
	@echo "**** Measuring $(eur3_firmware_source) to $@ ****"
	$(measure_bbfw) -i $(eur3_firmware_source) -o $@
	plutil -convert binary1 $@

$(eur3_intermediate_paths): $(eur3_firmware_source) $(eur3_intermediate_dir)

$(eur3_intermediate_dir): $(eur3_firmware_source)
	@mkdir -p $@
	@echo "**** Extracting $(notdir $(eur3_firmware_source)) to $(eur3_intermediate_dir) ****"
	$(eur3_unzip) -o $(eur3_firmware_source) -d $(eur3_intermediate_dir)

