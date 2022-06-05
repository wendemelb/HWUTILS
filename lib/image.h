#ifndef _IMAGE_H_
#define _IMAGE_H_

#include <stdio.h>
#include <string.h>
#include "dvp.h"
#include "fpioa.h"
#include "lcd.h"
#include "ov5640.h"
#include "ov2640.h"
#include "plic.h"
#include "math.h"
#include "bsp.h"

#include "sysctl.h"
#include "uarths.h"
#include "nt35310.h"
#include "board_config.h"
#include "unistd.h"
#include "iomem.h"
#define MASKR(x) ((x&(0xf800))>>8)
#define MASKB(x) ((x&(0x001f))>>0)
#define MASKG(x) ((x&(0x07e0))>>4)

#define MASKR64(x) ((x&(0xf800f800f800f800))>>13)
#define MASKB64(x) ((x&(0x001f001f001f001f))>>2)
#define MASKG64(x) ((x&(0x07e007e007e007e0))>>7)

#define MASK64p(x) (((x>>1)|(0x8410841084108410)))
#define MASK64n(x) (((x>>1)&(0x7bef7bef7bef7bef)))


#define THRER (0xf8/8)
#define THREG (0x7e/8)
#define THREB (0x1f/8)
void grayScale(uint16_t *src, uint16_t* dst,int size);
void grayScale64(uint64_t *src, uint64_t* dst,int size);

void subt(uint16_t *b1,uint16_t* b2, uint16_t* dst,int mode);
void subt64(uint64_t *src, uint64_t *src2,uint64_t* dst,int size);
void biv(int16_t* dst,int size);


#endif

