##############################################################################
# Baseband Firmware Builder
#
# Copyright 2009 Apple Inc. All rights reserved.
##############################################################################

##############################################################################
# Internal variables
##############################################################################

ifeq ($(STACK_SRCROOT),)
  ICE3_SRCROOT := $(RC_EMBEDDEDPROJECT_DIR)/BaseBandFW
else
  ICE3_SRCROOT := $(STACK_SRCROOT)
endif

xcode_target = NuFlsTools
xcode_build_dir = build

nu_fls_pack = $(wildcard $(xcode_build_dir)/NuFlsPack $(xcode_build_dir)/$(XCODE_CONFIG)/NuFlsPack)
nu_strip_fls = $(wildcard $(xcode_build_dir)/NuStripFls $(xcode_build_dir)/$(XCODE_CONFIG)/NuStripFls)

psi_ram = data/psi_ram.fls
ebl = data/ebl.fls
psi_flash = data/psi_flash.fls

# Pick the source of the stack.
stack_source := $(lastword $(wildcard $(ICE3_SRCROOT)/$(FIRMWARE_PATH)/ICE3_[0-9]*.fls))

# Must have a valid stack when building or installing
ifneq ($(filter $(action), build install),)
  ifeq ($(stack_source),)
      $(error unable to locate the file that contains the baseband stack in $(ICE3_SRCROOT)/$(FIRMWARE_PATH))
  endif
endif

# TODO: Determine stack string from the file contents instead from the filename
firmware_version := $(patsubst ICE3_%,%,$(basename $(notdir $(stack_source))))
firmware_version := $(patsubst ICE3%,%,$(firmware_version))
ifeq ($(firmware_version),)
    firmware_version := XX.YY.ZZ
endif

# TODO: Determine bootcore version from the file contents
bootcore_version := 02.13
ifeq ($(bootcore_version),)
    bootcore_version := AA.BB
endif

##############################################################################
# Output files
##############################################################################

bbname_prefix = ICE3
stack_prefix = $(bbname_prefix)_$(firmware_version)
bootloader_prefix = BOOT_$(bootcore_version)
file_variants = Release Debug POS
image_basename = $(stack_prefix)_$(bootloader_prefix)
image_basename_copy = $(bbname_prefix)
preflash_image_basename = $(stack_prefix)
preflash_image_basename_copy = $(bbname_prefix)

firmware_img := $(foreach file_type, $(file_variants), $(output_img_dir)/$(image_basename).$(file_type).bbfw)
firmware_img_copy := $(foreach file_type, $(file_variants), $(output_img_dir)/$(image_basename).$(file_type).bbfw\ $(DSTROOT)/$(FIRMWARE_PATH_RESTORE_UPDATE)/$(image_basename_copy).$(file_type).bbfw)
firmware_plist := $(foreach file_type, $(file_variants), $(output_img_dir)/$(image_basename).$(file_type).plist)
firmware_plist_copy := $(foreach file_type, $(file_variants), $(output_img_dir)/$(image_basename).$(file_type).plist\ $(DSTROOT)/$(FIRMWARE_PATH_RESTORE_UPDATE)/$(image_basename_copy).$(file_type).plist)
preflash_bin := $(foreach file_type, $(file_variants), $(output_img_dir)/$(preflash_image_basename).$(file_type).bin)
preflash_bin_copy := $(foreach file_type, $(file_variants), $(output_img_dir)/$(preflash_image_basename).$(file_type).bin\ $(DSTROOT)/$(FIRMWARE_PATH_RESTORE_UPDATE)/$(preflash_image_basename_copy).$(file_type).bin)


stack = $(intermediate_dir)/stack.fls
preflash_fls := $(foreach file_type, $(file_variants), $(intermediate_dir)/$(preflash_image_basename).$(file_type).preflash.bbfw)


nu_fls_tools = $(nu_fls_pack) $(nu_strip_fls)

##############################################################################
# Build rules
##############################################################################

.PHONY: ice3_build
ice3_build: nuflstools ice3_firmware preflash


.PHONY: ice3_install
ice3_install: ice3_install_firmware ice3_install_preflash

.PHONY: ice3_clean
ice3_clean:
	rm -rf $(OUTPUT_DIR)
	find . \( -name '.DS_Store' \) -exec rm -rf {} \;

.PHONY: ice3_install_firmware
ice3_install_firmware: ice3_build
	ditto $(firmware_img) $(firmware_plist) $(DSTROOT)/$(FIRMWARE_PATH_RESTORE)/
	for img_cmd_map in $(firmware_img_copy) ; do \
		ditto $$img_cmd_map ; \
	done
	for plist_cmd_map in $(firmware_plist_copy) ; do \
		ditto $$plist_cmd_map ; \
	done

.PHONY: ice3_install_preflash
ice3_install_preflash: ice3_build
	ditto $(preflash_bin) $(DSTROOT)/$(FIRMWARE_PATH_RESTORE)/
	for preflash_cmd_map in $(preflash_bin_copy) ; do \
		ditto $$preflash_cmd_map ; \
	done

.PHONY: ice3_firmware
ice3_firmware: $(firmware_img)
ice3_firmware: $(firmware_plist)

.PHONY: preflash
preflash: nuflstools $(preflash_bin)

.PHONY: nuflstools
nuflstools: force
	xcodebuild -target $(xcode_target) -configuration $(XCODE_CONFIG) build

$(stack): $(stack_source)
	@mkdir -p $(@D)
	@echo ""
	@echo "---- Creating stack image ----"
	$(call nu_fls_pack) --pack --removeboot $< --output $@

$(firmware_img): $(stack) $(psi_ram) $(ebl) $(psi_flash)
	@mkdir -p $(@D)
	@echo ""
	@echo "**** Creating firmware image ****"
	zip -j $@ $(psi_ram) $(ebl) $(stack) $(psi_flash)

$(firmware_plist): $(firmware_img)
	@mkdir -p $(@D)
	@echo ""
	@echo "**** Measuring firmware image ****"
	$(measure_bbfw) -i $< -o $@
	plutil -convert binary1 $@

$(preflash_fls): $(stack)
	@mkdir -p $(@D)
	@echo "**** Creating preflash fls ****"
	$(call nu_fls_pack) --pack $< --output $@

$(preflash_bin): $(preflash_fls)
	@mkdir -p $(@D)
	@echo ""
	@echo "**** Creating preflash image ****"
	$(call nu_strip_fls) $< --output $@
