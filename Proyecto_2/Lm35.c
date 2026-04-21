#include <xc.h>
#pragma config FOSC = INTOSC_EC  // Oscilador interno
#pragma config WDT = OFF         // Desactivo verificador de ciclos
#pragma config PBADEN = OFF      // Pines del Puerto B como digitales al reset

#define _XTAL_FREQ 8000000
//funciones a usar
void ADC_Init(void);
unsigned int ADC_Read(unsigned char canal);

void main(void) {
    OSCCON = 0x72;     //Oscilador a 8MHz
    TRISA = 0x01;   // RA0 como entrada para Lm35 
    TRISD = 0x00;   // Puerto D como salida
    LATD = 0x00;    // Inicia todo en cero led apagado
    
    ADC_Init();     // Inicializar el conversor
    
    unsigned int adc_value;
    float temperatura;
//ciclo para medir indefinidamete
    while(1) {
        adc_value = ADC_Read(0); // Lee canal AN0 convierte valor de voltaje a valor de 10 bits
        
        /* * Conversion de voltaje a temperatura 
         *  5V / 1023 pasos = 4.88mV por paso
         * Temperatura: (Valor ADC * 5V / 1023) / 10mV lm35 10mv=1°C
         * Simplificado: (Valor ADC * 500) / 1023
         */
        temperatura = (adc_value * 500.0) / 1023.0;

        //Si la temperatura es mayor a 30°C encender led
        if (temperatura > 30.0) {
            LATDbits.LATD0 = 1; 
        } else {
            LATDbits.LATD0 = 0;
        }
        __delay_ms(200); // pausa para estabilidad
    }
}
// Funcion del ADC
void ADC_Init(void) {
    ADCON1 = 0x0D;  //0x0D=00001101 voltaje de referencia 0-5v  AN0 y AN1 como analogicos
    ADCON2 = 0x95;  // 0x95 justificacion derecha, 8 TAD de tiempo de adquisision 1/(Fosc/16)
}
// funcion lectura de canal
unsigned int ADC_Read(unsigned char canal) {
    //canal en ADCON0 bits 5-2 y encender módulo ADON
    ADCON0 = (unsigned char)((canal << 2) | 0x01);
    
    __delay_us(20);         //Espera tiempo de adquisiocion capacitor se cargue
    ADCON0bits.GO_DONE = 1; // Iniciar conversion
    
    while(ADCON0bits.GO_DONE); // Esperar a que termine
    
    return ((unsigned int)((ADRESH << 8) + ADRESL)); // unir h y l valor de 10 bits
}