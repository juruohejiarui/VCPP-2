RED_COLOR='\E[1;31m'
RESET='\E[0m'

echo -e "${RED_COLOR}=== gen kernel.bin ===${RESET}"
cd kernel
python3 ./rely.py
make 

if [ $? -ne 0 ];then
    echo -e "${RED_COLOR}==kernel make failed!Please checkout first!==${RESET}"
    cd ../
else
    cd ../
    echo -e "${RED_COLOR}=== copying files ===${RESET}"
    sudo mount /dev/sdb1 /mnt/
    sudo cp ./BootLoader.efi /mnt/EFI/BOOT/bootx64.efi
    sudo cp ./kernel/kernel.bin /mnt/
    sudo sync
    sudo umount /mnt/

    # sudo vmware
fi
