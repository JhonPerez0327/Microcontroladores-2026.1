#include <xc.h>
#pragma config FOSC = INTOSC_EC  
#pragma config WDT = OFF         
#pragma config PBADEN = OFF      
#pragma config LVP = OFF

#define _XTAL_FREQ 8000000

//funciones a usar
void ADC_Init(void);
unsigned int ADC_Read(void);

void main(void) {
    OSCCON = 0x72;      // 8MHz
    TRISA = 0x01;       // RA0 como entrada (LM35)
    TRISD = 0x00;       // Puerto D como salida (LED en RD0)
    LATD = 0x00;        //Iniciar en 0 puerto d 
    
    ADC_Init();     //Iniciar conversor 
    
    unsigned int adc_res;
    unsigned long temp; // Usamos long para evitar que la opracion falle

    while(1) {
        adc_res = ADC_Read();  // Lee canal AN0 convierte valor de voltaje a valor de 10 bits
        /* * Conversion de voltaje a temperatura 
         *  5V / 1023 pasos = 4.88mV por paso
         * Temperatura: (Valor ADC * 5V / 1023) / 10mV lm35 10mv=1°C
         * Simplificado: (Valor ADC * 500) / 1023
         */
        temp = (unsigned long)adc_res * 500 / 1023;

        //Si la temperatura es mayor a 30°C encender led
        if (temp > 30) {
            LATDbits.LATD0 = 1; 
        } else {
            LATDbits.LATD0 = 0;
        }
        
        __delay_ms(200); 
    }
}

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