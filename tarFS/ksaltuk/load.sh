losetup /dev/loop0 ../tar/archieve.tar
insmod ./tarfs.ko
mount -t tarfs /dev/loop0 ./dir/
