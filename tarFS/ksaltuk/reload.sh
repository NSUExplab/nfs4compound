umount ./dir/
rmmod tarfs
insmod ./tarfs.ko
mount -t tarfs /dev/loop0 ./dir/
