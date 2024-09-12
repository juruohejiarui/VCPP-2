BUSID='3-4'
usbipd.exe attach -w Ubuntu-22.04 --busid ${BUSID}
if [ $? -ne 0 ];then
	echo -e "${RED_COLOR}==usbipd attach failed!==${RESET}"
else
	. ./install-EFI.sh
	usbipd.exe detach --busid ${BUSID}  
fi
