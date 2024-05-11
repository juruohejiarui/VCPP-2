usbipd.exe attach -w Ubuntu-22.04 --busid 2-19
if [ $? -ne 0 ];then
	echo -e "${RED_COLOR}==usbipd attach failed!==${RESET}"
else
	. ./install-EFI.sh
	usbipd.exe detach --busid 2-19
fi