
unzip=unzip
zip=zip -j

firmware_extension=bbfw
measured_extension=plist
unpacked_suffix=.unpacked

################################################################
# INTERNAL ERROR CHECKING
################################################################

# verify that specified property is defined for everything
# 	$(1) = The property
check_missing_property = $(foreach platform,$(new_platforms),$(if $($(platform)_$(1)),,missing definition of $(platform)_$(1), ))
check_missing_property_bbcfg = $(foreach platform,$(new_platforms_bbcfg),$(if $($(platform)_$(1)),,missing definition of $(platform)_$(1), ))
check_missing_property_vinyl = $(foreach platform,$(new_platforms_vinyl),$(if $($(platform)_$(1)),,missing definition of $(platform)_$(1), ))

# Everything that's missing
missing_stuff = $(strip $(foreach what,prefix variants latest_files firmware_dirs files,$(call check_missing_property,$(what))))

################################################################
# RUNTIME CHECKS
################################################################

# If applicable, do a test on each platform to see if we should bother
new_platforms=$(foreach platform,$(new_platforms_all),$(if $($(platform)_check),,$(platform)))
new_platforms_bbcfg=$(foreach platform,$(bbcfg_platforms),$(if $($(platform)_check),,$(platform)))
new_platforms_vinyl=$(foreach platform,$(vinyl_platforms),$(if $($(platform)_check),,$(platform)))
new_platforms_super="$(new_platforms) $(new_platforms_bbcfg) $(new_platforms_vinyl) "

################################################################

# All variants
all_variants=$(sort $(foreach platform,$(new_platforms_super),$($(platform)_variants)))

################################################################
# User defined functions
################################################################

# get the firmware directory for a given platform and variant
# 	$(1) = The platform
# 	$(2) = The variant
firmware_dir_for_platform_and_variant = $(patsubst $(2)=%,%,$(filter $(2)=%,$($(1)_firmware_dirs)))

# get the BBCFG embedded directory for a given platform and variant
# 	$(1) = The platform
# 	$(2) = The variant
bbcfg_embedded_dir_for_platform_and_variant = $($(1)_bbcfg_dirs)/usr/standalone/firmware/

# get the VINYL embedded directory for a given platform and variant
# 	$(1) = The platform
# 	$(2) = The variant
vinyl_embedded_dir_for_platform_and_variant = $($(1)_vinyl_dirs)/usr/local/standalone/firmware/

# get the 'latest' file name for a given platform and variant
# 	$(1) = The platform
# 	$(2) = The variant
latest_filename_for_platform_and_variant = $(patsubst $(2)=%,%,$(filter $(2)=%,$($(1)_latest_files)))

# get the latest file name and path for given platform and variant
# 	$(1) = The platform
# 	$(2) = The variant
latest_file_for_platform_and_variant = $(call firmware_dir_for_platform_and_variant,$(1),$(2))/$(call latest_filename_for_platform_and_variant,$(1),$(2))

# get the raw zip file for a platform. This strips the path because the path is windows based and bogus
# 	$(1) = The platform
# 	$(2) = The variant
raw_zip_filename_for_platform_and_variant = $(notdir $(shell cat $(call latest_file_for_platform_and_variant,$(1),$(2)) | tr '\\' '/' ))

# get the raw zip file path for a platform
#	$(1) = The platform
# 	$(2) = The variant
raw_zip_file_for_platform_and_variant = $(call firmware_dir_for_platform_and_variant,$(1),$(2))/$(call raw_zip_filename_for_platform_and_variant,$(1),$(2))

# get the BBCFG file for a platform
# $(1) = The platform
# $(2) = The variant
get_bbcfg_file_for_platform_and_variant = $(call bbcfg_embedded_dir_for_platform_and_variant,$(1),$(2))/$($(1)_bbcfg_files)*.zip

# get the VINYL files for a platform
# $(1) = The platform
# $(2) = The variant
get_vinyl_files_for_platform_and_variant = $(addprefix $(call vinyl_embedded_dir_for_platform_and_variant,$(1),$(2))/,$($(1)_vinyl_files))

# get the firmware version for a platform. Expected format $(prefix)-<version>.zip
#	$(1) = The platform
# 	$(2) = The variant
version_for_platform_and_variant = $(subst _DEBUG,,$(patsubst $($(1)_prefix)-%.zip,%,$(call raw_zip_filename_for_platform_and_variant,$(1),$(2))))

# Used to extract the variant for a file. %.$(variant).$(extension)
# 	$(1) = The file name
variant_for_filename = $(strip $(foreach variant,$(all_variants),$(if $(filter %.$(variant),$(basename $(1))),$(variant),)))

# get the firmware file name for a platform. $(prefix)-$(version).$(variant).bbfw
#	$(1) = The platform
#	$(2) = The variant
firmware_filename_for_platform_and_variant = $($(1)_prefix)-$(call version_for_platform_and_variant,$(1),$(2)).$(2).$(firmware_extension)

# get all firmware files for a platform
# 	$(1) = The platform
firmware_filenames_for_platform = $(foreach variant,$($(1)_variants),$(call firmware_filename_for_platform_and_variant,$(1),$(variant)))

# get the platform from a file. Expected format $(prefix)%
# 	$(1)  = The file
platform_for_file = $(strip $(foreach platform,$(new_platforms),$(if $(filter $($(platform)_prefix)%,$(1)),$(platform),)))
platform_for_file_bbcfg = $(strip $(foreach platform,$(new_platforms_bbcfg),$(if $(filter $($(platform)_prefix)%,$(1)),$(platform),)))
platform_for_file_vinyl = $(strip $(foreach platform,$(new_platforms_vinyl),$(if $(filter $($(platform)_prefix)%,$(1)),$(platform),)))

# get the update file name for a platform. $(prefix).$(variant).bbfw
# 	$(1) = The platform
#  	$(2) = The variant
update_filename_for_platform_and_variant = $($(1)_prefix).$(2).$(firmware_extension)

# get all update files for a platform
#  	$(1) = The platform
update_filenames_for_platform = $(foreach variant,$($(1)_variants),$(call update_filename_for_platform_and_variant,$(1),$(variant)))

# get the measure file name for a platform. $(prefix)-$(version).$(variant).plist
#  	$(1) = The platform
#  	$(2) = The variant
measured_filename_for_platform_and_variant = $(if $($(1)_dont_measure),,$($(1)_prefix)-$(call version_for_platform_and_variant,$(1),$(2)).$(2).$(measured_extension))

# get all measure files for a platform
#  	$(1) = The platform
measured_filenames_for_platform = $(foreach variant,$($(1)_variants),$(call measured_filename_for_platform_and_variant,$(1),$(variant)))


# get the folder into which the unzipped files will be placed
#	$(1) = The platform
#	$(2) = The variant
unpacked_folder_for_platform_and_variant = $(addsuffix .$(2)$(unpacked_suffix),$(addprefix $(intermediate_dir)/,$(call raw_zip_filename_for_platform_and_variant,$(1),$(2))))

# get the paths of all unzipped files in the temporary location
#	$(1) = The platform
#	$(2) = The variant
unpacked_files_for_platform_and_variant = $(addprefix $(call unpacked_folder_for_platform_and_variant,$(1),$(2))/,$($(1)_files)) 

# get the option files for a particular platform and variant
#	$(1) = The platform
#	$(2) = The variant
options_file_for_platform_and_variant = $(traits_dir)/$($(1)_prefix)/$(2)/$(options_file)

# get the options files for a particular platform
# 	$(1) = The platform
options_files_for_platform = $(foreach variant,$($(1)_variants),$(call options_file_for_platform_and_variant,$(1),$(variant)))

################################################################
# COMPOSITING OF FILES
################################################################

# All of the latest text files
latest_files := $(foreach platform,$(new_platforms),$(foreach variant,$($(platform)_variants),$(call latest_file_for_platform_and_variant,$(platform),$(variant))))

latest_files_bbcfg := $(foreach platform,$(new_platforms_bbcfg),$(foreach variant,$($(platform)_variants),$(call latest_file_for_platform_and_variant,$(platform),$(variant))))

latest_files_vinyl := $(foreach platform,$(new_platforms_vinyl),$(foreach variant,$($(platform)_variants),$(call latest_file_for_platform_and_variant,$(platform),$(variant))))

# Compositing of zip files
raw_zip_files := $(foreach platform,$(new_platforms),$(foreach variant,$($(platform)_variants),$(call raw_zip_file_for_platform_and_variant,$(platform),$(variant))))

raw_zip_files_bbcfg := $(foreach platform,$(new_platforms_bbcfg),$(foreach variant,$($(platform)_variants),$(call raw_zip_file_for_platform_and_variant,$(platform),$(variant))))

raw_zip_files_vinyl := $(foreach platform,$(new_platforms_vinyl),$(foreach variant,$($(platform)_variants),$(call raw_zip_file_for_platform_and_variant,$(platform),$(variant))))

# These are the files that well get when we temporarily unzip everything
unpacked_files := $(foreach platform,$(new_platforms),$(foreach variant,$($(platform)_variants),$(call unpacked_files_for_platform_and_variant,$(platform),$(variant))))

unpacked_files_bbcfg := $(foreach platform,$(new_platforms_bbcfg),$(foreach variant,$($(platform)_variants),$(call unpacked_files_for_platform_and_variant,$(platform),$(variant))))

unpacked_files_vinyl := $(foreach platform,$(new_platforms_vinyl),$(foreach variant,$($(platform)_variants),$(call unpacked_files_for_platform_and_variant,$(platform),$(variant))))

# These are the folders into which we temporarily unzip everything
unpacked_folders := $(foreach platform,$(new_platforms),$(foreach variant,$($(platform)_variants),$(call unpacked_folder_for_platform_and_variant,$(platform),$(variant))))

unpacked_folders_bbcfg := $(foreach platform,$(new_platforms_bbcfg),$(foreach variant,$($(platform)_variants),$(call unpacked_folder_for_platform_and_variant,$(platform),$(variant))))

unpacked_folders_vinyl := $(foreach platform,$(new_platforms_vinyl),$(foreach variant,$($(platform)_variants),$(call unpacked_folder_for_platform_and_variant,$(platform),$(variant))))


# Compositing and Rule for the individual zipfiles and info files
# File Format $platform.$variant.bbfw
#             $platform.$variant.plist

# These are the resulting firmware files
firmware_files := $(addprefix $(output_img_dir)/,$(foreach platform,$(new_platforms),$(call firmware_filenames_for_platform,$(platform))))
firmware_files_bbcfg := $(addprefix $(output_img_dir)/,$(foreach platform,$(new_platforms_bbcfg),$(call firmware_filenames_for_platform,$(platform))))
firmware_files_vinyl := $(addprefix $(output_img_dir)/,$(foreach platform,$(new_platforms_vinyl),$(call firmware_filenames_for_platform,$(platform))))

# These are the resulting update files
update_files := $(addprefix $(output_img_dir)/,$(foreach platform,$(new_platforms),$(call update_filenames_for_platform,$(platform))))
update_files_bbcfg := $(addprefix $(output_img_dir)/,$(foreach platform,$(new_platforms_bbcfg),$(call update_filenames_for_platform,$(platform))))
update_files_vinyl := $(addprefix $(output_img_dir)/,$(foreach platform,$(new_platforms_vinyl),$(call update_filenames_for_platform,$(platform))))

# These are the resulting measured files
measured_files := $(addprefix $(output_img_dir)/,$(foreach platform,$(new_platforms),$(call measured_filenames_for_platform,$(platform))))
measured_files_bbcfg := $(addprefix $(output_img_dir)/,$(foreach platform,$(new_platforms_bbcfg),$(call measured_filenames_for_platform,$(platform))))
measured_files_vinyl := $(addprefix $(output_img_dir)/,$(foreach platform,$(new_platforms_vinyl),$(call measured_filenames_for_platform,$(platform))))

# Options files to be inserted into each bbfw. 
options_files := $(foreach platform,$(new_platforms),$(call options_files_for_platform,$(platform)))
options_files_bbcfg := $(foreach platform,$(new_platforms_bbcfg),$(call options_files_for_platform,$(platform)))
options_files_vinyl := $(foreach platform,$(new_platforms_vinyl),$(call options_files_for_platform,$(platform)))

################################################################
# INTERNAL RULES
################################################################

# Rule for unpacked folders with files in them
$(unpacked_folders): platform = 	$(call platform_for_file,$(@F))
$(unpacked_folders): variant  = 	$(call variant_for_filename,$(@F))
$(unpacked_folders): zip_file = 	$(call raw_zip_file_for_platform_and_variant,$(platform),$(variant))
$(unpacked_folders): $(raw_zip_files)
	@echo "====================================="
	@echo "UNZIPPING INTO $@ using $(zip_file) for $(platform):$(variant)"
	@echo "====================================="
	@mkdir -p $@
	@unzip -d $@ $(zip_file)

$(unpacked_folders_bbcfg): platform = 	$(call platform_for_file_bbcfg,$(@F))
$(unpacked_folders_bbcfg): variant  = 	$(call variant_for_filename,$(@F))
$(unpacked_folders_bbcfg): zip_file = 	$(call raw_zip_file_for_platform_and_variant,$(platform),$(variant))
$(unpacked_folders_bbcfg): bbcfg_file= $(call get_bbcfg_file_for_platform_and_variant,$(platform),$(variant))
$(unpacked_folders_bbcfg): $(raw_zip_files_bbcfg)
	@echo "====================================="
	@echo "UNZIPPING INTO $@ using $(zip_file) for $(platform):$(variant)"
	@echo "====================================="
	@mkdir -p $@
	@unzip -d $@ $(zip_file)
	# unzip the bbcfg root in same directory
	@echo "====================================="
	@echo "UNZIPPING BBCFG INTO $@ using $(bbcfg_file) for $(platform):$(variant)"
	@echo "====================================="
	@unzip -d $@ $(bbcfg_file)
	# renaming file for measure_bbfw - TODO create per-project files in 15580802
	@echo "using ALL.mbn from bbcfg project"
	@cp $@/*ALL.mbn $@/bbcfg.mbn
	@rm $@/*CFG*.mbn

$(unpacked_folders_vinyl): platform = 	$(call platform_for_file_vinyl,$(@F))
$(unpacked_folders_vinyl): variant  = 	$(call variant_for_filename,$(@F))
$(unpacked_folders_vinyl): zip_file = 	$(call raw_zip_file_for_platform_and_variant,$(platform),$(variant))
$(unpacked_folders_vinyl): bbcfg_file= $(call get_bbcfg_file_for_platform_and_variant,$(platform),$(variant))
$(unpacked_folders_vinyl): vinyl_files= $(call get_vinyl_files_for_platform_and_variant,$(platform),$(variant))
$(unpacked_folders_vinyl): $(raw_zip_files_vinyl)
	@echo "====================================="
	@echo "UNZIPPING INTO $@ using $(zip_file) for $(platform):$(variant)"
	@echo "====================================="
	@mkdir -p $@
	@unzip -d $@ $(zip_file)
	# unzip the bbcfg root in same directory
	@echo "====================================="
	@echo "UNZIPPING BBCFG INTO $@ using $(bbcfg_file) for $(platform):$(variant)"
	@echo "====================================="
	@unzip -d $@ $(bbcfg_file)
	# renaming file for measure_bbfw - TODO create per-project files in 15580802
	@echo "using ALL.mbn from bbcfg project"
	@cp $@/*ALL.mbn $@/bbcfg.mbn
	@rm $@/*CFG*.mbn
	@echo "====================================="
	@echo "COPYING VINYL INTO $@ using $(vinyl_files) for $(platform):$(variant)"
	@echo "====================================="
	@cp -r $(vinyl_files) $@

# Rule for generating the actual bbfw file
$(firmware_files): platform = 	$(call platform_for_file,$(@F))
$(firmware_files): variant  = 	$(call variant_for_filename,$(@F))
$(firmware_files): unpacked = 	$(call unpacked_files_for_platform_and_variant,$(platform),$(variant))
$(firmware_files): options  = 	$(call options_file_for_platform_and_variant,$(platform),$(variant))
$(firmware_files): $(unpacked_folders) $(options_files)
	@mkdir -p  $(@D)
	@echo "====================================="
	@echo "ZIPPING $@ for $(platform):$(variant)"
	@echo "====================================="
	$(zip) $@ $(unpacked) $(options)

$(firmware_files_bbcfg): platform = 	$(call platform_for_file_bbcfg,$(@F))
$(firmware_files_bbcfg): variant  = 	$(call variant_for_filename,$(@F))
$(firmware_files_bbcfg): unpacked = 	$(call unpacked_files_for_platform_and_variant,$(platform),$(variant))
$(firmware_files_bbcfg): options  = 	$(call options_file_for_platform_and_variant,$(platform),$(variant))
$(firmware_files_bbcfg): $(unpacked_folders_bbcfg) $(options_files_bbcfg)
	@mkdir -p  $(@D)
	@echo "====================================="
	@echo "ZIPPING $@ for $(platform):$(variant)"
	@echo "====================================="
	@echo "(unpacked)=$(unpacked)"
	$(zip) $@ $(unpacked) $(options)

$(firmware_files_vinyl): platform 			 = 	$(call platform_for_file_vinyl,$(@F))
$(firmware_files_vinyl): variant  			 = 	$(call variant_for_filename,$(@F))
$(firmware_files_vinyl): unpacked_folder  	 = 	$(call unpacked_folder_for_platform_and_variant,$(platform),$(variant))
$(firmware_files_vinyl): unpacked 			 =  $($(platform)_files)	
$(firmware_files_vinyl): options 		 	 = 	$(call options_file_for_platform_and_variant,$(platform),$(variant))
$(firmware_files_vinyl): $(unpacked_folders_vinyl) $(options_files_vinyl)
	@mkdir -p  $(@D)
	@echo "====================================="
	@echo "ZIPPING $@ for $(platform):$(variant)"
	@echo "====================================="
	@echo "(unpacked_folder)=$(unpacked_folder)"
	@echo "(unpacked)=$(unpacked)"
	@cd $(unpacked_folder) && zip -r /tmp/mav13-bbfw.bbfw $(unpacked) $($(platform)_vinyl_files) && cd ../../../
	$(zip) /tmp/mav13-bbfw.bbfw $(options)
	@mv /tmp/mav13-bbfw.bbfw $@

# Rule for copying firmware file to the update file
$(update_files): platform = 	$(call platform_for_file,$(@F))
$(update_files): variant  = 	$(call variant_for_filename,$(@F))
$(update_files): firmware = 	$(output_img_dir)/$(call firmware_filename_for_platform_and_variant,$(platform),$(variant))
$(update_files): $(firmware_files)
	@echo "====================================="
	@echo "COPYING $@ from $(firmware)"
	@echo "====================================="
	@cp $(firmware) $@

# Rule for copying firmware file to the update file
$(update_files_bbcfg): platform = 	$(call platform_for_file_bbcfg,$(@F))
$(update_files_bbcfg): variant  = 	$(call variant_for_filename,$(@F))
$(update_files_bbcfg): firmware = 	$(call firmware_filename_for_platform_and_variant,$(platform),$(variant))
$(update_files_bbcfg): $(firmware_files_bbcfg)
	@echo "====================================="
	@echo "COPYING bbcfg $@ from $(output_img_dir)/$(firmware)"
	@echo "====================================="
	@cp $(output_img_dir)/$(firmware) $@

$(update_files_vinyl): platform = 	$(call platform_for_file_vinyl,$(@F))
$(update_files_vinyl): variant  = 	$(call variant_for_filename,$(@F))
$(update_files_vinyl): firmware = 	$(call firmware_filename_for_platform_and_variant,$(platform),$(variant))
$(update_files_vinyl): $(firmware_files_vinyl)
	@echo "====================================="
	@echo "COPYING bbcfg $@ from $(output_img_dir)/$(firmware)"
	@echo "====================================="
	@cp $(output_img_dir)/$(firmware) $@


# Rule for generating the plist file
$(measured_files): platform = 	$(call platform_for_file,$(@F))
$(measured_files): variant  = 	$(call variant_for_filename,$(@F))
$(measured_files): zip_file = 	$(call raw_zip_file_for_platform_and_variant,$(platform),$(variant))
$(measured_files): $(raw_zip_files)
	@echo "====================================="
	@echo "MEASURING for $@ using $(zip_file) for $(platform):$(variant)"
	@echo "====================================="
	$(measure_bbfw) -i $(zip_file) -o $@
	@echo "Converting plist to binary"
	plutil -convert binary1 $@

$(measured_files_bbcfg): platform = 	$(call platform_for_file_bbcfg,$(@F))
$(measured_files_bbcfg): variant  = 	$(call variant_for_filename,$(@F))
$(measured_files_bbcfg): zip_file = 	$(output_img_dir)/$(call firmware_filename_for_platform_and_variant,$(platform),$(variant))
$(measured_files_bbcfg): $(raw_zip_files_bbcfg)
	@echo "====================================="
	@echo "MEASURING for $@ using $(zip_file) for $(platform):$(variant)"
	@echo "====================================="
	$(measure_bbfw) -i $(zip_file) -o $@
	@echo "Converting plist to binary"
	plutil -convert binary1 $@

$(measured_files_vinyl): platform = 	$(call platform_for_file_vinyl,$(@F))
$(measured_files_vinyl): variant  = 	$(call variant_for_filename,$(@F))
$(measured_files_vinyl): zip_file = 	$(output_img_dir)/$(call firmware_filename_for_platform_and_variant,$(platform),$(variant))
$(measured_files_vinyl): $(raw_zip_files_vinyl)
	@echo "====================================="
	@echo "MEASURING for $@ using $(zip_file) for $(platform):$(variant)"
	@echo "====================================="
	$(measure_bbfw) -i $(zip_file) -o $@
	@echo "Converting plist to binary"
	plutil -convert binary1 $@


.PHONY: build_prereqs
build_prereqs: $(latest_files) $(raw_zip_files) $(options_files) $(latest_files_bbcfg) $(raw_zip_files_bbcfg) $(options_files_bbcfg) $(latest_files_vinyl) $(raw_zip_files_vinyl) $(options_files_vinyl)

################################################################
# Entrypoints

.PHONY: new_clean
new_clean:
	@echo "====================================="
	@echo "CLEANING"
	@echo "====================================="
	rm -f $(measured_files) $(firmware_files) $(measured_files_bbcfg) $(firmware_files_bbcfg) $(measured_files_vinyl) $(firmware_files_vinyl)
	rm -rf $(unpacked_folders) $(unpacked_folders_bbcfg) $(unpacked_folders_vinyl)
	rm -rf $(intermediate_dir) $(output_img_dir)

.PHONY: new_build
new_build: build_prereqs $(firmware_files) $(update_files) $(measured_files) $(firmware_files_bbcfg) $(update_files_bbcfg) $(measured_files_bbcfg) $(firmware_files_vinyl) $(update_files_vinyl) $(measured_files_vinyl)

.PHONY: new_install
new_install: new_build
	@echo "====================================="
	@echo "INSTALLING INTO $(DSTROOT)/$(FIRMWARE_PATH_RESTORE)/"
	@echo "====================================="
	@ditto $(firmware_files) $(measured_files) $(firmware_files_bbcfg) $(measured_files_bbcfg) $(firmware_files_vinyl) $(measured_files_vinyl)  $(DSTROOT)/$(FIRMWARE_PATH_RESTORE)/
	@echo "====================================="
	@echo "INSTALLING INTO $(DSTROOT)/$(FIRMWARE_PATH_RESTORE_UPDATE)/"
	@echo "====================================="
	@ditto $(update_files) $(update_files_bbcfg) $(update_files_vinyl) $(DSTROOT)/$(FIRMWARE_PATH_RESTORE_UPDATE)/

ifneq ($(missing_stuff),)
    $(error $(missing_stuff))
endif
