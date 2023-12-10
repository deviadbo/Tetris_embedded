/*
 * Tetris
 * Author: Deviad Bokobza
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "System/system.h"
#include "System/delay.h"
#include "System\clock.h"
#include "oledDriver/oledC.h"
#include "oledDriver/oledC_colors.h"
#include "oledDriver/oledC_shapes.h"
#include "xc.h"

// VARS
typedef unsigned char BOOL;
#define FALSE 0
#define TRUE  !FALSE

#define MAX_PM_VAL  1023
#define MAX_RGB_VAL  255

#define MINI_BLOCK_SIZE  6

#define MAX_BORDER 95

//new defines
#define MAX_BLOCKS_ROW 12
#define MINI_BLOCK_SIZE_2  8
#define BLOCK_MATRIX_SIZE 4
#define MINI_BLOCKS 4

//LED PROTS
#define R_LED OC1R
#define G_LED OC2R
#define B_LED OC3R

#define TimerPeriod T1CONbits.TCKPS
int timer_limit = 60;
//My Colors
#define BackgroundColor OLEDC_COLOR_BLACK

int FallingSpeed = 1;
int levelTimer = 0;
int roateLeft = 0;
int roateRight = 0;

typedef enum {
    O, L, J, I, Z, T, S, X, W
} block_name;

//Board size 12 mini block over a row
//LED Screen pixels = 96

//The brain of the game
block_name LCD_Board[MAX_BLOCKS_ROW][MAX_BLOCKS_ROW];

int Global_Counter = 0; //for timer
//int Global_X = 0;
//int Global_Y = 0;

//VARS
int pmValue = 0;
int new_pmValue = 0;

//block_name CurrentBlock;
//block_name NextBlock;



struct Blocks_Data {
   block_name  name;
   int  block[MINI_BLOCKS][2];
   uint16_t color;
};  

typedef struct Blocks_Data Blocks_Data;

Blocks_Data G_CurrentBlock;
Blocks_Data G_NextBlock;
Blocks_Data G_Temp;

//Blocks 
//Global Blocks 

/*
 In any structure of: Blocks Data
  There is the name of the block, a matrix of coordinates, and its unique color
 */
 Blocks_Data O_Block = 
 {O,
 { {0,0}, {1,0}, {0,1}, {1,1}} 
 ,0x1f};
 
  Blocks_Data I_Block = 
 {I,
 { {0,0}, {0,1}, {0,2}, {0,3}} 
 ,0xec1d};
  
  Blocks_Data L_Block = 
 {L,
 { {0,0}, {1,0}, {1,1}, {1,2}} 
 ,0xf81f};
  
Blocks_Data J_Block = 
 {J,
 { {1,1}, {1,0}, {1,2}, {0,2}} 
 , 0xf800};
  
  Blocks_Data T_Block = 
 {T,
 { {0,1}, {1,0}, {1,1}, {1,2}} 
 ,0x400};

 Blocks_Data S_Block = 
 {S,
 { {0,1}, {1,0}, {0,2}, {1,1}} 
 ,0xffe0    };
 
 Blocks_Data Z_Block = 
 {Z,
 { {0,0}, {0,1}, {1,1}, {1,2}} 
 ,0x7ff};
 
 
 uint16_t getColor (block_name _n)
 {
     switch (_n)
     {
        case O:
            return 0x1f;
            break;
        case L:
            return 0xf81f;
            break;
        case J:
            return 0xf800;
            break;
        case I:
            return 0xec1d;
            break;
        case Z:
            return 0x7ff;
            break;
        case T:
            return 0x400;
            break;
        case S:
            return 0xffe0;
            break;
        case W:
            return 0xffff;
            break;
        case X:
            return oledC_getBackground();
            break;
    }
 }
        
void showBlock(Blocks_Data X)
/*In each block structure the basic coordinates of the shape are reserved and color*/
    {
        int i=0;
        //oledC_DrawRectangle(uint8_t start_x, uint8_t start_y, uint8_t end_x, uint8_t end_y, uint16_t color)
        for (i=0; i<BLOCK_MATRIX_SIZE; i++)
        {
            uint8_t start_x = X.block[i][1]* MINI_BLOCK_SIZE_2;
            uint8_t start_y = X.block[i][0]*MINI_BLOCK_SIZE_2;
            uint8_t end_x = start_x + MINI_BLOCK_SIZE_2;
            uint8_t end_y = start_y + MINI_BLOCK_SIZE_2;
            uint16_t color = X.color;
            oledC_DrawRectangle(start_x,  start_y, end_x, end_y, color);
        }
    }

void hideBlock(Blocks_Data X)
//Paint the shape in the background color - and thus "erase"
    {
        int i=0;
        //oledC_DrawRectangle(uint8_t start_x, uint8_t start_y, uint8_t end_x, uint8_t end_y, uint16_t color)
        for (i=0; i<BLOCK_MATRIX_SIZE; i++)
        {
            uint8_t start_x = X.block[i][1]* MINI_BLOCK_SIZE_2;
            uint8_t start_y = X.block[i][0]*MINI_BLOCK_SIZE_2;
            uint8_t end_x = start_x + MINI_BLOCK_SIZE_2;
            uint8_t end_y = start_y + MINI_BLOCK_SIZE_2;

            uint16_t color = oledC_getBackground();
            oledC_DrawRectangle(start_x,  start_y, end_x, end_y, color);
        }
    }

/*
 * The function checks that the new block will not slide beyond the boundaries
 good not change*/
BOOL checkBoardBorders(Blocks_Data X, char direction)
{
    int i=0; //row
    if (direction == 'R')
    {
        for (i=0; i<BLOCK_MATRIX_SIZE; i++)
            if (X.block[i][1]> MAX_BLOCKS_ROW-2)
                return FALSE;
    }
    else if (direction == 'L')
    {
        for (i=0; i<BLOCK_MATRIX_SIZE; i++)
            if (X.block[i][1] < 1)
                return FALSE;
    }
    else if (direction == 'D')
    {
        for (i=0; i<BLOCK_MATRIX_SIZE; i++)
            if (X.block[i][0] > MAX_BLOCKS_ROW-2)
                return FALSE;
    }
    return TRUE;
}

void moveBlock(Blocks_Data* X, char direction)
{
    int i=0;
    //If there is no surfing from the border, you can move
    if (checkBoardBorders(*X, direction) == TRUE)
    {
        if (direction == 'R')
        {
            hideBlock(*X);
            //Move the X-axis to the right
            for (i=0; i<BLOCK_MATRIX_SIZE; i++)
                X->block[i][1]++;
            showBlock(*X);
        }
        else if (direction == 'L')
        {
            hideBlock(*X);
            //Move the X-axis to the left
            for (i=0; i<BLOCK_MATRIX_SIZE; i++)
                X->block[i][1]--;
            showBlock(*X);
        }
        else if (direction == 'D')
        {
            hideBlock(*X);
            //Move the Y-axis to the down (in LCD is in increase)
            for (i=0; i<BLOCK_MATRIX_SIZE; i++)
                X->block[i][0] = X->block[i][0] + 1;
            showBlock(*X);
        }
    }
}

void roateBlockRight(Blocks_Data* X)
{
    int new_coordinate[MINI_BLOCKS][2];
    int i=0;
    if (X->name == L || X->name == T || X->name == J || X->name == S || X->name == I || X->name == Z)
    {
        if (roateRight == 1)
        {
            //X->block[1][0] =  X->block[1][0] + 1;
            //X->block[1][1] = X->block[1][1] + 1;
            for (i=0; i<BLOCK_MATRIX_SIZE; i++)
            {
                new_coordinate[i][0] = G_Temp.block[i][1] - X->block[1][0];
                new_coordinate[i][1] = -G_Temp.block[i][0] - X->block[1][1];  
            }
        }
        else if (roateRight == 2)
        {
            for (i=0; i<BLOCK_MATRIX_SIZE; i++)
            {
                new_coordinate[i][0] = -G_Temp.block[i][0] - X->block[1][0];
                new_coordinate[i][1] = -G_Temp.block[i][1] - X->block[1][1];  
            }
        }
        else if (roateRight == 3)
        {
            for (i=0; i<BLOCK_MATRIX_SIZE; i++)
            {
                new_coordinate[i][0] = -G_Temp.block[i][1] - X->block[1][0];
                new_coordinate[i][1] = G_Temp.block[i][0] - X->block[1][1];    
            }
        }
        else if (roateRight == 4)
        {
            for (i=0; i<BLOCK_MATRIX_SIZE; i++)
            {
                new_coordinate[i][0] = G_Temp.block[i][0] + X->block[1][0];
                new_coordinate[i][1] = G_Temp.block[i][1] + X->block[1][1];      
            }
            roateRight = 0;
        }
        //Update coordinate 
        for (i=0; i<BLOCK_MATRIX_SIZE; i++)
        {
            X->block[i][0] = new_coordinate[i][0];
            X->block[i][1] = new_coordinate[i][1];
        }
    }
}

void roateBlockLeft(Blocks_Data* X)
{
    int new_coordinate[MINI_BLOCKS][2];
    int i=0;
    if (X->name == L || X->name == T || X->name == J || X->name == S || X->name == I || X->name == Z)
    {
        if (roateLeft == 3)
        {
            for (i=0; i<BLOCK_MATRIX_SIZE; i++)
            {
                new_coordinate[i][0] = G_Temp.block[i][1] - X->block[1][0];
                new_coordinate[i][1] = -G_Temp.block[i][0] - X->block[1][1];  
            }
        }
        else if (roateLeft == 2)
        {
            for (i=0; i<BLOCK_MATRIX_SIZE; i++)
            {
                new_coordinate[i][0] = -G_Temp.block[i][0] - X->block[1][0];
                new_coordinate[i][1] = -G_Temp.block[i][1] - X->block[1][1];  
            }
        }
        else if (roateLeft == 1)
        {
            for (i=0; i<BLOCK_MATRIX_SIZE; i++)
            {
                new_coordinate[i][0] = -G_Temp.block[i][1] - X->block[1][0];
                new_coordinate[i][1] = G_Temp.block[i][0] - X->block[1][1];    
            }
        }
        else if (roateLeft == 4)
        {
            for (i=0; i<BLOCK_MATRIX_SIZE; i++)
            {
                new_coordinate[i][0] = G_Temp.block[i][0] + X->block[1][0];
                new_coordinate[i][1] = G_Temp.block[i][1] + X->block[1][1];      
            }
            roateLeft = 0;
        }
        //Update coordinate 
        for (i=0; i<BLOCK_MATRIX_SIZE; i++)
        {
            X->block[i][0] = new_coordinate[i][0];
            X->block[i][1] = new_coordinate[i][1];
        }
    }
}

void roateBlock(Blocks_Data* X)
{
    int i=0;
    if (X->name == I)
    {
        hideBlock(*X);
        for (i=0; i<BLOCK_MATRIX_SIZE; i++)
        {
            int x = X->block[i][0];
            int y = X->block[i][1];
            X->block[i][0] =  y;
            X->block[i][1] = x;
        }
        showBlock(*X);
    }
    else if (X->name == L)
    {
        hideBlock(*X);
        for (i=0; i<BLOCK_MATRIX_SIZE; i++)
        {
            int x = X->block[i][1];
            int y = X->block[i][0];
            X->block[i][1] =  y;
            X->block[i][0] = x;
        }
        showBlock(*X);
    }

}

int getPotentiometerValue() {
    AD1CON1bits.SAMP = 1;
    int i = 0;
    for (i = 0; i < 10; i++);
    //empty loop
    AD1CON1bits.SAMP = 0;
    return ADC1BUF0;
}

//If clr is equal to 0, it means I want to paint the shape black,
//and "delete" the previous step.

static void Draw_O_Block(uint8_t x, uint8_t y, uint8_t clr)
{    
    //Draw big O block
    uint16_t color = OLEDC_COLOR_DARKBLUE;
    if (clr==0) color = BackgroundColor;
    oledC_DrawRectangle(x,y,x+MINI_BLOCK_SIZE*2 + 2,y+MINI_BLOCK_SIZE*2 + 2, color);
    //Draw 4 mini blocks
    color = OLEDC_COLOR_BLUE;
    if (clr==0) color = BackgroundColor;
    oledC_DrawRectangle(x,y,x+MINI_BLOCK_SIZE,y+MINI_BLOCK_SIZE, color);
    uint8_t nx = x + 2 + MINI_BLOCK_SIZE;
    oledC_DrawRectangle(nx,y,nx+MINI_BLOCK_SIZE,y+MINI_BLOCK_SIZE, color);
    uint8_t ny = y + MINI_BLOCK_SIZE + 2;  
    oledC_DrawRectangle(x,ny,x+MINI_BLOCK_SIZE,ny+MINI_BLOCK_SIZE, color);
    oledC_DrawRectangle(nx,ny,nx+MINI_BLOCK_SIZE,ny+MINI_BLOCK_SIZE, color);
}

static void Draw_I_Block(uint8_t x, uint8_t y, uint8_t clr)
{
    //Draw big I block
    uint16_t color = OLEDC_COLOR_DARKVIOLET; 
    if (clr==0) color = BackgroundColor;
    oledC_DrawRectangle(x,y,x+MINI_BLOCK_SIZE*4 + 6,y + MINI_BLOCK_SIZE, color);
    //Draw 4 mini blocks
    color = OLEDC_COLOR_VIOLET;
    if (clr==0) color = BackgroundColor;
    int i = 1;
    for (i=1;i<=4;i++)
    {
        oledC_DrawRectangle(x,y,x+MINI_BLOCK_SIZE,y+MINI_BLOCK_SIZE, color);
        x = x + 2 + MINI_BLOCK_SIZE;
    }
}

static void Draw_J_Block(uint8_t x, uint8_t y, uint8_t clr)
{
     //     *
     //     ***
    //Draw big block
    uint16_t color = OLEDC_COLOR_DARKRED; 
    if (clr==0) color = BackgroundColor;
    oledC_DrawRectangle(x,y,x+MINI_BLOCK_SIZE,y + MINI_BLOCK_SIZE + 2, color);
    uint8_t ny = y + MINI_BLOCK_SIZE + 2;
    oledC_DrawRectangle(x,ny,x+MINI_BLOCK_SIZE*3 + 4,ny + MINI_BLOCK_SIZE, color);
    //Draw 4 mini blocks
    color = OLEDC_COLOR_RED;
    if (clr==0) color = BackgroundColor;
    oledC_DrawRectangle(x,y,x+MINI_BLOCK_SIZE,y + MINI_BLOCK_SIZE, color);
    int i = 1;
    //y = y + MINI_BLOCK_SIZE +2;
    for (i=1;i<=3;i++)
    {
        oledC_DrawRectangle(x,ny,x+MINI_BLOCK_SIZE,ny + MINI_BLOCK_SIZE, color);
        x = x + 2 + MINI_BLOCK_SIZE;
    }
}

static void Draw_T_Block(uint8_t x, uint8_t y, uint8_t clr)
{
     //      *
     //     ***
    //Draw big block
    uint16_t color = OLEDC_COLOR_DARKGREEN; 
    if (clr==0) color = BackgroundColor;
    uint16_t tx = x + MINI_BLOCK_SIZE +2;
    oledC_DrawRectangle(tx,y,tx+MINI_BLOCK_SIZE,y + MINI_BLOCK_SIZE + 2, color);
    uint8_t ny = y + MINI_BLOCK_SIZE + 2;
    oledC_DrawRectangle(x,ny,x+MINI_BLOCK_SIZE*3 + 4,ny + MINI_BLOCK_SIZE, color);
    //Draw 4 mini blocks
    color = OLEDC_COLOR_GREEN;
    if (clr==0) color = BackgroundColor;
    oledC_DrawRectangle(tx,y,tx+MINI_BLOCK_SIZE,y + MINI_BLOCK_SIZE, color);
    int i = 1;
    //y = y + MINI_BLOCK_SIZE +2;
    for (i=1;i<=3;i++)
    {
        oledC_DrawRectangle(x,ny,x+MINI_BLOCK_SIZE,ny + MINI_BLOCK_SIZE, color);
        x = x + 2 + MINI_BLOCK_SIZE;
    }
}

static void Draw_L_Block(uint8_t x, uint8_t y, uint8_t clr)
{
     //       *
     //     ***
     
    //Draw big block
    uint16_t color = OLEDC_COLOR_DARKMAGENTA; 
    if (clr==0) color = BackgroundColor;
    uint16_t nx = x+MINI_BLOCK_SIZE*2+4;
    oledC_DrawRectangle(nx,y,nx+MINI_BLOCK_SIZE,y + MINI_BLOCK_SIZE + 2, color);
    uint8_t ny = y + MINI_BLOCK_SIZE + 2;
    oledC_DrawRectangle(x,ny,x+MINI_BLOCK_SIZE*2 + 4,ny + MINI_BLOCK_SIZE, color);
    //Draw 4 mini blocks
    color = OLEDC_COLOR_MAGENTA;
    if (clr==0) color = BackgroundColor;
    oledC_DrawRectangle(nx,y,nx+MINI_BLOCK_SIZE,y + MINI_BLOCK_SIZE, color);
    int i = 1;
    //y = y + MINI_BLOCK_SIZE +2;
    for (i=1;i<=3;i++)
    {
        oledC_DrawRectangle(x,ny,x+MINI_BLOCK_SIZE,ny + MINI_BLOCK_SIZE, color);
        x = x + 2 + MINI_BLOCK_SIZE;
    }
}

static void Draw_S_Block(uint8_t x, uint8_t y, uint8_t clr)
{
     //      
     //     **
    //Draw big block
    uint16_t color = OLEDC_COLOR_YELLOWGREEN; 
    if (clr==0) color = BackgroundColor;
    uint16_t nx = x+MINI_BLOCK_SIZE+2;
    oledC_DrawRectangle(nx,y,nx+MINI_BLOCK_SIZE*2 +2,y + MINI_BLOCK_SIZE, color);
    uint8_t ny = y + MINI_BLOCK_SIZE+2;
    oledC_DrawRectangle(x,ny,x+MINI_BLOCK_SIZE*2 +2,ny + MINI_BLOCK_SIZE, color);
    oledC_DrawRectangle(nx,y,nx+MINI_BLOCK_SIZE,ny + MINI_BLOCK_SIZE, color);
    //Draw 4 mini blocks
    color = OLEDC_COLOR_YELLOW;
    if (clr==0) color = BackgroundColor;
    int i = 1;
    for (i=1;i<=2;i++)
    {
        oledC_DrawRectangle(nx,y,nx+MINI_BLOCK_SIZE,y + MINI_BLOCK_SIZE, color);
        nx = nx + 2 + MINI_BLOCK_SIZE;
    }
    //y = y + MINI_BLOCK_SIZE +2;
    for (i=1;i<=2;i++)
    {
        oledC_DrawRectangle(x,ny,x+MINI_BLOCK_SIZE,ny + MINI_BLOCK_SIZE, color);
        x = x + 2 + MINI_BLOCK_SIZE;
    }
}

static void Draw_Z_Block(uint8_t x, uint8_t y, uint8_t clr)
{
     //     **
     //      **
    //Draw big block
    uint16_t color = OLEDC_COLOR_DARKCYAN; 
    if (clr==0) color = BackgroundColor;
    uint16_t nx = x+MINI_BLOCK_SIZE+2;
    oledC_DrawRectangle(x,y,x+MINI_BLOCK_SIZE*2 +2,y + MINI_BLOCK_SIZE, color);
    uint8_t ny = y + MINI_BLOCK_SIZE+2;
    oledC_DrawRectangle(nx,ny,nx+MINI_BLOCK_SIZE*2 +2,ny + MINI_BLOCK_SIZE, color);
    oledC_DrawRectangle(nx,y,nx+MINI_BLOCK_SIZE,ny + MINI_BLOCK_SIZE, color);
    //Draw 4 mini blocks
    color = OLEDC_COLOR_CYAN;
    if (clr==0) color = BackgroundColor;
    int i = 1;
    for (i=1;i<=2;i++)
    {
        oledC_DrawRectangle(x,y,x+MINI_BLOCK_SIZE,y + MINI_BLOCK_SIZE, color);
        x = x + 2 + MINI_BLOCK_SIZE;
    }
    //y = y + MINI_BLOCK_SIZE +2;
    for (i=1;i<=2;i++)
    {
        oledC_DrawRectangle(nx,ny,nx+MINI_BLOCK_SIZE,ny + MINI_BLOCK_SIZE, color);
        nx = nx + 2 + MINI_BLOCK_SIZE;
    }
}

static block_name ChoseCurrentBlock()
{
    //O, L, J, I, Z, T, S
    // Use current time as seed for random generator
    int blockID = rand() % 7 +1;
    switch (blockID)
    {
        case 1:
            return O;
        case 2:
            return L;
        case 3:
            return J;
        case 4:
            return I;
        case 5:
            return Z;
        case 6:
            return T;
        case 7:
            return S; 
    }
    return 0;
}

static Blocks_Data ChoseCurrentBlockRef()
{
    //O, L, J, I, Z, T, S
    int blockID = rand() % 490 + 1;
    blockID = blockID % 7 + 1;
    //blockID = 6;
    switch (blockID)
    {
        case 1:
            return O_Block;
        case 2:
            return L_Block;
        case 3:
            return J_Block;
        case 4:
            return I_Block;
        case 5:
            return Z_Block;
        case 6:
            return T_Block;
        case 7:
            return S_Block; 
    }
}

static void InitializeSystemAndLED(void) {
    //Configure the Port Directions (TRISx registers)
    TRISAbits.TRISA11 = 1;
    TRISAbits.TRISA12 = 1;
    TRISAbits.TRISA8 = 0;
    TRISAbits.TRISA9 = 0;

    //NEW FOR RGB LED
    TRISAbits.TRISA0 = 0;
    TRISAbits.TRISA1 = 0;
    TRISCbits.TRISC7 = 0;

    //Configure the ADCON registers

    //ANSB: Configure potentiometer pin for Analog Input (RB12) =12
    AD1CON1bits.ADON = 1;
    PORTB = 12;

    //AD1CON1 Fields: 
    //SSRC (use 0), MODE12(use 0), ADON (1), FORM (use 0) AD1CON2 (use 0) 
    AD1CON1bits.SSRC = 0;
    AD1CON1bits.MODE12 = 0;
    AD1CON1bits.FORM = 0;
    AD1CON2 = 0;

    //AD1CON3 Fields
    AD1CON3bits.ADCS = 0xFF; //ADCS (use 0xFF) 
    AD1CON3bits.SAMC = 0x10; //SAMC (use 0x10) 

    AD1CHS = 8; //Select channel (ADICHS, channel A = 8)

    //New Initialize
    //Static defintion
    //RED
    OC1CON2bits.SYNCSEL = 0x1F; //self-sync
    OC1CON2bits.OCTRIG = 0; //sync mode
    OC1CON1bits.OCTSEL = 0b111; //FOSC/2
    OC1CON1bits.OCM = 0b110; //edge aligned
    OC1CON2bits.TRIGSTAT = 1;

    OC1RS = 1023; //PWM period
    RPOR13 = 13;
    OC1R = 0; //intensity - Can be changed

    //GREEN
    OC2CON2bits.SYNCSEL = 0x1F; //self-sync
    OC2CON2bits.OCTRIG = 0; //sync mode
    OC2CON1bits.OCTSEL = 0b111; //FOSC/2
    OC2CON1bits.OCM = 0b110; //edge aligned
    OC2CON2bits.TRIGSTAT = 1;

    OC2RS = 1023; //PWM period
    OC2R = 0;

    //BLUE
    OC3CON2bits.SYNCSEL = 0x1F; //self-sync
    OC3CON2bits.OCTRIG = 0; //sync mode
    OC3CON1bits.OCTSEL = 0b111; //FOSC/2
    OC3CON1bits.OCM = 0b110; //edge aligned  
    OC3CON2bits.TRIGSTAT = 1;

    OC3RS = 1023; //PWM period
    OC3R = 0;

    RPOR13bits.RP26R = 13;
    RPOR13bits.RP27R = 14;
    RPOR11bits.RP23R = 15;

    //TURN ON COLOR LEDS
    PORTAbits.RA0 = 1; //RED
    PORTAbits.RA1 = 1; //GREEN
    PORTCbits.RC7 = 1; //BLUE
   
    //IOCPA
    PADCON = 0x8000;
    IOCSTAT = 1;
    IOCPAbits.IOCPA11 = 1;
    IOCPAbits.IOCPA12 = 1;
    IFS1bits.IOCIF = 0;
    IEC1bits.IOCIE = 1;
}

static void Turn_Little_LED(Blocks_Data tmp)
{
    switch (tmp.name)
    {
        case O:
            //OLEDC_COLOR_BLUE
            R_LED = 0;
            G_LED = 0;
            B_LED = 255;
            break;
        case L:
            //OLEDC_COLOR_MAGENTA
            R_LED = 255;
            G_LED = 0;
            B_LED = 255;
            break;
        case J:
            //OLEDC_COLOR_RED
            R_LED = 255;
            G_LED = 0;
            B_LED = 0;
            break;
        case I:
            //OLEDC_COLOR_VIOLET
            R_LED = 238;
            G_LED = 130;
            B_LED = 238;
            break;
        case Z:
            //OLEDC_COLOR_CYAN
            R_LED = 0;
            G_LED = 255;
            B_LED = 255;
            break;
        case T:
            //OLEDC_COLOR_GREEN
            R_LED = 0;
            G_LED = 255;
            B_LED = 0;
            break;
        case S:
            //OLEDC_COLOR_YELLOW
            R_LED = 255;
            G_LED = 255;
            B_LED = 0;
            break;
        case X:
            //
            break;
    }
}

void TimerInitialize()
{
    INTCON2=0x8000; //Enable
    INTCON1=0x0;
    IFS0bits.T1IF=0;      //  OFF Interrupt Flag
    IEC0bits.T1IE=1;      //  Enable interrupt    
    T1CONbits.TSIDL=1;    //Timer1 stop in idle mode bit
    T1CONbits.TON=1;      // Timer ON
    T1CONbits.TCS=0;      //Ignore- synchronize the external clock input
    //T1CONbits.TCKPS=2;   
    TimerPeriod = 2.4;
}

void DrawBlock(block_name _name, uint8_t x, uint8_t y)
{
    switch (_name)
    {
        case O:
            Draw_O_Block(x,y, 1);
            break;
        case L:
            Draw_L_Block(x,y, 1);
            break;
        case J:
            Draw_J_Block(x,y, 1);
            break;
        case I:
            Draw_I_Block(x,y, 1);
            break;
        case Z:
            Draw_Z_Block(x,y, 1);
            break;
        case T:
            Draw_T_Block(x,y, 1);
            break;
        case S:
            Draw_S_Block(x,y, 1);
            break;
        case X:
            break;
    }
}

void drawGrid()
{
    //MINI_BLOCK_SIZE_2  7
    //MAX_BORDER 95
    //MAX_BLOCKS_ROW 12
    //oledC_DrawLine(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, uint8_t width, uint16_t color)
    //The function draw grid of 12*12 (one block size 8*8)
    
    int i = 0;
    int block = 8;
    //Print rows
    for (i=0;i<=MAX_BLOCKS_ROW; i++)
    {
        oledC_DrawLine(0, i*block, MAX_BORDER, i*block, 1, OLEDC_COLOR_SLATEGREY);
    }
    //Print columns
    for (i=0;i<=MAX_BLOCKS_ROW  ; i++)
    {
        oledC_DrawLine(i*block,0,i*block, MAX_BORDER, 1, OLEDC_COLOR_SLATEGREY);
    }
}

/*BOOL CheckBlock_Lower_borderOK()
{
    //uint16_t oledC_ReadPoint(uint8_t x, uint8_t y)
    int i=0;
    int limit = Global_X+ 6*4;
    for (i=Global_X; i< limit; i++)
    {
        if (oledC_ReadPoint(i, Global_Y+1)!= BackgroundColor)
        {
            return FALSE;
        }
    }
    return TRUE;
}*/
      
BOOL checkFreeBlockAeraUnder(Blocks_Data tmp)
{
    int i=0;
    int x;
    int y;
    //Check for each mini block
    for (i=0; i<4; i++)
    {
        int y = tmp.block[i][1];
        int x = tmp.block[i][0];
        if (x >= 0 && x < MAX_BLOCKS_ROW && y >= 0 && y < MAX_BLOCKS_ROW) 
        {
            //if not empty - X is mean black, return FALSE
            if (LCD_Board[x+1][y] != X)
                return FALSE;
        }
        //Check out of border
    }
    return TRUE;
}

BOOL checkFreeBlockAeraRL(Blocks_Data temp, char direction)
{
    int i=0;
    if (direction == 'R')
    {
        //Check for each mini block
        for (i=0; i<4; i++)
        {
            int y = temp.block[i][1];
            int x = temp.block[i][0];
            if (x >= 0 && x < MAX_BLOCKS_ROW && y >= 0 && y < MAX_BLOCKS_ROW-1) 
            {
                //if not empty - X is mean black, return FALSE
                if (LCD_Board[x][y+1] != X)
                    return FALSE;
            }
            //Check out of border
        }
    }
    else if (direction == 'L')
    {
        //Check for each mini block
        for (i=0; i<4; i++)
        {
            int y = temp.block[i][1];
            int x = temp.block[i][0];
            if (x >= 0 && x < MAX_BLOCKS_ROW && y > 0 && y < MAX_BLOCKS_ROW) 
            {
                //if not empty - X is mean black, return FALSE
                if (LCD_Board[x][y-1] != X)
                    return FALSE;
            }
            //Check out of border
        }    
    }
    return TRUE;
}
//Timer T1 Interrupt
void __attribute__((interrupt))_T1Interrupt(void)
{
    Global_Counter++;
    levelTimer++;
    if (Global_Counter==1) 
    {
        showBlock(G_CurrentBlock);
        //O, L, J, I, Z, T, S
        G_NextBlock = ChoseCurrentBlockRef();
        Turn_Little_LED(G_NextBlock);
        //Draw new Random Block
    }
    //Finish falling
            //If the block has reached the lower limit   OR   There's a block under me
    else if (checkBoardBorders(G_CurrentBlock, 'D')== FALSE || checkFreeBlockAeraUnder(G_CurrentBlock) == FALSE)
    { 
        saveBlockInBorad(G_CurrentBlock);
        searchFullRows();
        checkGameOver();
        //Refresh - fix dead pixels 
        drawBoard(LCD_Board);
        //
        roateRight = 0;
        roateLeft = 0;
        G_CurrentBlock = G_NextBlock;
        G_Temp = G_CurrentBlock;
        Global_Counter = 0;
    }
    else 
    {
        //Fall of the block down
        moveBlock(&G_CurrentBlock,'D');
    }
    //Falling Speed
    if (levelTimer == timer_limit)
    {
        //Runs the timer faster - T1CONbits.TCKPS
        if (TimerPeriod > 2)
        {
            //Reduces the period
            TimerPeriod -= 0.2;
            //Resets the time counter
            levelTimer = 0;
            //Increases the limit of time change
            timer_limit = timer_limit*2;
        }
        else
            TimerPeriod == 2;
    }
    /* Falling Speed end */
    
    
    IFS0bits.T1IF=0; //flag of interrupt=0
}

void __attribute__((interrupt))_IOCInterrupt(void)
{
    //Restart game by pressing both buttons
    if ((IOCFAbits.IOCFA12 == 1 && IOCFAbits.IOCFA11 == 1) || (IOCFAbits.IOCFA11 == 1 && IOCFAbits.IOCFA12 == 1))
    {
        PORTAbits.RA8 = 1; //Turns on RA8
        PORTAbits.RA9 = 1; //Turns on RA9
        userRestart();
        restartGame();
        IOCFAbits.IOCFA11 = 0;
        IOCFAbits.IOCFA12 = 0;
        PORTAbits.RA9 = 0; //Turns on RA8
        PORTAbits.RA8 = 0; //Turns on RA9
    }
    //S1 Pressed
    else if (IOCFAbits.IOCFA11 == 1)
    {
        if (G_CurrentBlock.name == L || G_CurrentBlock.name == Z || G_CurrentBlock.name == T || G_CurrentBlock.name == J || G_CurrentBlock.name == S || G_CurrentBlock.name == I)
        {
            roateLeft++;
            hideBlock(G_CurrentBlock);
            roateBlockLeft(&G_CurrentBlock);
            roateBlockLeft(&G_CurrentBlock);
            showBlock(G_CurrentBlock);
        }
        IOCFAbits.IOCFA11 = 0;
    }
    //S2 Pressed
    else if (IOCFAbits.IOCFA12 == 1)
    {
        if (G_CurrentBlock.name == L || G_CurrentBlock.name == Z || G_CurrentBlock.name == T || G_CurrentBlock.name == J || G_CurrentBlock.name == S || G_CurrentBlock.name == I)
        {
            roateRight++;
            hideBlock(G_CurrentBlock);
            roateBlockRight(&G_CurrentBlock);
            roateBlockRight(&G_CurrentBlock);
            showBlock(G_CurrentBlock);
        }
        IOCFAbits.IOCFA12 = 0;
    }
    IFS1bits.IOCIF = 0;
}

void WelcomeShow()
{
    oledC_clearScreen();
    Draw_O_Block(5,5,1);
    DELAY_milliseconds(700);
    Draw_I_Block(30,30,1);  
    DELAY_milliseconds(700);
    Draw_J_Block(50,50,1);
    DELAY_milliseconds(700);
    Draw_L_Block(0,40,1);
    DELAY_milliseconds(700);
    Draw_T_Block(0,80,1);
    DELAY_milliseconds(700);
    Draw_S_Block(40,80,1);
    DELAY_milliseconds(700);
    Draw_Z_Block(70,80,1);
    oledC_clearScreen();
    uint8_t x, y;
    
    R_LED = 255; G_LED = 0;B_LED = 255;
    for (x=0 ; x<=94 ; ++x)
        oledC_DrawLine(47, 47, x, 0, 1, OLEDC_COLOR_FUCHSIA);
    R_LED = 255; G_LED = 0;B_LED = 0;
    for (x=0 ; x<=94 ; ++x)
        oledC_DrawLine(47, 47, x, 94, 1, OLEDC_COLOR_RED);
    R_LED = 0; G_LED = 255;B_LED = 255;
    for (y=0 ; y<94 ; ++y)
        oledC_DrawLine(47, 47, 0, y, 1, OLEDC_COLOR_CYAN);
    R_LED = 148; G_LED = 0;B_LED = 211;
    for (y=0 ; y<94 ; ++y)
        oledC_DrawLine(47, 47, 94, y, 1, OLEDC_COLOR_DARKVIOLET);
    DELAY_milliseconds(1000);
    char * strPrint = "TETRIS";
                   //x  y
    oledC_DrawString(0, 10, 3 , 3, strPrint, BackgroundColor);
    strPrint = " Game";
    oledC_DrawString(6, 45, 3, 3, strPrint, BackgroundColor);
    DELAY_milliseconds(2000);
    R_LED = 50; G_LED = 50;B_LED = 50;
    oledC_clearScreen();
}


void SlidingControl()
{
    const int sensitivity = 15;
    //Potentiometer change
    new_pmValue = getPotentiometerValue();
    int check_difference = new_pmValue - pmValue;
    //If the change is above +/- sensitivity
    /* After sampling, I test the effect of the change,
      if it is +/- sensitivity, then I affect the movement of the block. */
    pmValue = new_pmValue;
    if (check_difference > sensitivity)
    {
        if (checkFreeBlockAeraRL(G_CurrentBlock,'R') == TRUE)
            moveBlock(&G_CurrentBlock,'R');
    }
    else if (check_difference< -sensitivity)
    {
       if (checkFreeBlockAeraRL(G_CurrentBlock,'L') == TRUE)
        moveBlock(&G_CurrentBlock,'L');
    }
    //END Potentiometer change
}

//OK - redraws the board according to the matrix I saved
void drawBoard(block_name boardi[MAX_BLOCKS_ROW][MAX_BLOCKS_ROW])
{
        int i=0;
        int j=0;
        //oledC_DrawRectangle(uint8_t start_x, uint8_t start_y, uint8_t end_x, uint8_t end_y, uint16_t color)
        for (i=0; i<12; i++)
            for (j=0; j<12; j++)
        {
            uint8_t start_x = j*MINI_BLOCK_SIZE_2;
            uint8_t start_y = i*MINI_BLOCK_SIZE_2;
            uint8_t end_x = start_x + MINI_BLOCK_SIZE_2;
            uint8_t end_y = start_y + MINI_BLOCK_SIZE_2;
            block_name tmp = boardi[i][j];
            uint16_t color = getColor(tmp);
            oledC_DrawRectangle(start_x,  start_y, end_x, end_y, color);
        }
}

void earceGUI_Borad()
{
   oledC_DrawRectangle(0, 0, 96, 96, oledC_getBackground());  
}

BOOL checkIF_Full_COLOR_Solid_COL(int col)
{
    int i=0;
    for (i=0; i<12; i++)
    {
        // X = BLACK
        // IF X STOP CHECK
        if (LCD_Board[i][col] == X)
            return FALSE;
    }
    return TRUE;
}


    //if T is mean the row is full COLOR!
BOOL checkIF_Full_COLOR_SolidRow(int row)
{
    int i=0;
    for (i=0; i<12; i++)
    {
        // X = BLACK
        // IF X STOP CHECK
        if (LCD_Board[row][i] == X)
            return FALSE;
    }
    return TRUE;
}

void flashRow(int row)
{
    int i=0;
    int j=0;
    block_name temp_Board[MAX_BLOCKS_ROW][MAX_BLOCKS_ROW];
    
    //Copy the main brain
    for (i=0; i<12; i++)
        for (j=0; j<12; j++)
            temp_Board[i][j] = LCD_Board[i][j];
    
    //white line
    for (i=0; i<12; i++)
    {
        temp_Board[row][i] = W;
    }
    
    drawBoard(LCD_Board);
    DELAY_milliseconds(200);
    drawBoard(temp_Board);
    DELAY_milliseconds(200);
    drawBoard(LCD_Board);
    DELAY_milliseconds(200);
    drawBoard(temp_Board);
    DELAY_milliseconds(200);
}
void deleteRow(int row)
{
    int i=0;
    int j=0;
    int r=row;
    block_name temp_Board[MAX_BLOCKS_ROW][MAX_BLOCKS_ROW];

    //Flash delete row    
    flashRow(row);
    
    //Init LCD_Board_new Set X as default
    for (i=0; i<12; i++)
        for (j=0; j<12; j++)
            temp_Board[i][j] = X;
    
    //Copying part below the deleted row
    for (i=11; i > row; i--)
        for (j=0; j < 12; j++)
        {temp_Board[i][j] =  LCD_Board[i][j];}

    //Copies part above the deleted row
    for (r=row; r>=1; r--)
      for (j=0; j< 12; j++)
          temp_Board[r][j] =  LCD_Board[r-1][j];
    
    //Updating the main brain
    for (i=0; i<12; i++)
        for (j=0; j<12; j++)
            LCD_Board[i][j] = temp_Board[i][j];
}

void searchFullRows()
{
    //Check if we get a full row
    int i=0;
    for (i = 0; i < 12; i++)
    {
        if (checkIF_Full_COLOR_SolidRow(i)== TRUE)
        {
            deleteRow(i);
            earceGUI_Borad();
            drawBoard(LCD_Board);
        }
    }
}

//If there is a full column to the end, the game is over
void checkGameOver()
{
    //Check if we get a full col
    int i=0;
    for (i = 0; i < 12; i++)
    {
        if (checkIF_Full_COLOR_Solid_COL(i)== TRUE)
        {
            gameoverMsg();
            restartGame();
        }
    }
}

//OK - save the block in the memo
void saveBlockInBorad(Blocks_Data tmp)
{
    int i=0;
    for (i=0; i<4; i++)
    {
        int y = tmp.block[i][1];
        int x = tmp.block[i][0];
        LCD_Board[x][y] = G_CurrentBlock.name;
    }
}

void initBoard()
{
    int i=0;
    int j=0;
    //Set X as default
    for (i=0; i<12; i++)
        for (j=0; j<12; j++)
            LCD_Board[i][j] = X;
}

void gameoverMsg()
{
    earceGUI_Borad();
    char * strPrint = "GAME";
                   //x  y
    oledC_DrawString(6, 10, 3, 3, strPrint, OLEDC_COLOR_RED);
    strPrint = "OVER";
    oledC_DrawString(6, 45, 3, 3, strPrint, OLEDC_COLOR_RED);
    DELAY_milliseconds(3000);
}

void userRestart()
{
    earceGUI_Borad();
    char * strPrint = "Restart";
                   //x  y
    oledC_DrawString(10, 10, 2, 2, strPrint, OLEDC_COLOR_RED);
    strPrint = "Game";
    oledC_DrawString(10, 45, 2, 2, strPrint, OLEDC_COLOR_RED);
    DELAY_milliseconds(3000);
}

void restartGame()
{
    timer_limit = 60;
    FallingSpeed = 1;
    levelTimer = 0;
    roateLeft = 0;
    roateRight = 0;
    Global_Counter = 0;
    //Initialize System
    SYSTEM_Initialize();
    InitializeSystemAndLED();
    // Use current time as seed for random generator
    srand(time(0));
    WelcomeShow();
    TimerInitialize();
    //Set Background
    oledC_clearScreen();
    oledC_setBackground(BackgroundColor);
    
    G_CurrentBlock = ChoseCurrentBlockRef();
    G_Temp = G_CurrentBlock;
    Global_Counter = 0;
    initBoard();
}


/*
                         Main application
 */
int main(void)
{
    //All settings for restart the game
    restartGame();
    //Main loop
    while(1)
    {  
        SlidingControl();
    }
    return 1;
}
/**
 End of File
*/

