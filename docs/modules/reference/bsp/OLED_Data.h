#ifndef __OLED_DATA_H
#define __OLED_DATA_H

#include <stdint.h>

/*字符集定义*/
/*以下两个宏定义只可解除其中一个的注释*/
#define OLED_CHARSET_UTF8			//定义字符集为UTF8
//#define OLED_CHARSET_GB2312		//定义字符集为GB2312

/*字模基本单元*/
typedef struct 
{
	
#ifdef OLED_CHARSET_UTF8			//定义字符集为UTF8
	char Index[5];					//汉字索引，空间为5字节
#endif
	
#ifdef OLED_CHARSET_GB2312			//定义字符集为GB2312
	char Index[3];					//汉字索引，空间为3字节
#endif
	
	uint8_t Data[32];				//字模数据
} ChineseCell_t;

/*ASCII字模数据声明*/
extern const uint8_t OLED_F8x16[][16];
extern const uint8_t OLED_F6x8[][6];
extern const uint8_t OLED_F12x24[][36];
/*汉字字模数据声明*/
extern const ChineseCell_t OLED_CF16x16[];

/* Chinese glyphs for OLED_12X24: 24 columns x 24 rows, 72 bytes. */
typedef struct
{
#ifdef OLED_CHARSET_UTF8
    char Index[5];
#endif
#ifdef OLED_CHARSET_GB2312
    char Index[3];
#endif
    uint8_t Data[72];
} ChineseCell24_t;

extern const ChineseCell24_t OLED_CF24x24[];

/*图像数据声明*/
extern const uint8_t Diode[];
/* Enter-style exit arrow: vertical 8-pixel pages, bit 0 at top. */
#define OLED_EXIT_ARROW_WIDTH    24U
#define OLED_EXIT_ARROW_HEIGHT   24U
#define OLED_EXIT_ARROW_BYTES    72U
extern const uint8_t ExitArrow[OLED_EXIT_ARROW_BYTES];
/* Course 1-3 menu icons and 44x44 selection frame. */
extern const uint8_t Menu_Graph[7][128];
extern const uint8_t Frame[264];
/* Course 1-8: two 16x16 eyebrows and one 20x20 mouth. */
extern const uint8_t Eyebrow[2][32];
extern const uint8_t Mouth[60];

/* Running red panda and cactus: vertical 8-pixel pages, bit 0 at top. */
#define OLED_PANDA_WIDTH         32U
#define OLED_PANDA_HEIGHT        32U
#define OLED_PANDA_RUN_FRAMES     6U
#define OLED_PANDA_RUN_BYTES    128U
#define OLED_PANDA_FRAME_MS      80U
#define OLED_CACTUS_WIDTH        16U
#define OLED_CACTUS_HEIGHT       24U
#define OLED_CACTUS_BYTES        48U

extern const uint8_t Panda_Run[OLED_PANDA_RUN_FRAMES][OLED_PANDA_RUN_BYTES];
extern const uint8_t Cactus[OLED_CACTUS_BYTES];

/* Monochrome dance animation, compatible with OLED_ShowImage(). */
#define OLED_KUN_DANCE_WIDTH      72U
#define OLED_KUN_DANCE_HEIGHT     56U
#define OLED_KUN_DANCE_FRAMES     45U
#define OLED_KUN_DANCE_BYTES     504U
#define OLED_KUN_DANCE_FRAME_MS   60U
/* Decodes into one shared buffer, valid until the next frame request. */
const uint8_t *OLED_GetKunDanceFrame(uint8_t FrameIndex);
/*按照上面的格式，在这个位置加入新的图像数据声明*/
//...

#endif


/*****************江协科技|版权所有****************/
/*****************jiangxiekeji.com*****************/
