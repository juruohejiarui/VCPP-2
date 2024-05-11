usbipd.exe attach -w Ubuntu-22.04 --busid 5-1
sleep 2s
. ./install-EFI.sh

usbipd.exe detach --busid 5-1