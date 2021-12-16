TOOLCHAIN_PREFIX = aarch64-linux-gnu-
SRC_DIR = src
OBJ_DIR = obj

SRCS = $(wildcard $(SRC_DIR)/*.c)
#OBJS = $(SRCS:.c=.o)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRCS))
CFLAGS = -Wall -O2 -g -ffreestanding -nostdinc -nostdlib -nostartfiles -I include

all: clean kernel8.img

start.o: start.S
	$(TOOLCHAIN_PREFIX)gcc $(CFLAGS) -c start.S -o start.o

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(TOOLCHAIN_PREFIX)gcc $(CFLAGS) -c $< -o $@

kernel8.img: start.o $(OBJS)
	$(TOOLCHAIN_PREFIX)ld -nostdlib -nostartfiles start.o $(OBJS) -T linker.ld -o kernel8.elf
	$(TOOLCHAIN_PREFIX)objcopy -O binary kernel8.elf kernel8.img

clean:
	rm kernel8.img kernel8.elf *.o >/dev/null 2>/dev/null || true
	rm $(OBJ_DIR)/*.o >/dev/null 2>/dev/null || true

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

run:
	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -serial null -serial stdio -display none -initrd initramfs.cpio
	
serial:
	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -serial null -serial pty -display none

debug:
	qemu-system-aarch64 -M raspi3 -kernel kernel8.img -serial null -serial stdio -display none -initrd initramfs.cpio -S -s