qemu-system-x86_64 \
	-drive file=/usr/share/ovmf/OVMF.fd,if=pflash,format=raw,unit=0,readonly=on \
	-device qemu-xhci \
	-net none \
	-device usb-host,hostbus=2,hostaddr=4,id=hostdev0 \
	-m 4096M \
	
