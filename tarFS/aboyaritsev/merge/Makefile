CURRENT = $(shell uname -r)
KDIR = /lib/modules/$(CURRENT)/build
PWD = $(shell pwd)
TARGET1 = fstar
OBJS =  module_fs.o tarparser.o super.o inode.o file.o address_space.o block.o
obj-m	:= $(TARGET1).o
$(TARGET1)-objs := $(OBJS)
default:	
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	@rm -f *.o .*.cmd .*.flags *.mod.c *.order
	@rm -f .*.*.cmd *~ *.*~ TODO.*
	@rm -fR .tmp*
	@rm -rf .tmp_versions
	@rm *.ko *.symvers