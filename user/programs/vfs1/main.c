#include "img.h"
#include "mbox.h"
#include "stdio.h"
#include "string.h"
#include "syscall.h"

void wait_msec(unsigned int n) {
  register unsigned long f, t, r;
  // get the current counter frequency
  asm volatile("mrs %0, cntfrq_el0" : "=r"(f));
  // read the current counter
  asm volatile("mrs %0, cntpct_el0" : "=r"(t));
  // calculate expire value for counter
  // t += ((f / 1000) * n) / 1000;
  t += f * n / 1000;
  do {
    asm volatile("mrs %0, cntpct_el0" : "=r"(r));
  } while (r < t);
}

void blow_stack() {
  char arr[4096];
  arr[0] = '\0';
  arr[4095] = '\0';
  asm volatile("" : : "r"(arr) : "memory");
}

int strncmp(const char *s1, const char *s2, register size_t n) {
  register unsigned char u1, u2;
  while (n-- > 0) {
    u1 = (unsigned char)*s1++;
    u2 = (unsigned char)*s2++;
    if (u1 != u2) return u1 - u2;
    if (u1 == '\0') return 0;
  }
  return 0;
}

int atoi(char *str) {
  int res = 0;

  for (int i = 0; str[i] != '\0'; ++i) res = res * 10 + str[i] - '0';

  return res;
}

void read_string(char *cmd) {
  char now;
  cmd[0] = 0;
  int now_cur = 0;
  while ((now = getchar()) != '\n') {
    if ((int)now == 240) {
      continue;
    } else if (now == 127) {
      now_cur -= 1;
      if (now_cur >= 0) {
        printf("\b \b");
        now_cur -= 1;
      }
    } else {
      cmd[now_cur] = now;
      printf("%c", now);
    }
    now_cur++;
  }
  printf("\n");
  cmd[now_cur] = 0;
}

int strcmp(const char *s1, const char *s2) {
  int value;

  s1--, s2--;
  do {
    s1++, s2++;
    if (*s1 == *s2) {
      value = 0;
    } else if (*s1 < *s2) {
      value = -1;
      break;
    } else {
      value = 1;
      break;
    }
  } while (*s1 != 0 && *s2 != 0);
  return value;
}

long _strlen(char *s) {
  long n = 0;

  while (*s++) n++;
  return n;
}

struct framebuffer_info {
  unsigned int width;
  unsigned int height;
  unsigned int pitch;
  unsigned int isrgb;
};

void showimage() {
  int fb = open("/dev/framebuffer", 0);
  if (fb < 0) {
    printf("open framebuffer failed\n");
    exit(0);
  }

  struct framebuffer_info fb_info;
  // {.width=1024, .height=768, .pitch=4096, .isrgb=1};
  fb_info.width = 1024;
  fb_info.height = 768;
  fb_info.pitch = 4096;
  fb_info.isrgb = 1;

  ioctl(fb, 0, &fb_info);
  

  int x, y;

  while (1) {
    for (int frame_id = 0; frame_id < img_frames; frame_id++) {
      long offset = 0;

      char *data = header_data + frame_id * img_width * img_height * 4;
      char pixel[4];

      offset += (fb_info.height - img_height) / 2 * fb_info.pitch +
                (fb_info.width - img_width) * 2;

      for (y = 0; y < img_height; y++) {
        lseek64(fb, offset, SEEK_SET);
        for (x = 0; x < img_width; x++) {
          HEADER_PIXEL(data, pixel);
          // the image is in RGB. So if we have an RGB framebuffer, we
          // can copy the pixels directly, but for BGR we must swap R
          // (pixel[0]) and B (pixel[2]) channels.
          // *((unsigned int *)ptr) =
          //     isrgb ? *((unsigned int *)&pixel)
          //           : (unsigned int)(pixel[0] << 16 | pixel[1] << 8 |
          //           pixel[2]);
          // ptr += 4;
          unsigned int color =
              fb_info.isrgb
                  ? *((unsigned int *)&pixel)
                  : (unsigned int)(pixel[0] << 16 | pixel[1] << 8 | pixel[2]);
          write(fb, &color, 4);
          offset += 4;
        }
        // ptr += pitch - img_width * 4;
        offset += fb_info.pitch - img_width * 4;
      }
      wait_msec(100);
    }
  }
}

void vfs_test() {
  int fd = 0;
  char buf[64];
  long len;

  printf("\nBasic 1\ntmpfile: ");
  fd = open("/tmpfile", O_CREAT);
  //printf("fd: %d\n", fd);
  len = write(fd, "tmpfile test", 12);
  //printf("wlen: %ld\n", len);
  close(fd);
  fd = open("/tmpfile", 0);
  //printf("fd: %d\n", fd);
  len = read(fd, buf, 64);
  //printf("rlen: %ld\n", len);
  close(fd);
  len = len < 0 ? 0 : len;
  buf[len] = 0;
  if(strcmp(buf, "tmpfile test") == 0) {
    printf("success\n");
  } else {
    printf("failed\n");
  }

  printf("\nBasic 2\ntmpdir: ");
  mkdir("/tmp", 0);
  fd = open("/tmp/tmpfile", O_CREAT);
  //printf("fd: %d\n", fd);
  len = write(fd, "tmpdir test", 11);
  //printf("wlen: %ld\n", len);
  close(fd);
  fd = open("/tmp/tmpfile", 0);
  //printf("fd: %d\n", fd);
  len = read(fd, buf, 64);
  //printf("rlen: %ld\n", len);
  close(fd);
  len = len < 0 ? 0 : len;
  buf[len] = 0;
  if(strcmp(buf, "tmpdir test") == 0) {
    printf("success\n");
  } else {
    printf("failed\n");
  }

  printf("mount: ");
  mount(NULL, "/tmp", "tmpfs", 0, NULL);
  fd = open("/tmp/tmpfile", O_CREAT);
  //printf("fd: %d\n", fd);
  len = write(fd, "mnt test", 8);
  //printf("wlen: %ld\n", len);
  close(fd);
  fd = open("/tmp/tmpfile", 0);
  //printf("fd: %d\n", fd);
  len = read(fd, buf, 64);
  //printf("rlen: %ld\n", len);
  close(fd);
  if(len == 8 && strncmp(buf, "mnt test", 8) == 0) {
    printf("success\n");
  } else {
    printf("failed\n");
  }

  printf("\nBasic 3\n");
  chdir("/tmp");
  fd = open("./lookup1", O_CREAT);
  // printf("fd: %d\n", fd);
  len = write(fd, "lookup1 test", 12);
  // printf("wlen: %ld\n", len);
  close(fd);
  fd = open("../lookup2", O_CREAT);
  // printf("fd: %d\n", fd);
  len = write(fd, "lookup2 test", 12);
  // printf("wlen: %ld\n", len);
  close(fd);
  chdir("/");
  fd = open("./tmp/lookup1", 0);
  // printf("fd: %d\n", fd);
  len = read(fd, buf, 64);
  // printf("rlen: %ld\n", len);
  close(fd);
  len = len < 0 ? 0 : len;
  buf[len] = 0;
  printf("lookup1: ");
  if(strcmp(buf, "lookup1 test") == 0) {
    printf("success\n");
  } else {
    printf("failed\n");
  }
  fd = open("./lookup2", 0);
  // printf("fd: %d\n", fd);
  len = read(fd, buf, 64);
  // printf("rlen: %ld\n", len);
  close(fd);
  len = len < 0 ? 0 : len;
  buf[len] = 0;
  printf("lookup2: ");
  if(strcmp(buf, "lookup2 test") == 0) {
    printf("success\n");
  } else {
    printf("failed\n");
  }

  printf("\nBasic 4\ninitramfs: ");
  fd = open("/initramfs/vfs1.img", 0);
  //printf("fd: %d\n", fd);
  len = read(fd, buf, 64);
  //printf("rlen: %ld\n", len);
  close(fd);
  if(len == 64 && *(unsigned int *)(buf + 4) == 0xd503201f) {
    printf("success\n");
  } else {
    printf("failed\n");
  }

  printf("\nAdvance 1\nuart, you should see stdin, stdout, stderr\n");
  write(1, "uart stdin\n", 11);
  printf("stdin (should block), press enter: ");
  read(0, buf, 1);
  write(2, "\nuart stderr\n", 12);
}

void kill_print() {
  printf("\nYou can't kill gura!!!!!\n");
  wait_msec(3000);
  printf("\nOh Nyo!!!!\n");
}

void shell() {
  printf("# ");
  char cmd[256];
  read_string(cmd);
  if (!strcmp(cmd, "help")) {
    printf(
        "This is user process\n"
        "help:                  Print this help message.\n"
        "exec:                  Exec vfs1.img.\n"
        "pid:                   Show pid of current process.\n"
        "fork:                  Fork a child process that plays awesome "
        "video.\n"
        "kill [pid]:            Kill process by pid.\n"
        "signal_kill [pid]:     Kill process by pid with signal(Advanced).\n"
        "register:              Regist signal handler(Advanced).\n"
        "demand:                Demand paging.\n"
        "mmap_r:                mmap test 1.\n"
        "mmap_w:                mmap test 2.\n"
        "vfs:                   vfs test.\n"
        "exit:                  Quit.\n");
  } else if (!strcmp(cmd, "pid")) {
    int pid = getpid();
    printf("Pid is: %d\n", pid);
  } else if (!strcmp(cmd, "exec")) {
    exec("/initramfs/vfs1.img", 0);
  } else if (!strcmp(cmd, "fork")) {
    int child_pid = fork();
    if (child_pid == 0) {
      showimage(0);
    } else {
      printf("Child pid: %d\n", child_pid);
    }
  } else if (!strcmp(cmd, "exit")) {
    exit(0);
  } else if (!strncmp(cmd, "kill", 4)) {
    int pid = atoi(cmd + 5);
    kill(pid);
  } else if (!strncmp(cmd, "signal_kill", 11)) {
    int pid = atoi(cmd + 12);
    p_signal(pid, 9);
  } else if (!strcmp(cmd, "register")) {
    register_posix(9, kill_print);
  } else if (!strcmp(cmd, "check")) {
    int el;
    asm volatile("mrs %0, CurrentEL" : "=r"(el));
    printf("Current: %d\n", el >> 2);
  } else if (!strcmp(cmd, "mmap_r")) {
    char *ptr = mmap(0x1000, 4096, 0x1, 0x20 | 0x08000);
    printf("addr: %x\n", ptr);
    printf("%d\n", ptr[1000]);  // should be 0
    printf("%d\n", ptr[4097]);  // should be segfault
  } else if (!strcmp(cmd, "mmap_w")) {
    char *ptr = mmap(NULL, 4096, 0x1, 0x20 | 0x08000);
    printf("addr: %x\n", ptr);
    printf("%d\n", ptr[1000]);  // should be 0
    ptr[0] = 1;                 // should be seg fault
    printf("%d\n", ptr[0]);     // not reached
  } else if (!strncmp(cmd, "jump", 4)) {
    asm volatile("mov x0, 0x0");
    asm volatile("br x0");
  } else if (!strcmp(cmd, "demand")) {
    blow_stack();
  } else if (!strcmp(cmd, "vfs")) {
    vfs_test();
  } else {
    printf("Not a vaild command!\n");
  }
}

int main() {
  int pid = getpid();
  printf("Process start, pid is: %d\n", pid);

  while (1) shell();
}
