#include <xc.h>
#define _XTAL_FREQ 8000000

#pragma config FOSC = INTOSC_EC
#pragma config WDT  = OFF
#pragma config LVP  = OFF
#pragma config PBADEN = OFF
#pragma config MCLRE = OFF

/* ========== OLED ========== */
#define OLED_ADDR 0x78

/* ========== I2C SOFTWARE ========== */
#define SDA TRISBbits.TRISB0
#define SCL TRISBbits.TRISB1

#define SDA_OUT LATBbits.LATB0
#define SCL_OUT LATBbits.LATB1

#define SDA_IN  PORTBbits.RB0

void I2C_Delay(){ __delay_us(5); }

void I2C_Start(){
    SDA = 0; SCL = 0;
    SDA_OUT = 1; SCL_OUT = 1;
    I2C_Delay();
    SDA_OUT = 0;
    I2C_Delay();
    SCL_OUT = 0;
}

void I2C_Stop(){
    SDA = 0;
    SCL_OUT = 0;
    SDA_OUT = 0;
    I2C_Delay();
    SCL_OUT = 1;
    I2C_Delay();
    SDA_OUT = 1;
}

void I2C_Write(unsigned char data){
    for(char i=0;i<8;i++){
        SDA_OUT = (data & 0x80) ? 1 : 0;
        SCL_OUT = 1; I2C_Delay();
        SCL_OUT = 0; I2C_Delay();
        data <<= 1;
    }
    SDA = 1;
    SCL_OUT = 1; I2C_Delay();
    SCL_OUT = 0;
    SDA = 0;
}

/* ========== OLED ========== */
void OLED_Comando(unsigned char cmd){
    I2C_Start();
    I2C_Write(OLED_ADDR);
    I2C_Write(0x00);
    I2C_Write(cmd);
    I2C_Stop();
}

void OLED_Dato(unsigned char d){
    I2C_Start();
    I2C_Write(OLED_ADDR);
    I2C_Write(0x40);
    I2C_Write(d);
    I2C_Stop();
}

void OLED_SetCursor(unsigned char p, unsigned char c){
    OLED_Comando(0xB0 | p);
    OLED_Comando(c & 0x0F);
    OLED_Comando(0x10 | (c >> 4));
}

void OLED_Clear(){
    for(char i=0;i<8;i++){
        OLED_SetCursor(i,0);
        for(char j=0;j<128;j++) OLED_Dato(0x00);
    }
}

void OLED_Init(){
    __delay_ms(100);
    OLED_Comando(0xAE);
    OLED_Comando(0xA6);
    OLED_Comando(0xAF);
    OLED_Clear();
}

/* ========== TEXTO SIMPLE ========== */
void OLED_Char(char c){
    for(char i=0;i<5;i++) OLED_Dato(0xFF); // simple bloque
    OLED_Dato(0x00);
}

void OLED_String(char p,char c,const char *t){
    OLED_SetCursor(p,c);
    while(*t) OLED_Char(*t++);
}

void OLED_Numero(char p,char c,unsigned int num){
    char buf[6];
    int i=0;
    if(num==0){ OLED_String(p,c,"0"); return; }
    while(num>0){ buf[i++]=(num%10)+'0'; num/=10; }
    OLED_SetCursor(p,c);
    while(i--) OLED_Char(buf[i]);
}

/* ========== ADC ========== */
#define CANAL_LM35 0
#define CANAL_LDR  1
#define CANAL_MQ135 2

#define ADC_MUESTRAS 8
#define AIRE_UMBRAL 400

void ADC_Init(){
    TRISAbits.TRISA0 = 1;
    TRISAbits.TRISA1 = 1;
    TRISAbits.TRISA2 = 1;

    ADCON1 = 0x0C;
    ADCON2 = 0xBD;
    ADCON0 = 0x01;
}

unsigned int ADC_Read(unsigned char c){
    ADCON0 = (ADCON0 & 0xC3) | (c << 2);
    __delay_us(50);
    ADCON0bits.GO = 1;
    while(ADCON0bits.GO);
    return ((ADRESH<<8)|ADRESL);
}

unsigned int ADC_ReadAvg(unsigned char c){
    unsigned long s=0;
    for(char i=0;i<ADC_MUESTRAS;i++){
        s+=ADC_Read(c);
        __delay_ms(1);
    }
    return s/ADC_MUESTRAS;
}

/* ========== MAIN ========== */
void main(){

    unsigned int t,l,a;
    unsigned int temp;

    OSCCON = 0x72;

    ADC_Init();
    OLED_Init();

    while(1){

        t = ADC_ReadAvg(CANAL_LM35);
        l = ADC_ReadAvg(CANAL_LDR);
        a = ADC_ReadAvg(CANAL_MQ135);

        temp = (t * 500) / 1024;

        OLED_Clear();

        OLED_String(0,0,"TEMP");
        OLED_Numero(2,0,temp);

        OLED_String(4,0,"LUZ");
        OLED_Numero(4,40,l);

        OLED_String(6,0,"AIRE");

        if(a > AIRE_UMBRAL)
            OLED_String(6,40,"MALO");
        else
            OLED_String(6,40,"OK");

        __delay_ms(500);
    }
}