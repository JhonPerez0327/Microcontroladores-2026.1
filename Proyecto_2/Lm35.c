#include <xc.h>
#include <stdio.h>
#pragma config FOSC = INTOSC_EC  
#pragma config WDT = OFF         
#pragma config PBADEN = OFF      
#pragma config LVP = OFF

#define _XTAL_FREQ 8000000
#define OLED_ADDR   0x78
//funciones a usar
void ADC_Init(void);
unsigned int ADC_Read(void);
//Fuente oled ACII 32-90
const unsigned char font5x8[][5] = {
    {0,0,0,0,0},{0,0,95,0,0},{0,7,0,7,0},{20,127,20,127,20},{36,42,127,42,18},{35,19,8,100,98},{54,73,85,34,80},{0,5,3,0,0},{0,28,34,65,0},{0,65,34,28,0},{8,42,28,42,8},{8,8,62,8,8},{0,80,48,0,0},{8,8,8,8,8},{0,96,96,0,0},{32,16,8,4,2},{62,81,73,69,62},{0,66,127,64,0},{66,97,81,73,70},{33,65,69,75,49},{24,20,18,127,16},{39,69,69,69,57},{60,74,73,73,48},{1,113,9,5,3},{54,73,73,73,54},{6,73,73,41,30},{0,102,102,0,0},{0,102,98,0,0},{8,20,34,65,0},{36,36,36,36,36},{0,65,34,20,8},{2,1,81,9,6},{50,73,121,65,62},{126,17,17,17,126},{127,73,73,73,54},{62,65,65,65,34},{127,65,65,34,28},{127,73,73,73,65},{127,9,9,9,1},{62,65,73,73,122},{127,8,8,8,127},{0,65,127,65,0},{32,64,65,63,1},{127,8,20,34,65},{127,64,64,64,64},{127,2,12,2,127},{127,4,8,16,127},{62,65,65,65,62},{127,9,9,9,6},{62,65,81,33,94},{127,9,25,41,70},{38,73,73,73,50},{1,1,127,1,1},{63,64,64,64,63},{31,32,64,32,31},{63,64,56,64,63},{99,20,8,20,99},{7,8,112,8,7},{97,81,73,69,67}
};
//Funciones I2c
void I2C_Init(void) {
    TRISBbits.TRISB0 = 1; TRISBbits.TRISB1 = 1; // Pines SDA y SCL
    SSPSTAT = 0x80; SSPCON1 = 0x28; SSPADD = 19; // 100kHz a 8MHz
}
void I2C_Start(void) { SSPCON2bits.SEN = 1; while(SSPCON2bits.SEN); }
void I2C_Stop(void)  { SSPCON2bits.PEN = 1; while(SSPCON2bits.PEN); }
void I2C_Write(unsigned char d) { SSPBUF = d; while(SSPSTATbits.BF); __delay_us(5); }

/* ============== FUNCIONES OLED ============== */
void OLED_Cmd(unsigned char c) { I2C_Start(); I2C_Write(OLED_ADDR); I2C_Write(0x00); I2C_Write(c); I2C_Stop(); }
void OLED_Data(unsigned char d) { I2C_Start(); I2C_Write(OLED_ADDR); I2C_Write(0x40); I2C_Write(d); I2C_Stop(); }
void OLED_SetCursor(char p, char c) { OLED_Cmd(0xB0+p); OLED_Cmd(c & 0x0F); OLED_Cmd(0x10 | (c>>4)); }

void OLED_Char(char c) {
    if(c < 32 || c > 90) c = 32;
    for(int i=0; i<5; i++) OLED_Data(font5x8[c-32][i]);
    OLED_Data(0x00);
}

void OLED_Str(const char *s) { while(*s) OLED_Char(*s++); }

void OLED_Num(unsigned int n) {
    char b[6]; int i=0;
    if(n==0){ OLED_Char('0'); return; }
    while(n>0){ b[i++]=(n%10)+'0'; n/=10; }
    while(i--) OLED_Char(b[i]);
}

void OLED_Init(void) {
    __delay_ms(150);
    OLED_Cmd(0xAE); OLED_Cmd(0xA6); OLED_Cmd(0xAF);
    for(int i=0; i<8; i++){ OLED_SetCursor(i,0); for(int j=0; j<128; j++) OLED_Data(0x00); }
}
//Funcion AN0
void ADC_Init(void) {
    ADCON1 = 0x0E;  //voltaje de referencia 0-5v solo AN0 como analogico
    ADCON2 = 0xBD;  // justificacion derecha,20 TAD espera mas larga para estabilidad
}
unsigned int ADC_Read(void) {
    ADCON0 = 0x01;       // Selecciona AN0 y enciende el ADC
    __delay_us(20);      // Tiempo de seguridad
    ADCON0bits.GO = 1;   // Inicia conversion  
    while(ADCON0bits.GO); // Espera a que termine
    return ((ADRESH << 8) + ADRESL);
}
//Principal
    void main(void) {
    OSCCON = 0x72;      // 8MHz
    TRISA = 0x01;       // RA0 como entrada (LM35)
    TRISD = 0x00;       // Puerto D como salida (LED en RD0)
    LATD = 0x00;        //Iniciar en 0 puerto d 
    
    I2C_Init();
    ADC_Init();     //Iniciar conversor 
    OLED_Init();
    
    unsigned int adc_res;
    unsigned long temp; // Usamos long para evitar que la opracion falle

    while(1) {
        adc_res = ADC_Read();  // Lee canal AN0 convierte valor de voltaje a valor de 10 bits
        /* * Conversion de voltaje a temperatura 
         *  5V / 1023 pasos = 4.88mV por paso
         * Temperatura: (Valor ADC * 5V / 1023) / 10mV lm35 10mv=1°C
         * Simplificado: (Valor ADC * 500) / 1023
         */
        temp = (unsigned int)((unsigned long)adc_res * 500 / 1023);

        //Si la temperatura es mayor a 33°C encender led
        if (temp > 27) {
            LATDbits.LATD0 = 1; 
        } else {
            LATDbits.LATD0 = 0;
        }
        // Mostrar en OLED
        OLED_SetCursor(2, 20); // Fila 2, centro aproximado
        OLED_Str("TEMP: ");
        OLED_Num(temp);
        OLED_Str(" C  "); // Espacios al final para borrar números viejos
        OLED_SetCursor(4, 20); // Fila 4
        if(temp > 33) OLED_Str("ESTADO: ALTA");
        else          OLED_Str("ESTADO: NORMAL");
        __delay_ms(200); 
    }
}  

