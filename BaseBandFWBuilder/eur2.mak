
##############################################################################
# Internal rules
##############################################################################

ifeq ($(STACK_SRCROOT),)
  TREK_SRCROOT := $(RC_EMBEDDEDPROJECT_DIR)/TrekBaseBandFW
else
  TREK_SRCROOT := $(STACK_SRCROOT)
endif

eur2_zip = zip
eur2_unzip = unzip
eur2_firmware_file_base = Trek

eur2_options_dir := $(traits_dir)/Trek

eur2_firmware_path := $(TREK_SRCROOT)/$(FIRMWARE_PATH2)

eur2_firmware_info_file := $(eur2_firmware_path)/Trek-Latest.txt
eur2_firmware_desired_file := $(eur2_firmware_path)/$(shell cat $(eur2_firmware_info_file))

#$(warning $(eur2_firmware_desired_file))

eur2_firmware_source_all := $(wildcard $(eur2_firmware_path)/Trek-[0-9]*.[0-9]*.*.zip)

eur2_firmware_source := $(filter $(eur2_firmware_desired_file),$(eur2_firmware_source_all))
eur2_firmware_file := $(basename $(notdir $(eur2_firmware_source)))
eur2_firmware_version := $(patsubst Trek-%,%,$(eur2_firmware_file))

eur2_intermediate_dir := $(intermediate_dir)/eur2
eur2_intermediate_files := amss.mbn osbl.mbn dbl.mbn restoredbl.mbn Info.plist
eur2_intermediate_paths := $(addprefix $(eur2_intermediate_dir)/,$(eur2_intermediate_files))

eur2_file_variants := Release Debug POS
eur2_firmware_img := $(foreach eur2_file_type, $(eur2_file_variants), $(output_img_dir)/$(eur2_firmware_file).$(eur2_file_type).bbfw)
eur2_firmware_img_copy := $(foreach eur2_file_type, $(eur2_file_variants), $(output_img_dir)/$(eur2_firmware_file).$(eur2_file_type).bbfw\ $(DSTROOT)/$(FIRMWARE_PATH_RESTORE_UPDATE)/$(eur2_firmware_file_base).$(eur2_file_type).bbfw)
eur2_firmware_plist := $(foreach eur2_file_type, $(eur2_file_variants), $(output_img_dir)/$(eur2_firmware_file).$(eur2_file_type).plist)
eur2_firmware_plist_copy := $(foreach eur2_file_type, $(eur2_file_variants), $(output_img_dir)/$(eur2_firmware_file).$(eur2_file_type).plist\ $(DSTROOT)/$(FIRMWARE_PATH_RESTORE_UPDATE)/$(eur2_firmware_file_base).$(eur2_file_type).plist)

eur2_options := $(foreach eur2_file_type, $(eur2_file_variants), $(eur2_options_dir)/$(eur2_file_type)/$(options_file)) 

ifneq ($(filter $(action), build install),)
  ifeq ($(eur2_firmware_source),)
    $(error unable to locate compatible firmware file in $(eur2_firmware_path))
  endif
endif

##############################################################################
# Build rules
##############################################################################

.PHONY: eur2_build
eur2_build: eur2_firmware

.PHONY: eur2_clean
eur2_clean:
	rm -rf $(OUTPUT_DIR)
	find . \( -name '.DS_Store' \) -exec rm -rf {} \;

.PHONY: eur2_install
eur2_install: eur2_build
	@echo "**** Installing $@ ****"
	ditto $(eur2_firmware_plist) $(eur2_firmware_img) $(DSTROOT)/$(FIRMWARE_PATH_RESTORE)/
	for img_cmd_map in $(eur2_firmware_img_copy) ; do \
		ditto $$img_cmd_map ; \
	done
	for plist_cmd_map in $(eur2_firmware_plist_copy) ; do \
		ditto $$plist_cmd_map ; \
	done

.PHONY: eur2_firmware
eur2_firmware: $(eur2_firmware_img)
eur2_firmware: $(eur2_firmware_plist)

$(eur2_firmware_img): $(eur2_intermediate_paths) $(eur2_options)
	@mkdir -p $(@D)
	@echo "**** Creating $@ with $(eur2_intermediate_paths) $(eur2_options_dir)/$(subst .,,$(suffix $(basename $@)))/$(options_file) ****"
	$(eur2_zip) -0j $@ $(eur2_intermediate_paths) $(eur2_options_dir)/$(subst .,,$(suffix $(basename $@)))/$(options_file)

$(eur2_firmware_plist): $(eur2_intermediate_dirs) $(eur2_firmware_img)
	@echo "**** Measuring $(eur2_firmware_source) to $@ ****"
	$(measure_bbfw) -i $(eur2_firmware_source) -o $@
	plutil -convert binary1 $@

$(eur2_intermediate_paths): $(eur2_firmware_source) $(eur2_intermediate_dir)

$(eur2_intermediate_dir): $(eur2_firmware_source)
	@mkdir -p $@
	@echo "**** Extracting $(notdir $(eur2_firmware_source)) to $(eur2_intermediate_dir) ****"
	$(eur2_unzip) -o $(eur2_firmware_source) -d $(eur2_intermediate_dir)

