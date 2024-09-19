RED_COLOR='\E[1;31m'
RESET='\E[0m'
GOAL_DISK="/dev/sdb1"

echo -e "${RED_COLOR}=== gen kernel.bin ===${RESET}"
cd kernel
python3 ./rely.py
make all

if [ $? -ne 0 ]; then
    echo -e "${RED_COLOR}==kernel make failed!Please checkout first!==${RESET}"
    cd ../
else
    cd ../
    echo -e "${RED_COLOR}=== copying files ===${RESET}"
	python3 waitForGoalDisk.py ${GOAL_DISK}
    if [ $? -eq 0 ]; then
        sudo mount ${GOAL_DISK} /mnt/
        sudo cp ./BootLoader2.efi /mnt/EFI/BOOT/BOOTX64.efi
        sudo cp ./kernel/kernel.bin /mnt/
        sudo sync
        sudo umount /mnt/
    fi
fi