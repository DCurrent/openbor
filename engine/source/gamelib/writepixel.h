
// dest, src, transbg, fillcolor, blendfunc


#define wp_8_8_0_0_0() *cur_dest=*cur_src;
#define wp_8_8_0_0_1() *cur_dest=pfp(table,*cur_src,*cur_dest);
#define wp_8_8_0_1_0() *cur_dest=fillcolor;
#define wp_8_8_0_1_1() *cur_dest=pfp(table,fillcolor,*cur_dest);
#define wp_8_8_1_0_0() if(*cur_src)*cur_dest=*cur_src;
#define wp_8_8_1_0_1() if(*cur_src)*cur_dest=pfp(table,*cur_src,*cur_dest);
#define wp_8_8_1_1_0() if(*cur_src)*cur_dest=fillcolor;
#define wp_8_8_1_1_1() if(*cur_src)*cur_dest=pfp(table,fillcolor,*cur_dest);

#define dest16 ((unsigned short*)cur_dest)
#define table16 ((unsigned short*)table)
#define wp_16_x8_0_0_0() *dest16=table16[*cur_src];
#define wp_16_x8_0_0_1() *dest16=pfp16(table16[*cur_src],*dest16);
#define wp_16_x8_0_1_0() *dest16=fillcolor;
#define wp_16_x8_0_1_1() *dest16=pfp16(fillcolor,*dest16);
#define wp_16_x8_1_0_0() if(*cur_src)*dest16=table16[*cur_src];
#define wp_16_x8_1_0_1() if(*cur_src)*dest16=pfp16(table16[*cur_src],*dest16);
#define wp_16_x8_1_1_0() if(*cur_src)*dest16=fillcolor;
#define wp_16_x8_1_1_1() if(*cur_src)*dest16=pfp16(fillcolor,*dest16);

#define src16 ((unsigned short*)cur_src)
#define wp_16_16_0_0_0() *dest16=*src16;
#define wp_16_16_0_0_1() *dest16=pfp16(*src16,*dest16);
#define wp_16_16_0_1_0() *dest16=fillcolor;
#define wp_16_16_0_1_1() *dest16=pfp16(fillcolor,*dest16);
#define wp_16_16_1_0_0() if(*src16)*dest16=*src16;
#define wp_16_16_1_0_1() if(*src16)*dest16=pfp16(*src16,*dest16);
#define wp_16_16_1_1_0() if(*src16)*dest16=fillcolor;
#define wp_16_16_1_1_1() if(*src16)*dest16=pfp16(fillcolor,*dest16);

#define dest32 ((unsigned*)cur_dest)
#define table32 ((unsigned*)table)
#define wp_32_x8_0_0_0() *dest32=table32[*cur_src];
#define wp_32_x8_0_0_1() *dest32=pfp32(table32[*cur_src],*dest32);
#define wp_32_x8_0_1_0() *dest32=fillcolor;
#define wp_32_x8_0_1_1() *dest32=pfp32(fillcolor,*dest32);
#define wp_32_x8_1_0_0() if(*cur_src)*dest32=table32[*cur_src];
#define wp_32_x8_1_0_1() if(*cur_src)*dest32=pfp32(table32[*cur_src],*dest32);
#define wp_32_x8_1_1_0() if(*cur_src)*dest32=fillcolor;
#define wp_32_x8_1_1_1() if(*cur_src)*dest32=pfp32(fillcolor,*dest32);

#define src32 ((unsigned*)cur_src)
#define wp_32_32_0_0_0() *dest32=*src32;
#define wp_32_32_0_0_1() *dest32=pfp32(*src32,*dest32);
#define wp_32_32_0_1_0() *dest32=fillcolor;
#define wp_32_32_0_1_1() *dest32=pfp32(fillcolor,*dest32);
#define wp_32_32_1_0_0() if(*src32)*dest32=*src32;
#define wp_32_32_1_0_1() if(*src32)*dest32=pfp32(*src32,*dest32);
#define wp_32_32_1_1_0() if(*src32)*dest32=fillcolor;
#define wp_32_32_1_1_1() if(*src32)*dest32=pfp32(fillcolor,*dest32);


#define writepixelswitch(x,y) \
switch(wpcond) \
{ \
case  0: x wp_8_8_0_0_0() y break; \
case  1: x wp_8_8_0_0_1() y break; \
case  2: x wp_8_8_0_1_0() y break; \
case  3: x wp_8_8_0_1_1() y break; \
case  4: x wp_8_8_1_0_0() y break; \
case  5: x wp_8_8_1_0_1() y break; \
case  6: x wp_8_8_1_1_0() y break; \
case  7: x wp_8_8_1_1_1() y break; \
 \
case  8: x wp_16_x8_0_0_0() y break; \
case  9: x wp_16_x8_0_0_1() y break; \
case 10: x wp_16_x8_0_1_0() y break; \
case 11: x wp_16_x8_0_1_1() y break; \
case 12: x wp_16_x8_1_0_0() y break; \
case 13: x wp_16_x8_1_0_1() y break; \
case 14: x wp_16_x8_1_1_0() y break; \
case 15: x wp_16_x8_1_1_1() y break; \
 \
case 16: x wp_16_16_0_0_0() y break; \
case 17: x wp_16_16_0_0_1() y break; \
case 18: x wp_16_16_0_1_0() y break; \
case 19: x wp_16_16_0_1_1() y break; \
case 20: x wp_16_16_1_0_0() y break; \
case 21: x wp_16_16_1_0_1() y break; \
case 22: x wp_16_16_1_1_0() y break; \
case 23: x wp_16_16_1_1_1() y break; \
 \
case 24: x wp_32_x8_0_0_0() y break; \
case 25: x wp_32_x8_0_0_1() y break; \
case 26: x wp_32_x8_0_1_0() y break; \
case 27: x wp_32_x8_0_1_1() y break; \
case 28: x wp_32_x8_1_0_0() y break; \
case 29: x wp_32_x8_1_0_1() y break; \
case 30: x wp_32_x8_1_1_0() y break; \
case 31: x wp_32_x8_1_1_1() y break; \
 \
case 32: x wp_32_32_0_0_0() y break; \
case 33: x wp_32_32_0_0_1() y break; \
case 34: x wp_32_32_0_1_0() y break; \
case 35: x wp_32_32_0_1_1() y break; \
case 36: x wp_32_32_1_0_0() y break; \
case 37: x wp_32_32_1_0_1() y break; \
case 38: x wp_32_32_1_1_0() y break; \
case 39: x wp_32_32_1_1_1() y break; \
}


