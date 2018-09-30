##############################################################################
# Baseband Firmware Builder
#
# Copyright 2010 Apple Inc. All rights reserved.
##############################################################################

##############################################################################
# Over-ridable variables from the command line
##############################################################################

#
# Output directory for firmware images
#
OUTPUT_DIR = build

#
# Xcode configuration for the FLS tools
#
XCODE_CONFIG = Release

#
# Install location for the firmware files
#
FIRMWARE_PATH = usr/local/standalone/firmware
FIRMWARE_PATH2 = usr/standalone/firmware
FIRMWARE_PATH_RESTORE = usr/local/standalone/firmware
FIRMWARE_PATH_RESTORE_UPDATE = usr/standalone/update
FIRMWARE_PATH_BRINGUP = usr/standalone/firmware/Baseband
PLATFORM_DIR = $(shell xcodebuild -version PlatformPath -sdk iphoneos)

##############################################################################
# Internal variables
##############################################################################

intermediate_dir = $(OUTPUT_DIR)/bbfw.intermediate
output_img_dir = $(OUTPUT_DIR)/bbfw.image
sdk_version=$(shell xcodebuild -version -sdk iphoneos PlatformVersion)
sdk_tools_path = $(shell xcodebuild -version -sdk iphoneos$(sdk_version).internal PlatformPath)/usr/local/bin
measure_bbfw = $(sdk_tools_path)/measure_bbfw
traits_dir=traits
options_file=Options.plist


ifeq ($(MAKECMDGOALS),)
  action=build
else
  action=$(MAKECMDGOALS)
endif

old_platforms=ice3 eur eur2 eur3
new_platforms_all=mav5 mav7_mav8 mav7_mav8_old 
bbcfg_platforms=mav10

# NOTE: Vinyl platforms imply bbcfg support
vinyl_platforms=mav13

includes=$(addsuffix .mak,$(old_platforms) $(new_platforms_all) $(bbcfg_platforms) $(vinyl_platforms))
build_targets=$(addsuffix _build,$(old_platforms))
clean_targets=$(addsuffix _clean,$(old_platforms))
install_targets=$(addsuffix _install,$(old_platforms))

##############################################################################
# Includes
##############################################################################

include $(includes)
include rules.mak


##############################################################################
# Build rules
##############################################################################

.PHONY: build
build: $(build_targets) new_build

.PHONY: clean
clean: $(clean_targets) new_clean
	
.PHONY: install
install: $(install_targets) new_install

.PHONY: force
force: ;

.PHONY: installhdrs
installhdrs:
	@echo "Not applicable"

.PHONY:installsrc
installsrc:
	ditto . $(SRCROOT)
