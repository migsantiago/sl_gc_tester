//Sanlink Control Tester
//A program that uses the freetype library and ljpeg
//to show controller status

//devkitppc r15
//libogc 20080602

/*
max values found on a working controller:
stick x +-100
stick y +-100
c stick x +-100
c stick y +-100
L trigger 0-210
R trigger 0-210
*/

#include <gctypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mp3player.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include "freetype.h"
#include <jpeg/jpgogc.h>
#include <time.h>


//the makefile should include this
//LIBS	:=	-lmad -lm -logc -lfreetype -ljpeg

/*** External Picture ***/
//these come from picture.s
extern char     picdata[];
extern int      piclength;

//mp3 del archivo test.mp3
extern u8 mp3data[];
extern u32 mp3length;

/*** 2D Video Globals ***/
GXRModeObj *vmode;		/*** Graphics Mode Object ***/
u32 *xfb[2] = { NULL, NULL };	/*** Framebuffers ***/
int whichfb = 0;		/*** Frame buffer toggle ***/
int screenheight;

//global variables
int buttonsdown; //stores button status from controller
s8 stickx,sticky,c_x,c_y; //normal stick and c stick
u8 trigr,trigl; //pressure level from l and r
bool motor[4]={0,0,0,0}; //motor on or off 4 pads
bool plugged[4]; //shows which controllers are connected
u32 plugtot; //receives plugged controllers from padscan
char str[4]=""; //shows on or off, motor status
char slotstat[50]=""; //muestra el estado de la memcard
static u8 SysArea[CARD_WORKAREA] ATTRIBUTE_ALIGN(32); //memcard addressing
int errorA,errorB; //estado de las memorias en slot a y b

//////////////////////////////////////////////////////////////////////////////
static void
videoini (void)
{
  VIDEO_Init ();
  PAD_Init ();
  switch (VIDEO_GetCurrentTvMode ())
    {
    case VI_NTSC:
      vmode = &TVNtsc480IntDf;
      break;
    case VI_PAL:
      vmode = &TVPal528IntDf;
      break;
    case VI_MPAL:
      vmode = &TVMpal480IntDf;
      break;
    default:
      vmode = &TVNtsc480IntDf;
      break;
    }
  VIDEO_Configure (vmode);
  xfb[0] = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (vmode));
  xfb[1] = (u32 *) MEM_K0_TO_K1 (SYS_AllocateFramebuffer (vmode));
  console_init (xfb[0], 20, 64, vmode->fbWidth, vmode->xfbHeight,
		vmode->fbWidth * 2);
  screenheight = vmode->xfbHeight;
  VIDEO_ClearFrameBuffer (vmode, xfb[0], COLOR_BLACK);
  VIDEO_ClearFrameBuffer (vmode, xfb[1], COLOR_BLACK);
  VIDEO_SetNextFramebuffer (xfb[0]);
  VIDEO_SetPostRetraceCallback (PAD_ScanPads);
  VIDEO_SetBlack (0);
  VIDEO_Flush ();
  VIDEO_WaitVSync ();		/*** Wait for VBL ***/
  if (vmode->viTVMode & VI_NON_INTERLACE)
    VIDEO_WaitVSync ();
}

/////////////////////////////////////////////////////////////////////////////////////////
void readpad(int padnum)
{
buttonsdown = PAD_ButtonsDown(padnum)|PAD_ButtonsHeld(padnum);

c_x=PAD_SubStickX(padnum);
c_y=PAD_SubStickY(padnum);

stickx=PAD_StickX(padnum);
sticky=PAD_StickY(padnum);

trigl=PAD_TriggerL(padnum);
trigr=PAD_TriggerR(padnum);
    
if((buttonsdown&(PAD_BUTTON_B|PAD_BUTTON_A))==(PAD_BUTTON_B|PAD_BUTTON_A))
    motor[padnum]=1;
if((buttonsdown&(PAD_TRIGGER_Z|PAD_BUTTON_A))==(PAD_TRIGGER_Z|PAD_BUTTON_A))
    motor[padnum]=0;
if(motor[padnum])
    PAD_ControlMotor(padnum,PAD_MOTOR_RUMBLE);
else
    PAD_ControlMotor(padnum,PAD_MOTOR_STOP);
}

/////////////////////////////////////////////////////////////////////////////////////////
//On Off
char * on_off(int control)
{
if(motor[control])
    sprintf(str,"On");
else
    sprintf(str,"Off");
return str;
}

/////////////////////////////////////////////////////////////////////////////////////////
//Main function
int main()
{    
//Variables
char stats[50]; //shows stick and trigger levels
int i;
    
JPEGIMG         jpeg;
int             row,
                col,
                pix,
                offset;
unsigned int   *jpegout;
    
time_t gc_time;

memset(&jpeg, 0, sizeof(JPEGIMG));
    
//Video init
videoini();
    
//Freetypes font startup
FT_Init ();
    
ClearScreen (); /***clears the framebuffer : the screen.***/

/*** Set the Mandatory values ***/
jpeg.inbuffer = picdata;
jpeg.inbufferlength = piclength;

/*** Call decompressor ***/
JPEG_Decompress(&jpeg);
jpegout = (unsigned int *) jpeg.outbuffer;

/*** IMPORTANT - RELEASE THE MEMORY BUFFER ***/
free(jpeg.outbuffer);

/*** Initialise the MP3 Player ***/
MP3Player_Init ();
/*** Now just play from memory! ***/
ciclo:
MP3Player_PlayBuffer (mp3data,mp3length,NULL);

/*** MP3 is now playing in background ... ***/
while (MP3Player_IsPlaying ())
    {
    whichfb ^= 1;
    pix = 0;
    offset = 0;
    for (row = 0; row < jpeg.height; row++)
        {
        for (col = 0; col < (jpeg.width >> 1); col++)
            xfb[whichfb][offset + col] = jpegout[pix++];
        offset += 320;
        }
    
    setfontsize (25);/***sets the font size.***/
    setfontcolour (253, 144, 77); //orange
    DrawText(50,60,"Gamecube Test v0.4");
        
    setfontsize(15);
    setfontcolour (0,0,0); //black
    DrawText(50,80,"A + B - Motor ON");
    DrawText(50,100,"A + Z - Motor OFF");
    gc_time = time(NULL);
    sprintf(stats,"%s",ctime(&gc_time));
    stats[24]=0; //quita un cuadrito raro de texto, poniéndole un null
    setfontcolour (127,127,0); //green
    DrawText(50,120,stats);
    
    plugtot=PAD_ScanPads(); //receives plugged controllers    
    //cleans plugged controllers
    for(i=0;i<4;i++)
        plugged[i]=0;
    if(plugtot&0x0001) plugged[0]=1; //cont 1 connected
    if(plugtot&0x0002) plugged[1]=1; //cont 2 connected
    if(plugtot&0x0004) plugged[2]=1; //cont 3 connected
    if(plugtot&0x0008) plugged[3]=1; //cont 4 connected

    ///////////////////////////////////////////////////////////////////////
    readpad(0); //check controller 1
        
    setfontcolour (1, 76, 189); //blue
    DrawText(50,140,"Controller 1 Status");
    
    if(!plugged[0])
        {
        setfontcolour (191, 55, 0); //red
        DrawText(50,160,"Not connected!");
        }
    else
        {
        setfontcolour (0,0,0); //black
        DrawText(50,160,"Pressed Buttons:");
        
        if(buttonsdown&PAD_BUTTON_A){
            setfontcolour (127,127,0); //green
            DrawText(50,180,"A");
            }
        if(buttonsdown&PAD_BUTTON_B){
            setfontcolour (191, 55, 0); //red
            DrawText(62,180,"B");
            }
        if(buttonsdown&PAD_BUTTON_X){
            setfontcolour (0,0,0); //black
            DrawText(74,180,"X");
            }
        if(buttonsdown&PAD_BUTTON_Y){
            setfontcolour (0,0,0); //black
            DrawText(86,180,"Y");
            }
        if(buttonsdown&PAD_TRIGGER_Z){
            setfontcolour (95, 95, 243); //blue
            DrawText(98,180,"Z");
            }
            setfontcolour (0,0,0); //black
        if(buttonsdown&PAD_BUTTON_START){
            DrawText(110,180,"ST");
            }
        if(buttonsdown&PAD_TRIGGER_L){
            DrawText(134,180,"L");
            }
        if(buttonsdown&PAD_TRIGGER_R){
            DrawText(146,180,"R");
            }
        if(buttonsdown&PAD_BUTTON_UP){
            DrawText(158,180,"UP");
            }
        if(buttonsdown&PAD_BUTTON_DOWN){
            DrawText(182,180,"DW");
            }
        if(buttonsdown&PAD_BUTTON_LEFT){
            DrawText(206,180,"LF");
            }
        if(buttonsdown&PAD_BUTTON_RIGHT){
            DrawText(228,180,"RH");
            }
    
        sprintf(stats,"Stick - X:%03d Y:%03d",stickx,sticky);
        DrawText(50,200,stats);
        sprintf(stats,"Stick C - X:%03d Y:%03d",c_x,c_y);
        DrawText(50,220,stats);
        sprintf(stats,"But R - %03u",trigr);
        DrawText(50,240,stats);
        sprintf(stats,"But L - %03u",trigl);
        DrawText(50,260,stats);
        sprintf(stats,"Motor - %s",on_off(0));
        DrawText(50,280,stats);
        }
        
    ///////////////////////////////////////////////////////////////////////
    readpad(1); //check controller 2
    setfontcolour (1, 76, 189); //blue
    DrawText(300,140,"Controller 2 Status");
        
    if(!plugged[1])
        {
        setfontcolour (191, 55, 0); //red
        DrawText(300,160,"Not connected!");
        }
    else
        {
        setfontcolour (0,0,0); //black
        DrawText(300,160,"Pressed Buttons:");
        
        if(buttonsdown&PAD_BUTTON_A){
            setfontcolour (127,127,0); //green
            DrawText(300,180,"A");
            }
        if(buttonsdown&PAD_BUTTON_B){
            setfontcolour (191, 55, 0); //red
            DrawText(312,180,"B");
            }
        if(buttonsdown&PAD_BUTTON_X){
            setfontcolour (0,0,0); //black
            DrawText(324,180,"X");
            }
        if(buttonsdown&PAD_BUTTON_Y){
            setfontcolour (0,0,0); //black
            DrawText(336,180,"Y");
            }
        if(buttonsdown&PAD_TRIGGER_Z){
            setfontcolour (95, 95, 243); //blue
            DrawText(348,180,"Z");
            }
            setfontcolour (0,0,0); //black
        if(buttonsdown&PAD_BUTTON_START){
            DrawText(360,180,"ST");
            }
        if(buttonsdown&PAD_TRIGGER_L){
            DrawText(384,180,"L");
            }
        if(buttonsdown&PAD_TRIGGER_R){
            DrawText(396,180,"R");
            }
        if(buttonsdown&PAD_BUTTON_UP){
            DrawText(408,180,"UP");
            }
        if(buttonsdown&PAD_BUTTON_DOWN){
            DrawText(432,180,"DW");
            }
        if(buttonsdown&PAD_BUTTON_LEFT){
            DrawText(456,180,"LF");
            }
        if(buttonsdown&PAD_BUTTON_RIGHT){
            DrawText(478,180,"RH");
            }
    
        sprintf(stats,"Stick - X:%03d Y:%03d",stickx,sticky);
        DrawText(300,200,stats);
        sprintf(stats,"Stick C - X:%03d Y:%03d",c_x,c_y);
        DrawText(300,220,stats);
        sprintf(stats,"But R - %03u",trigr);
        DrawText(300,240,stats);
        sprintf(stats,"But L - %03u",trigl);
        DrawText(300,260,stats);
        sprintf(stats,"Motor - %s",on_off(1));
        DrawText(300,280,stats);
        }
        
    ///////////////////////////////////////////////////////////////////////
    readpad(2); //check controller 3
    setfontcolour (1, 76, 189); //blue
    DrawText(50,310,"Controller 3 Status");
        
    if(!plugged[2])
        {
        setfontcolour (191, 55, 0); //red
        DrawText(50,330,"Not connected!");
        }
    else
        {        
        setfontcolour (0,0,0); //black
        DrawText(50,330,"Pressed Buttons:");        
        if(buttonsdown&PAD_BUTTON_A){
            setfontcolour (127,127,0); //green
            DrawText(50,350,"A");
            }
        if(buttonsdown&PAD_BUTTON_B){
            setfontcolour (191, 55, 0); //red
            DrawText(62,350,"B");
            }
        if(buttonsdown&PAD_BUTTON_X){
            setfontcolour (0,0,0); //black
            DrawText(74,350,"X");
            }
        if(buttonsdown&PAD_BUTTON_Y){
            setfontcolour (0,0,0); //black
            DrawText(86,350,"Y");
            }
        if(buttonsdown&PAD_TRIGGER_Z){
            setfontcolour (95, 95, 243); //blue
            DrawText(98,350,"Z");
            }
            setfontcolour (0,0,0); //black
        if(buttonsdown&PAD_BUTTON_START){
            DrawText(110,350,"ST");
            }
        if(buttonsdown&PAD_TRIGGER_L){
            DrawText(134,350,"L");
            }
        if(buttonsdown&PAD_TRIGGER_R){
            DrawText(146,350,"R");
            }
        if(buttonsdown&PAD_BUTTON_UP){
            DrawText(158,350,"UP");
            }
        if(buttonsdown&PAD_BUTTON_DOWN){
            DrawText(182,350,"DW");
            }
        if(buttonsdown&PAD_BUTTON_LEFT){
            DrawText(206,350,"LF");
            }
        if(buttonsdown&PAD_BUTTON_RIGHT){
            DrawText(228,350,"RH");
            }
    
        sprintf(stats,"Stick - X:%03d Y:%03d",stickx,sticky);
        DrawText(50,370,stats);
        sprintf(stats,"Stick C - X:%03d Y:%03d",c_x,c_y);
        DrawText(50,390,stats);
        sprintf(stats,"But R - %03u",trigr);
        DrawText(50,410,stats);
        sprintf(stats,"But L - %03u",trigl);
        DrawText(50,430,stats);
        sprintf(stats,"Motor - %s",on_off(2));
        DrawText(50,450,stats);
        }

    ///////////////////////////////////////////////////////////////////////
    readpad(3); //check controller 4
    setfontcolour (1, 76, 189); //blue
    DrawText(300,310,"Controller 4 Status");
        
    if(!plugged[3])
        {
        setfontcolour (191, 55, 0); //red
        DrawText(300,330,"Not connected!");
        }
    else
        {        
        setfontcolour (0,0,0); //black
        DrawText(300,330,"Pressed Buttons:");
        
        if(buttonsdown&PAD_BUTTON_A){
            setfontcolour (127,127,0); //green
            DrawText(300,350,"A");
            }
        if(buttonsdown&PAD_BUTTON_B){
            setfontcolour (191, 55, 0); //red
            DrawText(312,350,"B");
            }
        if(buttonsdown&PAD_BUTTON_X){
            setfontcolour (0,0,0); //black
            DrawText(324,350,"X");
            }
        if(buttonsdown&PAD_BUTTON_Y){
            setfontcolour (0,0,0); //black
            DrawText(336,350,"Y");
            }
        if(buttonsdown&PAD_TRIGGER_Z){
            setfontcolour (95, 95, 243); //blue
            DrawText(348,350,"Z");
            }
            setfontcolour (0,0,0); //black
        if(buttonsdown&PAD_BUTTON_START){
            DrawText(360,350,"ST");
            }
        if(buttonsdown&PAD_TRIGGER_L){
            DrawText(384,350,"L");
            }
        if(buttonsdown&PAD_TRIGGER_R){
            DrawText(396,350,"R");
            }
        if(buttonsdown&PAD_BUTTON_UP){
            DrawText(408,350,"UP");
            }
        if(buttonsdown&PAD_BUTTON_DOWN){
            DrawText(432,350,"DW");
            }
        if(buttonsdown&PAD_BUTTON_LEFT){
            DrawText(456,350,"LF");
            }
        if(buttonsdown&PAD_BUTTON_RIGHT){
            DrawText(478,350,"RH");
            }
    
        sprintf(stats,"Stick - X:%03d Y:%03d",stickx,sticky);
        DrawText(300,370,stats);
        sprintf(stats,"Stick C - X:%03d Y:%03d",c_x,c_y);
        DrawText(300,390,stats);
        sprintf(stats,"But R - %03u",trigr);
        DrawText(300,410,stats);
        sprintf(stats,"But L - %03u",trigl);
        DrawText(300,430,stats);
        sprintf(stats,"Motor - %s",on_off(3));
        DrawText(300,450,stats);
        }
    
    //Checa el status de las memorias (no sd)
    setfontcolour (95, 95, 243); //blue
    CARD_Init(NULL,NULL);
    errorA=CARD_Mount(CARD_SLOTA, SysArea, NULL);
    switch(errorA)
        {
        case -3: //desconectada
            sprintf(slotstat,"SlotA: Empty");
            break;
        case -2: //sd card
            sprintf(slotstat,"SlotA: SD Card");
            break;
        case 0: //mem card
            sprintf(slotstat,"SlotA: Memory Card");
            break;
        default:
            sprintf(slotstat,"SlotA: Unknown %d",errorA);
            break;
        }
    CARD_Unmount (CARD_SLOTA);
    DrawText(350,80,slotstat); //slot a
    errorB=CARD_Mount(CARD_SLOTB, SysArea, NULL);
    switch(errorB)
        {
        case -3: //desconectada
            sprintf(slotstat,"SlotB: Empty");
            break;
        case -2: //sd card
            sprintf(slotstat,"SlotB: SD Card");
            break;
        case 0: //mem card
            sprintf(slotstat,"SlotB: Memory Card");
            break;
        default:
            sprintf(slotstat,"SlotB: Unknown %d",errorB);
            break;
        }
    CARD_Unmount (CARD_SLOTB);
    DrawText(350,100,slotstat); //slot b
        
    ShowScreen ();
    }
goto ciclo;

return 0;
}
