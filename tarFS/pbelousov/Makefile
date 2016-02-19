.PHONY: default
obj-m += tar_fs.o
tar_fs-objs = fs_module.o super.o inode.o tarparser.o file.o
ccflags-y := -std=gnu99 -Wno-declaration-after-statement
default:
	$(MAKE) -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules