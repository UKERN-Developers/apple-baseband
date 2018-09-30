
##############################################################################
# Internal rules
##############################################################################

ifeq ($(STACK_SRCROOT),)
  PHOENIX_SRCROOT := $(RC_EMBEDDEDPROJECT_DIR)/Phoenix
else
  PHOENIX_SRCROOT := $(STACK_SRCROOT)
endif

eur_zip = zip
eur_unzip = unzip
eur_firmware_file_base = Phoenix

eur_firmware_path := $(PHOENIX_SRCROOT)/$(FIRMWARE_PATH)

eur_firmware_info_file := $(eur_firmware_path)/Phoenix-Latest.txt
eur_firmware_desired_file := $(eur_firmware_path)/$(shell cat $(eur_firmware_info_file))

#$(warning $(eur_firmware_desired_file))

eur_firmware_source_all := $(wildcard $(eur_firmware_path)/Phoenix-[0-9]*.[0-9]*.*.zip)

eur_firmware_source := $(filter $(eur_firmware_desired_file),$(eur_firmware_source_all))
eur_firmware_file := $(basename $(notdir $(eur_firmware_source)))
eur_firmware_version := $(patsubst Phoenix-%,%,$(eur_firmware_file))

eur_intermediate_dir := $(intermediate_dir)/eur
eur_intermediate_files := amss.mbn osbl.mbn ENPRG.hex ENPRG.mbn dbl.mbn partition.mbn
eur_intermediate_paths := $(addprefix $(eur_intermediate_dir)/,$(eur_intermediate_files))

eur_file_variants := Release Debug POS
eur_firmware_img := $(foreach eur_file_type, $(eur_file_variants), $(output_img_dir)/$(eur_firmware_file).$(eur_file_type).bbfw)
eur_firmware_img_copy := $(foreach eur_file_type, $(eur_file_variants), $(output_img_dir)/$(eur_firmware_file).$(eur_file_type).bbfw\ $(DSTROOT)/$(FIRMWARE_PATH_RESTORE_UPDATE)/$(eur_firmware_file_base).$(eur_file_type).bbfw)
eur_firmware_plist := $(foreach eur_file_type, $(eur_file_variants), $(output_img_dir)/$(eur_firmware_file).$(eur_file_type).plist)
eur_firmware_plist_copy := $(foreach eur_file_type, $(eur_file_variants), $(output_img_dir)/$(eur_firmware_file).$(eur_file_type).plist\ $(DSTROOT)/$(FIRMWARE_PATH_RESTORE_UPDATE)/$(eur_firmware_file_base).$(eur_file_type).plist)

ifneq ($(filter $(action), build install),)
  ifeq ($(eur_firmware_source),)
    $(error unable to locate compatible firmware file in $(eur_firmware_path))
  endif
endif

##############################################################################
# Build rules
##############################################################################

.PHONY: eur_build
eur_build: eur_firmware

.PHONY: eur_clean
eur_clean:
	rm -rf $(OUTPUT_DIR)
	find . \( -name '.DS_Store' \) -exec rm -rf {} \;

.PHONY: eur_install
eur_install: eur_build
	@echo **** Installing $@ ****
	ditto $(eur_firmware_plist) $(eur_firmware_img) $(DSTROOT)/$(FIRMWARE_PATH_RESTORE)/
	for img_cmd_map in $(eur_firmware_img_copy) ; do \
		ditto $$img_cmd_map ; \
	done
	for plist_cmd_map in $(eur_firmware_plist_copy) ; do \
		ditto $$plist_cmd_map ; \
	done

.PHONY: eur_firmware
eur_firmware: $(eur_firmware_img)
eur_firmware: $(eur_firmware_plist)

$(eur_firmware_img): $(eur_intermediate_paths)
	@mkdir -p $(@D)
	@echo **** Creating $@ with $(eur_intermediate_files) ****
	$(eur_zip) -0j $@ $(eur_intermediate_paths)

$(eur_firmware_plist): $(eur_intermediate_dir) $(eur_firmware_img)
	@echo "**** Measuring $(eur_firmware_source) to $@ ****"
	$(measure_bbfw) -i $(eur_firmware_source) -o $@
	plutil -convert binary1 $@

$(eur_intermediate_paths): $(eur_firmware_source) $(eur_intermediate_dir)

$(eur_intermediate_dir): $(eur_firmware_source)
	@mkdir -p $@
	@echo "**** Extracting $(notdir $(eur_firmware_source)) to $(eur_intermediate_dir) ****"
	$(eur_unzip) -o $(eur_firmware_source) -d $(eur_intermediate_dir)

