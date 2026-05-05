#include <xc.h>
#define _XTAL_FREQ 8000000

#pragma config FOSC = INTOSC_EC
#pragma config WDT  = OFF
#pragma config LVP  = OFF
#pragma config PBADEN = OFF
#pragma config MCLRE = OFF

/* I2C software */
#define SDA_DIR TRISBbits.TRISB0
#define SCL_DIR TRISBbits.TRISB1

#define SDA LATBbits.LATB0
#define SCL LATBbits.LATB1

#define BME280_ADDR 0xEC   // 0x76 << 1

void I2C_Delay(){ __delay_us(5); }

void I2C_Start(){
    SDA_DIR = 0; SCL_DIR = 0;
    SDA = 1; SCL = 1;
    I2C_Delay();
    SDA = 0;
    I2C_Delay();
    SCL = 0;
}

void I2C_Stop(){
    SDA = 0;
    SCL = 1;
    I2C_Delay();
    SDA = 1;
}

void I2C_Write(unsigned char data){
    for(char i=0;i<8;i++){
        SDA = (data & 0x80) ? 1 : 0;
        SCL = 1; I2C_Delay();
        SCL = 0; I2C_Delay();
        data <<= 1;
    }
    SDA_DIR = 1;
    SCL = 1; I2C_Delay();
    SCL = 0;
    SDA_DIR = 0;
}

unsigned char I2C_Read(){
    unsigned char data = 0;
    SDA_DIR = 1;
    for(char i=0;i<8;i++){
        SCL = 1; I2C_Delay();
        data <<= 1;
        if(PORTBbits.RB0) data |= 1;
        SCL = 0; I2C_Delay();
    }
    SDA_DIR = 0;
    SDA = 1; SCL = 1; I2C_Delay();
    SCL = 0;
    return data;
}

/* BME280 funciones básicas */
void BME_Write(unsigned char reg, unsigned char val){
    I2C_Start();
    I2C_Write(BME280_ADDR);
    I2C_Write(reg);
    I2C_Write(val);
    I2C_Stop();
}

unsigned char BME_Read(unsigned char reg){
    unsigned char val;
    I2C_Start();
    I2C_Write(BME280_ADDR);
    I2C_Write(reg);
    I2C_Start();
    I2C_Write(BME280_ADDR | 1);
    val = I2C_Read();
    I2C_Stop();
    return val;
}

/* MAIN */
void main(){

    unsigned int humedad;

    OSCCON = 0x72;

    TRISDbits.TRISD0 = 0;

    /* Configuración mínima */
    BME_Write(0xF2, 0x01);  // humedad oversampling
    BME_Write(0xF4, 0x27);  // modo normal

    while(1){

        /* lectura simple (sin compensación real) */
        unsigned char msb = BME_Read(0xFD);
        unsigned char lsb = BME_Read(0xFE);

        humedad = ((unsigned int)msb << 8) | lsb;

        if(humedad > 30000){
            LATDbits.LATD0 = 1;
        } else {
            LATDbits.LATD0 = 0;
        }

        __delay_ms(500);
    }
}