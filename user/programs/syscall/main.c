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
    if (u1 != u2)
      return u1 - u2;
    if (u1 == '\0')
      return 0;
  }
  return 0;
}

int atoi(char *str) {
  int res = 0;

  for (int i = 0; str[i] != '\0'; ++i)
    res = res * 10 + str[i] - '0';

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

  while (*s++)
    n++;
  return n;
}

void showimage() {
  unsigned int __attribute__((aligned(16))) mbox[36];
  unsigned int width, height, pitch, isrgb; /* dimensions and channel order */
  unsigned char *lfb;                       /* raw frame buffer address */

  mbox[0] = 35 * 4;
  mbox[1] = MBOX_REQUEST;

  mbox[2] = 0x48003; // set phy wh
  mbox[3] = 8;
  mbox[4] = 8;
  mbox[5] = 1024; // FrameBufferInfo.width
  mbox[6] = 768;  // FrameBufferInfo.height

  mbox[7] = 0x48004; // set virt wh
  mbox[8] = 8;
  mbox[9] = 8;
  mbox[10] = 1024; // FrameBufferInfo.virtual_width
  mbox[11] = 768;  // FrameBufferInfo.virtual_height

  mbox[12] = 0x48009; // set virt offset
  mbox[13] = 8;
  mbox[14] = 8;
  mbox[15] = 0; // FrameBufferInfo.x_offset
  mbox[16] = 0; // FrameBufferInfo.y.offset

  mbox[17] = 0x48005; // set depth
  mbox[18] = 4;
  mbox[19] = 4;
  mbox[20] = 32; // FrameBufferInfo.depth

  mbox[21] = 0x48006; // set pixel order
  mbox[22] = 4;
  mbox[23] = 4;
  mbox[24] = 1; // RGB, not BGR preferably

  mbox[25] = 0x40001; // get framebuffer, gets alignment on request
  mbox[26] = 8;
  mbox[27] = 8;
  mbox[28] = 4096; // FrameBufferInfo.pointer
  mbox[29] = 0;    // FrameBufferInfo.size

  mbox[30] = 0x40008; // get pitch
  mbox[31] = 4;
  mbox[32] = 4;
  mbox[33] = 0; // FrameBufferInfo.pitch

  mbox[34] = MBOX_TAG_LAST;

  // this might not return exactly what we asked for, could be
  // the closest supported resolution instead
  if (mbox_call(MBOX_CH_PROP, mbox) && mbox[20] == 32 && mbox[28] != 0) {
    mbox[28] &= 0x3FFFFFFF; // convert GPU address to ARM address
    width = mbox[5];        // get actual physical width
    height = mbox[6];       // get actual physical height
    pitch = mbox[33];       // get number of bytes per line
    isrgb = mbox[24];       // get the actual channel order
    lfb = (void *)((unsigned long)mbox[28]);
  } else {
    puts("Unable to set screen resolution to 1024x768x32\n");
  }

  int x, y;

  while (1) {

    for (int frame_id = 0; frame_id < img_frames; frame_id++) {
      unsigned char *ptr = lfb;

      char *data = header_data + frame_id * img_width * img_height * 4,
           pixel[4];

      ptr += (height - img_height) / 2 * pitch + (width - img_width) * 2;

      for (y = 0; y < img_height; y++) {
        for (x = 0; x < img_width; x++) {
          HEADER_PIXEL(data, pixel);
          // the image is in RGB. So if we have an RGB framebuffer, we
          // can copy the pixels directly, but for BGR we must swap R
          // (pixel[0]) and B (pixel[2]) channels.

          *((unsigned int *)ptr) =
              isrgb ? *((unsigned int *)&pixel)
                    : (unsigned int)(pixel[0] << 16 | pixel[1] << 8 | pixel[2]);
          ptr += 4;
        }
        ptr += pitch - img_width * 4;
      }
      wait_msec(100);
    }
  }
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
    printf("This is user process\n"
           "help:                  Print this help message.\n"
           "exec:                  Exec vm.img.\n"
           "pid:                   Show pid of current process.\n"
           "fork:                  Fork a child process that plays awesome "
           "video.\n"
           "kill [pid]:            Kill process by pid.\n"
           "signal_kill [pid]:     Kill process by pid with signal(Advanced).\n"
           "register:              Regist signal handler(Advanced).\n"
           "demand:                Demand paging.\n"
           "mmap_r:                mmap test 1.\n"
           "mmap_w:                mmap test 2.\n"
           "exit:                  Quit.\n");
  } else if (!strcmp(cmd, "pid")) {
    int pid = getpid();
    printf("Pid is: %d\n", pid);
  } else if (!strcmp(cmd, "exec")) {
    exec("vm.img", 0);
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
  } else if(!strcmp(cmd, "mmap_r")) {
    char* ptr = mmap(0x1000, 4096, 0x1, 0x20);
    printf("addr: %x\n", ptr);
    printf("%d\n", ptr[1000]); // should be 0
    printf("%d\n", ptr[4097]); // should be segfault
  } else if(!strcmp(cmd, "mmap_w")){
    char* ptr = mmap(NULL, 4096, 0x1, 0x20);
    printf("addr: %x\n", ptr);
    printf("%d\n", ptr[1000]); // should be 0
    ptr[0] = 1; // should be seg fault
    printf("%d\n", ptr[0]); // not reached
  } else if (!strncmp(cmd, "jump", 4)) {
    asm volatile("mov x0, 0x0");
    asm volatile("br x0");
  } else if (!strcmp(cmd, "demand")) {
    blow_stack();
  } else {
    printf("Not a vaild command!\n");
  }
}

int main() {
  int pid = getpid();
  printf("Process start, pid is: %d\n", pid);

  while (1)
    shell();
}
