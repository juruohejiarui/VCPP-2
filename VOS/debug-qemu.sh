ovmfPath=/usr/share/edk2/ovmf/OVMF_CODE.fd
usbVendor=0x21c4
usbProduct=0x0cd1

qemu-system-x86_64 \
	-drive file="${ovmfPath}",if=pflash,format=raw,unit=0,readonly=on \
	-device qemu-xhci \
	-net none \
	-device usb-host,vendorid=${usbVendor},productid=${usbProduct},id=hostdev0 \
	-m 2048M \
	-smp 2	\
