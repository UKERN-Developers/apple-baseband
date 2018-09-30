if [ "$1" = "" ]
then
	prefix=""
else
	prefix="$1_"
fi

sdk_folder=`psw_vers | grep -e  "^Path\:" | awk '{ print $2 }'`

mkdir -p /tmp/bbfwbuilder
make $prefix"clean"  DSTROOT=/tmp/bbfwbuilder STACK_SRCROOT=$sdk_folder
make $prefix"build" $prefix"install" DSTROOT=/tmp/bbfwbuilder STACK_SRCROOT=$sdk_folder

echo "********* DONE *********"
