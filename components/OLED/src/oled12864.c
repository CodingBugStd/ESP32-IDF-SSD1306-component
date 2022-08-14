#include "oled12864.h"
#include "font_lib.h"

#if oled_printf == 1
#include "stdio.h"
#endif

/************************************************
 * OLED12864缓存
 * [paeg][x]
 * 一个元素保函8像素点信息
 * 在屏幕上,Bit0~Bit7自上向下排列 高位在下
************************************************/
static uint8_t OLED12864_Sbuffer[8][128];
//OLED_初始化指令
static unsigned char OLED12864_InitCmd[28] = {
    0xae,0x00,0x10,0x40,0x81,0xcf,
    0xa1,0xc8,0xa6,0xa8,0x3f,0xd3,
    0x00,0xd5,0x80,0xd9,0xf1,0xda,
    0x12,0xdb,0x40,0x20,0x00,0x8d,
    0x14,0xa4,0xa6,0xaf
};

void OLED12864_Init(void)
{
    OLED12864_GPIO_Init();
    OLED12864_SPI_Init();
    OLED12864_Hard_Reset();
    OLED12864_Set_Position(0,0);
    OLED12864_delay_ms(100);
}

void OLED12864_Auto_Refresh(int hz){
    OLED12864_Auto_Refresh_Set(hz,OLED12864_Sbuffer[0]);
}

void OLED12864_Hard_Reset(void)
{
    OLED12864_Reset_Bit(OLED_RES);
    OLED12864_delay_ms(300);
    OLED12864_Set_Bit(OLED_RES);

    OLED12864_Send_NumByte(OLED12864_InitCmd,28,OLED_CMD);
    OLED12864_Clear();
}

void OLED12864_Clear_Sbuffer(void)
{
    uint8_t page,x;
    for(page=0;page<8;page++)
    {
        for(x=0;x<128;x++)
            OLED12864_Sbuffer[page][x] = 0x00;
    }
}

void OLED12864_Clear(void)
{
    OLED12864_Clear_Sbuffer();
    OLED12864_Refresh();
}

void OLED12864_Refresh(void)
{
    #if 1
    OLED12864_Set_Position(0,0);
    OLED12864_Send_NumByte(OLED12864_Sbuffer[0],1024,OLED_DATA);
    #endif
}

void OLED12864_Set_Position(uint8_t page,uint8_t x)
{
    static uint8_t dat[3];
    dat[0] = 0xb0 + page;           //页地址
    dat[1] = ((0xf0&x)>>4)|0x10;    //x坐标高四位设定
    dat[2] = 0x0f&x;                //x坐标低四位设定
    OLED12864_Send_NumByte(dat,3,OLED_CMD);
}

void OLED12864_Clear_PageBlock(uint8_t page,uint8_t x,uint8_t len)
{
    uint8_t sx = x+len;
    if(sx > x_MAX-1 || page > page_MAX-1)
        return;
    for(uint8_t temp=0;temp<len;temp++)
        OLED12864_Sbuffer[page][temp+x] = 0x00;
}

void OLED12864_Clear_Page(uint8_t page)
{
    OLED12864_Clear_PageBlock(page,0,127);
}

//像素点相关操作
#if USE_POINT_CRT == 1

void OLED12864_Draw_Point(uint8_t x,uint8_t y,uint8_t bit)
{
    if(y > y_MAX-1 || x > x_MAX-1)
        return;
    uint8_t page = y/8;
    uint8_t col = y%8;
    if(bit)
        OLED12864_Sbuffer[page][x] |= (0x01<<col);
    else
        OLED12864_Sbuffer[page][x] &= ~(0x01<<col);
}

void OLED12864_Draw_Line(uint8_t x1,uint8_t y1,uint8_t x2,uint8_t y2)
{
    float sx,sy;
    float k,k_1;   //斜率
    k = ((float)y2-y1) / ((float)x2-x1);
    k_1 = 1/k;
    sx = x1;
    sy = y1;
    for(;x1<=x2;x1++)
    {
        sy += k;
        OLED12864_Draw_Point(x1,(int)sy,1);
    }
    for(;y1<=y2;y1++)
    {
        sx += k_1;
        OLED12864_Draw_Point((int)sx,y1,1);
    }
}

void OLED12864_Draw_Rect(uint8_t x,uint8_t y,uint8_t len,uint8_t hight)
{
    for(uint8_t temp=0;temp<len;temp++)
    {
        OLED12864_Draw_Point(x+temp,y,1);
        OLED12864_Draw_Point(x+temp,y+hight,1);
    }
    for(uint8_t temp=0;temp<hight;temp++)
    {
        OLED12864_Draw_Point(x,y+temp,1);
        OLED12864_Draw_Point(x+len,y+temp,1);
    }
}

void OLED12864_Show_Char(uint8_t x,uint8_t y,uint8_t chr,uint8_t size){
    unsigned char *offsetAddr = OLED12864_Sbuffer[0] + ( (y/8) * 128 ) + x;
    uint8_t pageOffset = y%8;
    uint8_t pageUpOffset = 8 - pageOffset;
    switch (size)
    {
    case 1:
        for(uint8_t sx = 0; sx<6 ;sx++){
            *(offsetAddr) &=  ~(0xff << pageOffset);
            *(offsetAddr) |=  assic_0806[chr-0x20][sx] << pageOffset;
            offsetAddr += 128;
            *(offsetAddr) &=  ~(0xff >> pageUpOffset);
            *(offsetAddr) |=  assic_0806[chr-0x20][sx] >> pageUpOffset;
            offsetAddr -= 127;
        }
        break;
    case 2:
        for(uint8_t sx = 0; sx<8 ;sx++){
            *(offsetAddr) &=  ~(0xff << pageOffset);
            *(offsetAddr) |=  (assic_1608[chr-0x20][sx] << pageOffset);
            offsetAddr += 128;
            *(offsetAddr) &=  ~0xff ;
            *(offsetAddr) |=  assic_1608[chr-0x20][sx] >> pageUpOffset;
            *(offsetAddr) |=  assic_1608[chr-0x20][sx+8] << pageOffset;
            offsetAddr += 128;
            *(offsetAddr) &=  ~(0xff >> pageUpOffset);
            *(offsetAddr) |=  (assic_1608[chr-0x20][sx+8] >> pageUpOffset);
            offsetAddr -= 255;
        }
        break;
    default:
        break;
    }
}

void OLED12864_Show_String(uint8_t x,uint8_t y,uint8_t*str,uint8_t size)
{
    uint8_t sx = 0;
    while(*str!='\0')
    {
        OLED12864_Show_Char(sx,y,*str,size);
        switch(size)
        {
            case 1:
                sx+=6;
                break;
            case 2:
                sx+=8;
                break;
            default:
                break;
        }
        str++;
    }
}

uint8_t OLED12864_Show_Num(uint8_t x,uint8_t y,int num,uint8_t size)
{
    return 0;
}
uint8_t OLED12864_Show_fNum(uint8_t x,uint8_t y,double num,uint8_t size,uint8_t d_len){
    return 0;
}

void OLED12864_Draw_Img(uint8_t x,uint8_t y,uint8_t len,uint8_t hight,uint8_t*img)
{
    uint8_t sx,sy;
    uint16_t dat_addr_pos;
    uint8_t page_pos;
    uint8_t bit_pos;
    for(sy=0;sy<hight;sy++)
    {
        page_pos = sy/8;
        bit_pos = sy%8;
        for(sx=0;sx<len;sx++)
        {
            dat_addr_pos = page_pos*len + sx;
            OLED12864_Draw_Point(sx+x,sy+y, *(img+dat_addr_pos) & ((0x80)>>bit_pos) );
        }
    }
}

void OLED12864_Draw_aImg(uint8_t x,uint8_t y,uint8_t*img)
{
    uint8_t len,hight;
    uint8_t sx,sy;
    uint16_t dat_addr_pos;
    uint8_t page_pos;
    uint8_t bit_pos;
    len = *(img+3) + *(img+2)*256;
    hight = *(img+5) + *(img+4)*256;
    for(sy=0;sy<hight;sy++)
    {
        page_pos = sy/8;
        bit_pos = sy%8;
        for(sx=0;sx<len;sx++)
        {
            dat_addr_pos = page_pos*len + sx + 6;
            OLED12864_Draw_Point(sx+x,sy+y, *(img+dat_addr_pos) & ((0x80)>>bit_pos) );
        }
    }
}

#else

void OLED12864_Show_Char(uint8_t page,uint8_t x,uint8_t chr,uint8_t size)
{
    switch(size)
    {
        case 1:
            for(uint8_t temp=0;temp<6;temp++)
                OLED12864_Sbuffer[page][x+temp] = assic_0806[chr-0x20][temp];
            break;
        case 2:
            //从左至右,从上至下
            for(uint8_t s_page = 0;s_page<2;s_page++)
            {
                for(uint8_t s_x = 0; s_x<8;s_x++)
                {
                    OLED12864_Sbuffer[s_page+page][x+s_x] = assic_1608[chr-0x20][s_page*8+s_x];
                }
            }
            break;
        default:break;
    }
}

void OLED12864_Show_String(uint8_t page,uint8_t x,uint8_t*str,uint8_t size)
{
    uint8_t sx = 0;
    while(*str!='\0')
    {
        OLED12864_Show_Char(page,x+sx,*str,size);
        switch(size)
        {
            case 1:
                sx+=6;
                break;
            case 2:
                sx+=8;
                break;
            default:
                break;
        }
        str++;
    }
}

uint8_t OLED12864_Show_fNum(uint8_t page,uint8_t x,double num,uint8_t size,uint8_t d_len)
{
    int L_Num;              //整数部分
    double R_Num;           //小数部分
    int R_Num2;             //根据小数部分化整
    uint8_t L_len = 0;      //整数部分长度
    if(d_len==0)
        d_len = 1;
    L_Num = (int)num;
    R_Num = num - L_Num;
    if(R_Num<0)
        R_Num = -R_Num;
    L_len = OLED12864_Show_Num(page,x,L_Num,size);
    switch(size)
    {
        case 1: x += 6*(L_len+1); OLED12864_Show_Char(page,x,'.',size); x+=6; break;
        case 2: x += 8*(L_len+1); OLED12864_Show_Char(page,x,'.',size); x+=8; break;
        default:break;
    }
    while(d_len!=0)
    {
        R_Num*=10;
        d_len--;
    }
    R_Num2 = (int)R_Num;
    OLED12864_Show_Num(page,x,R_Num2,size);
    return L_len+1+d_len;
}

uint8_t  OLED12864_Show_Num(uint8_t page,uint8_t x,int num,uint8_t size)
{
    uint8_t sbuf[8];
    sprintf((char*)sbuf,"%d",num);
    OLED12864_Show_String(page,x,sbuf,size);
    return 0;
}

#endif
