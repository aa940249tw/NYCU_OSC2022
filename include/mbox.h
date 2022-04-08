#ifndef __MBOX_H__
#define __MBOX_H__

extern volatile unsigned int mailbox[36];

#define MBOX_REQUEST    0

/* channels */
#define MBOX_CH_POWER   0
#define MBOX_CH_FB      1
#define MBOX_CH_VUART   2
#define MBOX_CH_VCHIQ   3
#define MBOX_CH_LEDS    4
#define MBOX_CH_BTNS    5
#define MBOX_CH_TOUCH   6
#define MBOX_CH_COUNT   7
#define MBOX_CH_PROP    8

/* tags */
#define MBOX_TAG_GETSERIAL  0x00010004
#define MBOX_TAG_LAST       0x00000000
#define GET_BOARD_REVISION  0x00010002
#define GET_ARM_ADDRESS  	0x00010005
#define REQUEST_CODE        0x00000000
#define REQUEST_SUCCEED     0x80000000
#define REQUEST_FAILED      0x80000001
#define TAG_REQUEST_CODE    0x00000000
#define END_TAG             0x00000000


int mailbox_call(unsigned char ch);
int __mbox_call(unsigned char, unsigned int *);
void get_revision();
unsigned int get_address();

#endif
