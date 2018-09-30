#!/usr/bin/env bash

./copy_firmware.sh MonarchFamily Eagle

~rc/bin/buildit . -rootsDirectory /tmp/myRoot -release Eagle -project BaseBandFWBuilder -buildTool `xcode-select -p`/usr/bin/make -target install -arch i386 -arch x86_64 -- STACK_SRCROOT=`xcodebuild -version -sdk iphoneos.internal Path`
