sdk_path=$(xcodebuild -version -sdk iphoneos.internal Path 2>/dev/null)
echo using iOS SDK $sdk_path

targets="moots trek mav4 mav5 mav7_mav8 mav10 mav13"

# Convention:
# $(target)_base=Build base name
# $(target)_build=Folder for actual specific build
# $(target)_folders=Folders within build folder to copy from, leave unset if DMG
# $(target)_dmg=DMG file if delivered as DMG
# $(target)_dmg_fiolders_base=Folders within the DMG file to take

moots_base=/SWE/iOS/Images/iOSNonUI/Solitude
moots_build=$moots_base/CurrentSolitude
moots_folders=$moots_build/Phoenix/usr/local/standalone/firmware

trek_base=/SWE/iOS/Images/iOSNonUI/Majestic
trek_build=$trek_base/CurrentMajestic
trek_folders=$trek_build/TrekBaseBandFW/usr/standalone/firmware

# Mav4 breaks a conventions rules
mav4_base=/SWE/iOS/Images/iOSNonUI/Zulu
mav4_build=$mav4_base/CurrentZulu
mav4_dmg=$mav4_build/Mav4BaseBandFW.dmg
mav4_dmg_folders_base=usr/local/standalone/firmware

mav5_base=/SWE/iOS/Images/iOSNonUI/Beartooth
mav5_build=$mav5_base/CurrentBeartooth
mav5_folders="$mav5_build/Mav5BaseBandFW/usr/standalone/firmware $mav5_build/Mav5BaseBandFW_Debug/usr/standalone/firmware"

mav7_mav8_base=/SWE/iOS/Images/iOSNonUI/Alpenzoo
mav7_mav8_build=$mav7_mav8_base/CurrentAlpenzoo
mav7_mav8_folders="$mav7_mav8_build/Mav7Mav8BaseBandFW/usr/standalone/firmware $mav7_mav8_build/Mav7Mav8BaseBandFW_Debug/usr/standalone/firmware"

mav10_base=/SWE/iOS/Images/iOSNonUI/Ludlow
mav10_build=$mav10_base/CurrentLudlow
mav10_folders="$mav10_build/Mav10BaseBandFW/usr/standalone/firmware $mav10_build/Mav10BaseBandFW_Debug/usr/standalone/firmware"

mav13_base=/SWE/iOS/Images/iOSNonUI/Aprica
mav13_build=$mav13_base/Aprica7A178
mav13_folders="$mav13_build/Mav13BaseBandFW/usr/standalone/firmware $mav13_build/Mav13BaseBandFW_Debug/usr/standalone/firmware"

# copy bbcfg from Train's roots directory
if [ $# -lt 2 ];then
  echo "$0 <build_train_family> <build_train>, e.g., $0 OkemoFamily Blacktail "
  exit 1
fi
current_sdk_train=$2
echo "Detected train $current_sdk_train from SDK"
bbcfg_vers=`~rc/bin/getvers bbcfg -release $current_sdk_train`
bbcfg_root_dir=`ls -1tr /SWE/iOS/Views/$1/${current_sdk_train}/Roots/bbcfg/|grep "$bbcfg_vers.*root$"|tail -n1`
bbcfg_root=/SWE/iOS/Views/$1/${current_sdk_train}/Roots/bbcfg/$bbcfg_root_dir/usr/standalone/firmware/
bbcfg_files="$bbcfg_root/Mav10BaseBandCFG*.zip $bbcfg_root/Mav13BaseBandCFG*.zip"

vinyl_vers=`~rc/bin/getvers VinylFirmware -release $current_sdk_train`
vinyl_root=/SWE/iOS/Views/$1/${current_sdk_train}/Roots/VinylFirmware/$vinyl_vers*.root/usr/local/standalone/firmware/
vinyl_files="$vinyl_root/"

# hardcode legacy ICE stuff
legacy_source=/SWE/iOS/Images/iOSNonUI/Sundance/CurrentSundance/Restore/Firmware
legacy_dest=$sdk_path/usr/local/standalone/firmware
legacy_files="ICE2*.eep ICE2*.fls ICE3*.bin ICE3*.fls"

echo -e "using:\n\t$moots_build\n\t$trek_build\n\t$mav4_build\n\t$mav5_build\n\t$mav7_mav8_build\n\t$mav10_build\n\t$mav13_build\n\t$mav16_build\n"
echo "--------------------------------------------------"
echo "STARTING COPY FIRMWARE"
echo "--------------------------------------------------"

echo -e "\n"

for target in $targets
do
	target_folders=${target}_folders
	eval target_folders=\$$target_folders

	target_build=${target}_build
	eval target_build=\$$target_build

	for folder in $target_folders
	do
		echo "++++++++++++++++++++++++++++++++++++++++++++++++++"
		echo "Folder: '$folder'"
		echo "++++++++++++++++++++++++++++++++++++++++++++++++++"
	
		dest=$sdk_path/`dirname $folder | sed -e "s/.*\(\\/usr\\/.*\)/\1/g"`
		sudo mkdir -p $dest
		echo "sudo rsync -av --progress $folder $dest"
		sudo rsync -av --progress $folder $dest

		echo -e "\n\n"
	done

	target_dmg=${target}_dmg
	eval target_dmg=\$$target_dmg

	for dmg in $target_dmg
	do
		echo "++++++++++++++++++++++++++++++++++++++++++++++++++"
		echo "DMG: '$dmg'"
		echo "++++++++++++++++++++++++++++++++++++++++++++++++++"

		echo Mounting $dmg
		hdiutil attach  -noautoopen $dmg
		mountpoint=/Volumes/`ls -dl $target_build | awk '{print $NF}'`*
		mountpoint=`ls -d $mountpoint`
		echo Mountpoint $folder

		folder=${target}_dmg_folders_base
		eval folder=\$$folder
		folder=$mountpoint/$folder

		dest=$sdk_path/`dirname $folder | sed -e "s/.*\(\\/usr\\/.*\)/\1/g"`
		sudo mkdir -p $dest
		echo "sudo rsync -av --progress $folder $dest"
		sudo rsync -av --progress $folder $dest

		hdiutil detach $mountpoint

	done
done

for bbcfg in $bbcfg_files
do
  dest=$sdk_path/usr/standalone/firmware
  base=`basename $bbcfg`
  echo "++++++++++++++++++++++++++++++++++++++++++++++++++"
  echo "Copying BBCFG: '$base' to SDK"
  echo "++++++++++++++++++++++++++++++++++++++++++++++++++"

  echo "sudo rsync -av --progress $bbcfg $dest"
  sudo rsync -av --progress $bbcfg $dest

  echo -e "\n\n"
done

for vinyl in $vinyl_files
do
  dest=$sdk_path/usr/local/standalone/firmware
  base=`basename $vinyl`
  echo "++++++++++++++++++++++++++++++++++++++++++++++++++"
  echo "Copying VINYL: '$base' to SDK"
  echo "++++++++++++++++++++++++++++++++++++++++++++++++++"

  echo "sudo rsync -av --progress $vinyl $dest"
  sudo rsync -av --progress $vinyl $dest

  echo -e "\n\n"
done

pushd "$legacy_source"

echo "++++++++++++++++++++++++++++++++++++++++++++++++++"
echo "Copying LEGACY ICE files to SDK"
echo "++++++++++++++++++++++++++++++++++++++++++++++++++"

sudo mkdir -p $legacy_dest
echo "sudo rsync -av --progress $legacy_files $legacy_dest"
sudo rsync -av --progress $legacy_files $legacy_dest

popd

echo -e "\n\n"

echo "--------------------------------------------------"
echo "FINISHED"
echo "--------------------------------------------------"
