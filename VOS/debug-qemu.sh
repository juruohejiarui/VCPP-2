ovmfPath=/usr/share/edk2/ovmf/OVMF_CODE.fd
# usbVendor=0x21c4
# usbProduct=0x0cd1
usbVendor=0x17ef
usbProduct=0x3899

qemu-system-x86_64 \
	-drive file="${ovmfPath}",if=pflash,format=raw,unit=0,readonly=on \
	-machine type=q35 \
	-cpu max \
	-device qemu-xhci \
	-net none \
	-device pcie-root-port,bus=pcie.0,id=root_port \
	-device usb-host,vendorid=${usbVendor},productid=${usbProduct},id=hostdev0 \
	-m 2048M \
	-device virtio-keyboard \
	-smp 2,cores=2	\
