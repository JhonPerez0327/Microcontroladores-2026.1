#include <xc.h>
#define _XTAL_FREQ 8000000

#pragma config FOSC = INTOSC_EC
#pragma config WDT  = OFF
#pragma config LVP  = OFF
#pragma config PBADEN = OFF
#pragma config MCLRE = OFF

/* ========== ADC ========== */
#define CANAL_LM35   0
#define CANAL_LDR    1
#define CANAL_MQ135  2
#define ADC_MUESTRAS 8

#define TEMP_MIN_ADC 60
#define TEMP_MAX_ADC 80
#define LUZ_UMBRAL   500
#define AIRE_UMBRAL  400

/* ========== LEDS ========== */
#define LED_TEMP     LATDbits.LATD0
#define LED_LUZ      LATDbits.LATD1
#define LED_AIRE     LATDbits.LATD2
#define LED_TEMP_DIR TRISDbits.TRISD0
#define LED_LUZ_DIR  TRISDbits.TRISD1
#define LED_AIRE_DIR TRISDbits.TRISD2

/* ========== I2C SOFTWARE (RB0=SDA, RB1=SCL) ========== */
#define SDA_PIN  PORTBbits.RB0
#define SDA_DIR  TRISBbits.TRISB0
#define SCL_DIR  TRISBbits.TRISB1
#define SDA      LATBbits.LATB0
#define SCL      LATBbits.LATB1

void I2C_Delay(){ __delay_us(5); }

void I2C_Start(){
    SDA_DIR = 0; SCL_DIR = 0;
    SDA = 1; SCL = 1; I2C_Delay();
    SDA = 0; I2C_Delay();
    SCL = 0;
}

void I2C_Stop(){
    SDA_DIR = 0;
    SDA = 0;
    SCL = 1; I2C_Delay();
    SDA = 1; I2C_Delay();
}

/*
 * Retorna 0 si el esclavo hizo ACK (SDA=0), 1 si NACK.
 * Esto es clave para detectar si el BME280 está conectado.
 */
unsigned char I2C_Write(unsigned char data){
    unsigned char ack;
    for(char i = 0; i < 8; i++){
        SDA_DIR = 0;
        SDA = (data & 0x80) ? 1 : 0;
        SCL = 1; I2C_Delay();
        SCL = 0; I2C_Delay();
        data <<= 1;
    }
    /* Leer ACK */
    SDA_DIR = 1;        // SDA como entrada
    SDA = 1;            // pull-up
    SCL = 1; I2C_Delay();
    ack = SDA_PIN;      // 0=ACK, 1=NACK
    SCL = 0; I2C_Delay();
    SDA_DIR = 0;
    return ack;         // 0 = OK, 1 = sin respuesta
}

unsigned char I2C_Read(unsigned char ultimo){
    unsigned char dato = 0;
    SDA_DIR = 1;
    for(char i = 0; i < 8; i++){
        dato <<= 1;
        SCL = 1; I2C_Delay();
        if(SDA_PIN) dato |= 1;
        SCL = 0; I2C_Delay();
    }
    SDA_DIR = 0;
    SDA = ultimo ? 1 : 0;   // NACK en último byte, ACK si hay más
    SCL = 1; I2C_Delay();
    SCL = 0; I2C_Delay();
    return dato;
}

/* ========== OLED SSD1306 ========== */
#define OLED_ADDR 0x78

/*
 * Fuente 5x8 ? caracteres ASCII desde espacio (32) hasta 'z' (122).
 * Solo incluimos los que necesitamos para ahorrar ROM.
 * Incluye letras minúsculas, números, ':', '%' y '°' (usamos 'o' pequeño).
 */
const unsigned char font5x8[][5] = {
/* 32 ' '  */ {0x00,0x00,0x00,0x00,0x00},
/* 33 '!'  */ {0x00,0x00,0x5F,0x00,0x00},
/* 34 '"'  */ {0x00,0x07,0x00,0x07,0x00},
/* 35 '#'  */ {0x14,0x7F,0x14,0x7F,0x14},
/* 36 '$'  */ {0x24,0x2A,0x7F,0x2A,0x12},
/* 37 '%'  */ {0x23,0x13,0x08,0x64,0x62},
/* 38 '&'  */ {0x36,0x49,0x55,0x22,0x50},
/* 39 '\'' */ {0x00,0x05,0x03,0x00,0x00},
/* 40 '('  */ {0x00,0x1C,0x22,0x41,0x00},
/* 41 ')'  */ {0x00,0x41,0x22,0x1C,0x00},
/* 42 '*'  */ {0x08,0x2A,0x1C,0x2A,0x08},
/* 43 '+'  */ {0x08,0x08,0x3E,0x08,0x08},
/* 44 ','  */ {0x00,0x50,0x30,0x00,0x00},
/* 45 '-'  */ {0x08,0x08,0x08,0x08,0x08},
/* 46 '.'  */ {0x00,0x60,0x60,0x00,0x00},
/* 47 '/'  */ {0x20,0x10,0x08,0x04,0x02},
/* 48 '0'  */ {0x3E,0x51,0x49,0x45,0x3E},
/* 49 '1'  */ {0x00,0x42,0x7F,0x40,0x00},
/* 50 '2'  */ {0x42,0x61,0x51,0x49,0x46},
/* 51 '3'  */ {0x21,0x41,0x45,0x4B,0x31},
/* 52 '4'  */ {0x18,0x14,0x12,0x7F,0x10},
/* 53 '5'  */ {0x27,0x45,0x45,0x45,0x39},
/* 54 '6'  */ {0x3C,0x4A,0x49,0x49,0x30},
/* 55 '7'  */ {0x01,0x71,0x09,0x05,0x03},
/* 56 '8'  */ {0x36,0x49,0x49,0x49,0x36},
/* 57 '9'  */ {0x06,0x49,0x49,0x29,0x1E},
/* 58 ':'  */ {0x00,0x36,0x36,0x00,0x00},
/* 59 ';'  */ {0x00,0x56,0x36,0x00,0x00},
/* 60 '<'  */ {0x00,0x08,0x14,0x22,0x41},
/* 61 '='  */ {0x14,0x14,0x14,0x14,0x14},
/* 62 '>'  */ {0x41,0x22,0x14,0x08,0x00},
/* 63 '?'  */ {0x02,0x01,0x51,0x09,0x06},
/* 64 '@'  */ {0x32,0x49,0x79,0x41,0x3E},
/* 65 'A'  */ {0x7E,0x11,0x11,0x11,0x7E},
/* 66 'B'  */ {0x7F,0x49,0x49,0x49,0x36},
/* 67 'C'  */ {0x3E,0x41,0x41,0x41,0x22},
/* 68 'D'  */ {0x7F,0x41,0x41,0x22,0x1C},
/* 69 'E'  */ {0x7F,0x49,0x49,0x49,0x41},
/* 70 'F'  */ {0x7F,0x09,0x09,0x09,0x01},
/* 71 'G'  */ {0x3E,0x41,0x41,0x49,0x7A},
/* 72 'H'  */ {0x7F,0x08,0x08,0x08,0x7F},
/* 73 'I'  */ {0x00,0x41,0x7F,0x41,0x00},
/* 74 'J'  */ {0x20,0x40,0x41,0x3F,0x01},
/* 75 'K'  */ {0x7F,0x08,0x14,0x22,0x41},
/* 76 'L'  */ {0x7F,0x40,0x40,0x40,0x40},
/* 77 'M'  */ {0x7F,0x02,0x04,0x02,0x7F},
/* 78 'N'  */ {0x7F,0x04,0x08,0x10,0x7F},
/* 79 'O'  */ {0x3E,0x41,0x41,0x41,0x3E},
/* 80 'P'  */ {0x7F,0x09,0x09,0x09,0x06},
/* 81 'Q'  */ {0x3E,0x41,0x51,0x21,0x5E},
/* 82 'R'  */ {0x7F,0x09,0x19,0x29,0x46},
/* 83 'S'  */ {0x46,0x49,0x49,0x49,0x31},
/* 84 'T'  */ {0x01,0x01,0x7F,0x01,0x01},
/* 85 'U'  */ {0x3F,0x40,0x40,0x40,0x3F},
/* 86 'V'  */ {0x1F,0x20,0x40,0x20,0x1F},
/* 87 'W'  */ {0x3F,0x40,0x38,0x40,0x3F},
/* 88 'X'  */ {0x63,0x14,0x08,0x14,0x63},
/* 89 'Y'  */ {0x03,0x04,0x78,0x04,0x03},
/* 90 'Z'  */ {0x61,0x51,0x49,0x45,0x43},
/* 91 '['  */ {0x00,0x00,0x00,0x00,0x00},
/* 92 '\\' */ {0x00,0x00,0x00,0x00,0x00},
/* 93 ']'  */ {0x00,0x00,0x00,0x00,0x00},
/* 94 '^'  */ {0x00,0x00,0x00,0x00,0x00},
/* 95 '_'  */ {0x40,0x40,0x40,0x40,0x40},
/* 96 '`'  */ {0x00,0x00,0x00,0x00,0x00},
/* 97 'a'  */ {0x20,0x54,0x54,0x54,0x78},
/* 98 'b'  */ {0x7F,0x48,0x44,0x44,0x38},
/* 99 'c'  */ {0x38,0x44,0x44,0x44,0x20},
/*100 'd'  */ {0x38,0x44,0x44,0x48,0x7F},
/*101 'e'  */ {0x38,0x54,0x54,0x54,0x18},
/*102 'f'  */ {0x08,0x7E,0x09,0x01,0x02},
/*103 'g'  */ {0x08,0x14,0x54,0x54,0x3C},
/*104 'h'  */ {0x7F,0x08,0x04,0x04,0x78},
/*105 'i'  */ {0x00,0x44,0x7D,0x40,0x00},
/*106 'j'  */ {0x20,0x40,0x44,0x3D,0x00},
/*107 'k'  */ {0x7F,0x10,0x28,0x44,0x00},
/*108 'l'  */ {0x00,0x41,0x7F,0x40,0x00},
/*109 'm'  */ {0x7C,0x04,0x18,0x04,0x78},
/*110 'n'  */ {0x7C,0x08,0x04,0x04,0x78},
/*111 'o'  */ {0x38,0x44,0x44,0x44,0x38},
/*112 'p'  */ {0x7C,0x14,0x14,0x14,0x08},
/*113 'q'  */ {0x08,0x14,0x14,0x18,0x7C},
/*114 'r'  */ {0x7C,0x08,0x04,0x04,0x08},
/*115 's'  */ {0x48,0x54,0x54,0x54,0x20},
/*116 't'  */ {0x04,0x3F,0x44,0x40,0x20},
/*117 'u'  */ {0x3C,0x40,0x40,0x40,0x7C},
/*118 'v'  */ {0x1C,0x20,0x40,0x20,0x1C},
/*119 'w'  */ {0x3C,0x40,0x30,0x40,0x3C},
/*120 'x'  */ {0x44,0x28,0x10,0x28,0x44},
/*121 'y'  */ {0x0C,0x50,0x50,0x50,0x3C},
/*122 'z'  */ {0x44,0x64,0x54,0x4C,0x44},
};

/* ---------- Funciones OLED ---------- */

void OLED_Comando(unsigned char c){
    I2C_Start();
    I2C_Write(OLED_ADDR);
    I2C_Write(0x00);
    I2C_Write(c);
    I2C_Stop();
}

void OLED_Dato(unsigned char d){
    I2C_Start();
    I2C_Write(OLED_ADDR);
    I2C_Write(0x40);
    I2C_Write(d);
    I2C_Stop();
}

void OLED_SetCursor(unsigned char pag, unsigned char col){
    OLED_Comando(0xB0 | pag);
    OLED_Comando(col & 0x0F);
    OLED_Comando(0x10 | (col >> 4));
}

void OLED_Clear(){
    for(unsigned char i = 0; i < 8; i++){
        OLED_SetCursor(i, 0);
        for(unsigned char j = 0; j < 128; j++) OLED_Dato(0x00);
    }
}

void OLED_Init(){
    __delay_ms(150);
    unsigned char cmds[] = {
        0xAE,       // display off
        0xD5,0x80,  // clock div
        0xA8,0x3F,  // multiplex
        0xD3,0x00,  // display offset
        0x40,       // start line
        0x8D,0x14,  // charge pump ON
        0x20,0x00,  // horizontal addressing
        0xA1,       // seg remap
        0xC8,       // com scan dir
        0xDA,0x12,  // com pins
        0x81,0xCF,  // contrast
        0xD9,0xF1,  // precharge
        0xDB,0x40,  // vcom detect
        0xA4,       // all on resume
        0xA6,       // normal display
        0xAF        // display ON
    };
    for(unsigned char i = 0; i < sizeof(cmds); i++)
        OLED_Comando(cmds[i]);
    OLED_Clear();
}

void OLED_Char(unsigned char c){
    if(c < 32 || c > 122) c = '?';
    for(unsigned char i = 0; i < 5; i++)
        OLED_Dato(font5x8[c - 32][i]);
    OLED_Dato(0x00); // espacio entre caracteres
}

void OLED_Str(unsigned char pag, unsigned char col, const char *s){
    OLED_SetCursor(pag, col);
    while(*s) OLED_Char(*s++);
}

/* Imprime número entero sin signo en posición dada */
void OLED_Num(unsigned char pag, unsigned char col, unsigned int n){
    char buf[6];
    unsigned char i = 0;
    if(n == 0){ buf[i++] = '0'; }
    else { while(n > 0){ buf[i++] = '0' + (n % 10); n /= 10; } }
    OLED_SetCursor(pag, col);
    while(i--) OLED_Char(buf[i]);
}

/* Borra una línea completa (página) */
void OLED_ClearLine(unsigned char pag){
    OLED_SetCursor(pag, 0);
    for(unsigned char j = 0; j < 128; j++) OLED_Dato(0x00);
}

/* ========== BME280 ========== */
#define BME_ADDR  0xEC   // SDO a GND ? 0x76 << 1 = 0xEC
                         // SDO a VCC ? 0x77 << 1 = 0xEE

/* Variables de calibración */
static unsigned int  dig_T1;
static int           dig_T2, dig_T3;
static unsigned int  dig_H1, dig_H3;
static int           dig_H2, dig_H4, dig_H5, dig_H6;

static unsigned char bme_ok = 0;  // 1 si el BME respondió

/*
 * Intenta escribir en el BME. Devuelve 0 si el dispositivo hizo ACK.
 * Si no hay ACK (NACK), cancela el bus y retorna 1.
 */
unsigned char BME_WriteReg(unsigned char reg, unsigned char val){
    unsigned char nack;
    I2C_Start();
    nack = I2C_Write(BME_ADDR);
    if(nack){ I2C_Stop(); return 1; }
    I2C_Write(reg);
    I2C_Write(val);
    I2C_Stop();
    return 0;
}

/*
 * Lee 'len' bytes del BME a partir del registro 'reg'.
 * Retorna 1 si el BME no respondió.
 */
unsigned char BME_ReadRegs(unsigned char reg, unsigned char *buf, unsigned char len){
    unsigned char nack;
    I2C_Start();
    nack = I2C_Write(BME_ADDR);
    if(nack){ I2C_Stop(); return 1; }
    I2C_Write(reg);
    I2C_Stop();

    I2C_Start();
    nack = I2C_Write(BME_ADDR | 0x01);
    if(nack){ I2C_Stop(); return 1; }
    for(unsigned char i = 0; i < len; i++)
        buf[i] = I2C_Read(i == len - 1);
    I2C_Stop();
    return 0;
}

/* Lee los coeficientes de calibración del BME280 */
unsigned char BME_ReadCalib(){
    unsigned char b[26];
    if(BME_ReadRegs(0x88, b, 26)) return 1;

    dig_T1 = (unsigned int)(b[1]<<8) | b[0];
    dig_T2 = (int)((b[3]<<8) | b[2]);
    dig_T3 = (int)((b[5]<<8) | b[4]);

    unsigned char h1;
    if(BME_ReadRegs(0xA1, &h1, 1)) return 1;
    dig_H1 = h1;

    unsigned char bh[7];
    if(BME_ReadRegs(0xE1, bh, 7)) return 1;
    dig_H2 = (int)((bh[1]<<8) | bh[0]);
    dig_H3 = bh[2];
    dig_H4 = (int)((bh[3]<<4) | (bh[4] & 0x0F));
    dig_H5 = (int)((bh[5]<<4) | (bh[4]>>4));
    dig_H6 = (int)(signed char)bh[6];
    return 0;
}

unsigned char BME_Init(){
    /* Verifica que el chip ID sea 0x60 (BME280) */
    unsigned char id;
    if(BME_ReadRegs(0xD0, &id, 1)) return 1;
    if(id != 0x60) return 1;

    BME_ReadCalib();

    /* Humidity oversampling x1 */
    BME_WriteReg(0xF2, 0x01);
    /* Temp x1, Hum x1, modo normal */
    BME_WriteReg(0xF4, 0x27);
    /* Standby 1000ms, filtro off */
    BME_WriteReg(0xF5, 0xA0);
    return 0;
}

/*
 * Obtiene temperatura (décimas de °C) y humedad (%) del BME280.
 * Retorna 1 si falla.
 */
unsigned char BME_GetData(int *temp_dc, unsigned int *hum){
    unsigned char d[8];
    if(BME_ReadRegs(0xF7, d, 8)) return 1;

    /* Temperatura (compensación oficial Bosch, integer) */
    long adc_T = ((long)d[3]<<12) | ((long)d[4]<<4) | (d[5]>>4);
    long var1 = (((adc_T>>3)-((long)dig_T1<<1)) * (long)dig_T2) >> 11;
    long var2 = (((adc_T>>4)-(long)dig_T1)*((adc_T>>4)-(long)dig_T1)>>12);
    var2 = (var2 * (long)dig_T3) >> 14;
    long t_fine = var1 + var2;
    *temp_dc = (int)((t_fine * 5 + 128) >> 8); // décimas de °C

    /* Humedad (compensación oficial Bosch, integer) */
    long adc_H = ((long)d[6]<<8) | d[7];
    long hv = t_fine - 76800L;
    hv = (adc_H<<14) - ((long)dig_H4<<20) - ((long)dig_H5 * hv);
    hv = (hv + 16384L) >> 15;
    hv = hv * (((((hv * (long)dig_H6)>>10)*
                 (((hv*(long)dig_H3)>>11)+32768L))>>10)+2097152L);
    hv = hv * (long)dig_H2;
    hv = (hv>>13) + (((hv & 0x1FFF) * (long)dig_H2)>>13);
    hv = hv >> 4;
    if(hv > 102400) hv = 102400;
    if(hv < 0)      hv = 0;
    *hum = (unsigned int)(hv >> 10); // %
    return 0;
}

/* ========== ADC ========== */
void ADC_Init(void){
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
    unsigned long s = 0;
    for(unsigned char i = 0; i < ADC_MUESTRAS; i++){
        s += ADC_Read(c);
        __delay_ms(1);
    }
    return (unsigned int)(s / ADC_MUESTRAS);
}

/* ========== PANTALLA ? Dibuja layout fijo ========== */
/*
 * Layout (cada página = 8px, pantalla 128x64):
 *  Pag 0: "Temp LM35:"
 *  Pag 1: valor + " C"
 *  Pag 2: "Humedad:"
 *  Pag 3: valor + " %" (o "Sin BME" si no hay sensor)
 *  Pag 4: "Luz:"
 *  Pag 5: valor + " lux"
 *  Pag 6: "Aire:"
 *  Pag 7: valor + (MALO / OK)
 */
void OLED_DrawLabels(unsigned char hay_bme){
    OLED_Str(0, 0, "Temp LM35:");
    OLED_Str(2, 0, "Humedad:");
    OLED_Str(4, 0, "Luz:");
    OLED_Str(6, 0, "Aire:");
    if(!hay_bme){
        OLED_ClearLine(3);
        OLED_Str(3, 0, "Sin BME280");
    }
}

void OLED_UpdateValues(unsigned int temp_c, int bme_temp, unsigned int hum,
                       unsigned int luz, unsigned int aire, unsigned char hay_bme){
    /* Línea 1 ? temperatura LM35 */
    OLED_ClearLine(1);
    OLED_Num(1, 0, temp_c);
    OLED_Str(1, (temp_c >= 100 ? 18 : temp_c >= 10 ? 12 : 6), " oC");

    /* Línea 3 ? humedad BME */
    OLED_ClearLine(3);
    if(hay_bme){
        /* bme_temp está en décimas de °C */
        unsigned int td = (unsigned int)(bme_temp / 10);
        OLED_Num(3, 0, td);
        OLED_Str(3, (td >= 100 ? 18 : td >= 10 ? 12 : 6), "oC Hum:");
        OLED_Num(3, 60, hum);
        OLED_Str(3, (hum >= 100 ? 78 : hum >= 10 ? 72 : 66), "%");
    } else {
        OLED_Str(3, 0, "Sin BME280");
    }

    /* Línea 5 ? luz */
    OLED_ClearLine(5);
    OLED_Num(5, 0, luz);

    /* Línea 7 ? calidad aire */
    OLED_ClearLine(7);
    OLED_Num(7, 0, aire);
    if(aire > AIRE_UMBRAL)
        OLED_Str(7, 30, "MALO");
    else
        OLED_Str(7, 30, "OK");
}

/* ========== MAIN ========== */
void main(){
    unsigned int t_adc, l_adc, a_adc;
    unsigned int temp_c;
    int          bme_temp = 0;
    unsigned int bme_hum  = 0;

    OSCCON = 0x72;  // 8 MHz interno

    LED_TEMP_DIR = 0;
    LED_LUZ_DIR  = 0;
    LED_AIRE_DIR = 0;
    LED_TEMP = 0; LED_LUZ = 0; LED_AIRE = 0;

    ADC_Init();

    /* La OLED siempre arranca primero e independiente del BME */
    OLED_Init();

    /* Intenta iniciar el BME280. Si no está, bme_ok queda en 0 */
    bme_ok = (BME_Init() == 0) ? 1 : 0;

    OLED_DrawLabels(bme_ok);

    while(1){
        /* --- Sensores analógicos --- */
        t_adc = ADC_ReadAvg(CANAL_LM35);
        l_adc = ADC_ReadAvg(CANAL_LDR);
        a_adc = ADC_ReadAvg(CANAL_MQ135);

        temp_c = (unsigned int)((unsigned long)t_adc * 500 / 1024);

        /* --- BME280 (solo si respondió al inicio) --- */
        if(bme_ok){
            if(BME_GetData(&bme_temp, &bme_hum) != 0){
                /*
                 * Si falla en runtime (se desconectó), marcamos como perdido
                 * pero NO colgamos el programa. La pantalla sigue funcionando.
                 */
                bme_ok = 0;
                OLED_DrawLabels(0);
            }
        }

        /* --- LEDs --- */
        LED_TEMP = (t_adc >= TEMP_MIN_ADC && t_adc <= TEMP_MAX_ADC);
        LED_LUZ  = (l_adc > LUZ_UMBRAL);
        LED_AIRE = (a_adc > AIRE_UMBRAL);

        /* --- Actualiza pantalla --- */
        OLED_UpdateValues(temp_c, bme_temp, bme_hum, l_adc, a_adc, bme_ok);

        __delay_ms(500);
    }
}