#include <xc.h>

// --- CONFIGURACIÓN DE BITS (PIC18F4550) ---
#pragma config FOSC  = INTOSCIO_EC
#pragma config WDT   = OFF
#pragma config LVP   = OFF
#pragma config PBADEN = OFF
#pragma config MCLRE = OFF
#pragma config PWRT  = ON

#define _XTAL_FREQ 8000000

// --- DEFINICIÓN DE PINES LCD (MODO 8 BITS) ---
#define RS  LATEbits.LATE0   // RE0
#define RW  LATEbits.LATE1   // RE1
#define E   LATEbits.LATE2   // RE2
#define DATA_LCD LATD        // Datos en PORTD completo

// --- MAPA DEL TECLADO (PORTB) ---
const char teclas[4][4] = {
    {'1','2','3','A'},
    {'4','5','6','B'},
    {'7','8','9','C'},
    {'*','0','#','D'}
};

// ==========================================
//          FUNCIONES DEL LCD
// ==========================================

void LCD_Enable() {
    E = 1;
    __delay_ms(1);
    E = 0;
    __delay_ms(1);
}

void LCD_Comando(unsigned char cmd) {
    RS = 0;
    RW = 0;
    DATA_LCD = cmd;
    LCD_Enable();
    __delay_ms(2);
}

void LCD_Dato(unsigned char dato) {
    RS = 1;
    RW = 0;
    DATA_LCD = dato;
    LCD_Enable();
    __delay_ms(1);
}

void LCD_Init() {
    __delay_ms(20);
    LCD_Comando(0x38); // 8 bits, 2 lineas, 5x8
    LCD_Comando(0x0C); // Display ON, Cursor OFF
    LCD_Comando(0x06); // Incremento de cursor
    LCD_Comando(0x01); // Limpiar
    __delay_ms(2);
}

void LCD_SetCursor(unsigned char fila, unsigned char col) {
    unsigned char dir = (fila == 0) ? (0x80 + col) : (0xC0 + col);
    LCD_Comando(dir);
}

void LCD_String(const char *texto) {
    while (*texto) { // Corregido: puntero al caracter
        LCD_Dato(*texto);
        texto++;
    }
}

// ==========================================
//          FUNCIONES DEL TECLADO
// ==========================================

char leer_teclado(void) {
    unsigned char fila, col;
    for (fila = 0; fila < 4; fila++) {
        // Poner una fila en BAJO (0), las demás en ALTO (1)
        LATB = (unsigned char)~(1 << fila) | 0xF0; 
        __delay_us(50);

        for (col = 0; col < 4; col++) {
            // Revisar si la columna (RB4-RB7) está en BAJO
            if (!(PORTB & (1 << (col + 4)))) {
                __delay_ms(20); // Antirebote
                if (!(PORTB & (1 << (col + 4)))) {
                    while (!(PORTB & (1 << (col + 4)))); // Esperar a soltar
                    return teclas[fila][col];
                }
            }
        }
    }
    return 0;
}

// ==========================================
//          INTERRUPCIÓN Y MAIN
// ==========================================

void __interrupt() ISR(void) {
    if (INTCONbits.INT0IF) {
        // Código de interrupción si se presiona algo en RB0
        INTCONbits.INT0IF = 0;
    }
}

void main(void) {
    // Configuración de Oscilador a 8MHz
    OSCCON = 0x72; 

    // Configuración de Puertos
    TRISD = 0x00;   // PORTD: Salida (Datos LCD)
    LATD  = 0x00;
    TRISE = 0x00;   // PORTE: Salida (Control LCD)
    LATE  = 0x00;
    
    // PORTB: RB0-RB3 Salidas (Filas), RB4-RB7 Entradas (Columnas)
    TRISB = 0xF0;   
    LATB  = 0x0F;
    
    ADCON1 = 0x0F;  // Todo digital

    LCD_Init();
    
    LCD_SetCursor(0, 0);
    LCD_String("Teclado listo:");
    LCD_SetCursor(1, 0);

    char tecla_presionada;

    while (1) {
        tecla_presionada = leer_teclado();
        
        if (tecla_presionada != 0) {
            LCD_Dato(tecla_presionada); // Escribir la tecla en el LCD
        }
    }
}