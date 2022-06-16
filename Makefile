TOOLCHAIN_PREFIX = aarch64-linux-gnu-
SRC_DIR = src
OBJ_DIR = obj

SRCS = $(wildcard $(SRC_DIR)/*.c)
ARMS = $(wildcard *.S)
#OBJS = $(SRCS:.c=.o)
ARMO = $(ARMS:.S=.o)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
CFLAGS = -Wall -O0 -g -ffreestanding -nostdinc -nostdlib -nostartfiles -mgeneral-regs-only -I include

all: clean cpio kernel8.img

%.o: %.S
	$(TOOLCHAIN_PREFIX)gcc $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(TOOLCHAIN_PREFIX)gcc $(CFLAGS) -c $< -o $@

kernel8.img: $(ARMO) $(OBJS)
	$(TOOLCHAIN_PREFIX)ld -nostdlib $(ARMO) $(OBJS) -T linker.ld -o kernel8.elf
	$(TOOLCHAIN_PREFIX)objcopy -O binary kernel8.elf kernel8.img

cpio:
	mkdir -p rootfs
	cd rootfs && find . | cpio -o -H newc > ../initramfs.cpio

clean:
	rm *.cpio kernel8.img kernel8.elf *.o >/dev/null 2>/dev/null || true
	rm $(OBJ_DIR)/*.o >/dev/null 2>/dev/null || true

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

run:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb  -drive if=sd,file=nctuos.img,format=raw
	
serial:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial pty -display none

debug:
	qemu-system-aarch64 -M raspi3b -kernel kernel8.img -serial null -serial stdio -display none -initrd initramfs.cpio -dtb bcm2710-rpi-3-b-plus.dtb  -drive if=sd,file=nctuos.img,format=raw -S -s