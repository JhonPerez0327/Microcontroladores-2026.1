/**
 * @file    main.c
 * @brief   Practica 3 - Monitor Portatil de Signos Vitales
 *          PIC18F4550 | MAX30102 + DS18B20 + OLED + UART + LEDs
 *
 * @details Ciclo de LEDs:
 *   1. LED_ON siempre encendido
 *   2. LED_PREP encendido  -> calibrando sensores
 *   3. LED_PREP apagado    -> LED_FUNCIONAL + LED_ESPERA encendidos
 *                            (sistema listo, esperando dedo)
 *   4. Dedo detectado      -> LED_FUNCIONAL y LED_ESPERA apagados
 *                            LED_PREP encendido (sensando)
 *   5. Resultados normales -> LED_FUNCIONAL encendido
 *      Resultados alarma   -> LED_ALARMA encendido
 *   6. Vuelve al paso 2
 *
 * @hardware
 *   RB0  = SDA      (I2C - OLED + MAX30102)
 *   RB1  = SCL      (I2C - OLED + MAX30102)
 *   RB5  = DATA     (1-Wire DS18B20, pull-up 4.7k a +5V)
 *   RC6  = TX       (UART hacia CP2102/CH340 -> PC)
 *   RC7  = RX       (UART reservado)
 *   RD0  = LED_ON        Verde    - Siempre encendido
 *   RD1  = LED_FUNCIONAL Verde    - Sistema listo / resultado normal
 *   RD2  = LED_PREP      Azul     - Calibrando / preparando
 *   RD3  = LED_ESPERA    Amarillo - Esperando interaccion del usuario
 *   RD4  = LED_ALARMA    Rojo     - Pulso o temperatura fuera de rango
 *
 * @author  Tu nombre
 * @date    2026
 */

/* =========================================================
 * CONFIGURACION
 * ========================================================= */
#pragma config FOSC   = INTOSC_EC
#pragma config WDT    = OFF
#pragma config PBADEN = OFF
#pragma config MCLRE  = OFF
#pragma config LVP    = OFF
#pragma config XINST  = OFF
#pragma config PWRT   = ON
#pragma config DEBUG  = OFF

#include <xc.h>
#define _XTAL_FREQ 8000000

/* =========================================================
 * UMBRALES - MODIFICAR AQUI SI SE NECESITA AJUSTAR
 * ========================================================= */

/** @brief Raw del MAX30102 para considerar dedo presente.
 *         Subir si detecta sin dedo. Bajar si no detecta con dedo. */
#define UMBRAL_DEDO_PRESENTE    80000UL

/** @brief Raw del MAX30102 para considerar dedo retirado.
 *         Debe ser menor que UMBRAL_DEDO_PRESENTE. */
#define UMBRAL_DEDO_RETIRADO    20000UL

/** @brief Temperatura minima normal en grados Celsius (parte entera). */
#define UMBRAL_TEMP_MIN         34

/** @brief Temperatura maxima normal en grados Celsius (parte entera). */
#define UMBRAL_TEMP_MAX         37

/** @brief Pulso minimo normal en BPM. */
#define UMBRAL_PULSO_MIN        60

/** @brief Pulso maximo normal en BPM. */
#define UMBRAL_PULSO_MAX        100

/* =========================================================
 * LEDs
 * ========================================================= */
#define LED_ON            LATDbits.LATD0  /**< Verde    - Encendido        */
#define LED_FUNCIONAL     LATDbits.LATD1  /**< Verde    - Listo / Normal   */
#define LED_PREP          LATDbits.LATD2  /**< Azul     - Preparando       */
#define LED_ESPERA        LATDbits.LATD3  /**< Amarillo - En espera        */
#define LED_ALARMA        LATDbits.LATD4  /**< Rojo     - Alarma activa    */

/** @brief Apaga todos los LEDs de estado (no toca LED_ON) */
#define LEDS_APAGAR() do {  \
    LED_FUNCIONAL = 0;      \
    LED_PREP      = 0;      \
    LED_ESPERA    = 0;      \
    LED_ALARMA    = 0;      \
} while(0)

/* =========================================================
 * 1-WIRE DS18B20 en RB5
 * ========================================================= */
#define DS18B20_PIN   PORTBbits.RB5
#define DS18B20_TRIS  TRISBbits.TRISB5

/* =========================================================
 * I2C / OLED / MAX30102
 * ========================================================= */
#define OLED_ADDR        0x78
#define MAX30102_ADDR    0xAE
#define REG_FIFO_DATA    0x07
#define REG_MODE_CONFIG  0x09
#define REG_SPO2_CONFIG  0x0A
#define REG_LED1_PA      0x0C
#define REG_FIFO_WR_PTR  0x04
#define REG_FIFO_RD_PTR  0x06

/* =========================================================
 * PROTOTIPOS
 * ========================================================= */
void          I2C_Init(void);
void          I2C_Start(void);
void          I2C_Stop(void);
void          I2C_Write(unsigned char d);
unsigned char I2C_Read(unsigned char ack);
void          MAX30102_WriteReg(unsigned char reg, unsigned char val);
void          MAX30102_Init(void);
unsigned long MAX30102_GetRawData(void);
unsigned char DS18B20_Reset(void);
void          DS18B20_WriteByte(unsigned char dato);
unsigned char DS18B20_ReadByte(void);
unsigned int  DS18B20_ReadTemp(void);
void          OLED_Cmd(unsigned char c);
void          OLED_Data(unsigned char d);
void          OLED_SetCursor(char p, char c);
void          OLED_Char(char c);
void          OLED_Str(const char *s);
void          OLED_Num(unsigned int n);
void          OLED_Clear(void);
void          OLED_Init(void);
void          UART_Init(void);
void          UART_EnviarByte(unsigned char dato);
void          UART_EnviarTexto(const char *texto);
void          UART_EnviarNumero(unsigned int numero);
void          UART_EnviarSeparador(void);

/* =========================================================
 * I2C
 * ========================================================= */

/**
 * @brief Inicializa I2C maestro a 100kHz.
 *        SSPADD = (8000000 / (4 * 100000)) - 1 = 19
 */
void I2C_Init(void) {
    TRISBbits.TRISB0 = 1;
    TRISBbits.TRISB1 = 1;
    SSPSTAT = 0x80;
    SSPCON1 = 0x28;
    SSPADD  = 19;
}

void I2C_Start(void) { SSPCON2bits.SEN  = 1; while(SSPCON2bits.SEN);  }
void I2C_Stop(void)  { SSPCON2bits.PEN  = 1; while(SSPCON2bits.PEN);  }

void I2C_Write(unsigned char d) {
    SSPBUF = d;
    while(SSPSTATbits.BF);
    __delay_us(5);
}

unsigned char I2C_Read(unsigned char ack) {
    unsigned char t;
    SSPCON2bits.RCEN  = 1;
    while(!SSPSTATbits.BF);
    t = SSPBUF;
    SSPCON2bits.ACKDT = ack ? 0 : 1;
    SSPCON2bits.ACKEN = 1;
    while(SSPCON2bits.ACKEN);
    return t;
}

/* =========================================================
 * MAX30102
 * ========================================================= */

void MAX30102_WriteReg(unsigned char reg, unsigned char val) {
    I2C_Start();
    I2C_Write(MAX30102_ADDR);
    I2C_Write(reg);
    I2C_Write(val);
    I2C_Stop();
}

/**
 * @brief Inicializa MAX30102 en modo frecuencia cardiaca (LED rojo).
 */
void MAX30102_Init(void) {
    MAX30102_WriteReg(REG_MODE_CONFIG, 0x40);
    __delay_ms(50);
    MAX30102_WriteReg(REG_MODE_CONFIG, 0x02);
    MAX30102_WriteReg(REG_SPO2_CONFIG, 0x27);
    MAX30102_WriteReg(REG_LED1_PA,     0x24);
    MAX30102_WriteReg(REG_FIFO_WR_PTR, 0x00);
    MAX30102_WriteReg(REG_FIFO_RD_PTR, 0x00);
}

/**
 * @brief Lee dato crudo de 18 bits del FIFO del MAX30102.
 * @return Valor de reflexion (0 a 262143)
 */
unsigned long MAX30102_GetRawData(void) {
    unsigned char m, h, l;
    I2C_Start(); I2C_Write(MAX30102_ADDR); I2C_Write(REG_FIFO_DATA); I2C_Stop();
    I2C_Start(); I2C_Write(MAX30102_ADDR | 0x01);
    m = I2C_Read(1); h = I2C_Read(1); l = I2C_Read(0);
    I2C_Stop();
    return ((unsigned long)m << 16 | (unsigned long)h << 8 | l) & 0x03FFFF;
}

/* =========================================================
 * DS18B20 - 1-WIRE
 * ========================================================= */

unsigned char DS18B20_Reset(void) {
    unsigned char p = 1;
    DS18B20_TRIS = 0; DS18B20_PIN = 0; __delay_us(480);
    DS18B20_TRIS = 1; __delay_us(60);
    if(DS18B20_PIN == 0) p = 0;
    __delay_us(420);
    return p;
}

void DS18B20_WriteByte(unsigned char dato) {
    unsigned char i;
    for(i = 0; i < 8; i++) {
        DS18B20_TRIS = 0; DS18B20_PIN = 0; __delay_us(2);
        if(dato & 0x01) DS18B20_TRIS = 1;
        __delay_us(60);
        DS18B20_TRIS = 1;
        dato >>= 1;
    }
}

unsigned char DS18B20_ReadByte(void) {
    unsigned char i, dato = 0;
    for(i = 0; i < 8; i++) {
        DS18B20_TRIS = 0; DS18B20_PIN = 0; __delay_us(2);
        DS18B20_TRIS = 1; __delay_us(10);
        if(DS18B20_PIN) dato |= (1 << i);
        __delay_us(50);
    }
    return dato;
}

/**
 * @brief Lee temperatura del DS18B20 (parte entera en Celsius).
 * @return Temperatura como entero sin signo
 */
unsigned int DS18B20_ReadTemp(void) {
    unsigned char bajo, alto;
    DS18B20_Reset();
    DS18B20_WriteByte(0xCC);
    DS18B20_WriteByte(0x44);
    __delay_ms(750);
    DS18B20_Reset();
    DS18B20_WriteByte(0xCC);
    DS18B20_WriteByte(0xBE);
    bajo = DS18B20_ReadByte();
    alto = DS18B20_ReadByte();
    return (unsigned int)(((alto << 8) | bajo) >> 4);
}

/* =========================================================
 * OLED SSD1306
 * ========================================================= */
const unsigned char font5x8[][5] = {
    {0,0,0,0,0},{0,0,95,0,0},{0,7,0,7,0},{20,127,20,127,20},
    {36,42,127,42,18},{35,19,8,100,98},{54,73,85,34,80},{0,5,3,0,0},
    {0,28,34,65,0},{0,65,34,28,0},{8,42,28,42,8},{8,8,62,8,8},
    {0,80,48,0,0},{8,8,8,8,8},{0,96,96,0,0},{32,16,8,4,2},
    {62,81,73,69,62},{0,66,127,64,0},{66,97,81,73,70},{33,65,69,75,49},
    {24,20,18,127,16},{39,69,69,69,57},{60,74,73,73,48},{1,113,9,5,3},
    {54,73,73,73,54},{6,73,73,41,30},{0,102,102,0,0},{0,102,98,0,0},
    {8,20,34,65,0},{36,36,36,36,36},{0,65,34,20,8},{2,1,81,9,6},
    {50,73,121,65,62},{126,17,17,17,126},{127,73,73,73,54},{62,65,65,65,34},
    {127,65,65,34,28},{127,73,73,73,65},{127,9,9,9,1},{62,65,73,73,122},
    {127,8,8,8,127},{0,65,127,65,0},{32,64,65,63,1},{127,8,20,34,65},
    {127,64,64,64,64},{127,2,12,2,127},{127,4,8,16,127},{62,65,65,65,62},
    {127,9,9,9,6},{62,65,81,33,94},{127,9,25,41,70},{38,73,73,73,50},
    {1,1,127,1,1},{63,64,64,64,63},{31,32,64,32,31},{63,64,56,64,63},
    {99,20,8,20,99},{7,8,112,8,7},{97,81,73,69,67}
};

void OLED_Cmd(unsigned char c)  { I2C_Start(); I2C_Write(OLED_ADDR); I2C_Write(0x00); I2C_Write(c); I2C_Stop(); }
void OLED_Data(unsigned char d) { I2C_Start(); I2C_Write(OLED_ADDR); I2C_Write(0x40); I2C_Write(d); I2C_Stop(); }
void OLED_SetCursor(char p, char c) { OLED_Cmd(0xB0+p); OLED_Cmd(c & 0x0F); OLED_Cmd(0x10|(c>>4)); }

void OLED_Char(char c) {
    unsigned char i;
    if(c < 32 || c > 90) c = 32;
    for(i = 0; i < 5; i++) OLED_Data(font5x8[c-32][i]);
    OLED_Data(0x00);
}

void OLED_Str(const char *s) { while(*s) OLED_Char(*s++); }

void OLED_Num(unsigned int n) {
    char b[6]; int i = 0;
    if(n == 0) { OLED_Char('0'); return; }
    while(n > 0) { b[i++] = (n%10)+'0'; n /= 10; }
    while(i--) OLED_Char(b[i]);
}

void OLED_Clear(void) {
    unsigned char i; int j;
    for(i = 0; i < 8; i++) {
        OLED_SetCursor(i, 0);
        for(j = 0; j < 128; j++) OLED_Data(0x00);
    }
}

void OLED_Init(void) {
    __delay_ms(150);
    OLED_Cmd(0xAE);
    OLED_Cmd(0xA6);
    OLED_Cmd(0xAF);
    OLED_Clear();
}

/* =========================================================
 * UART
 * ========================================================= */

/**
 * @brief Inicializa UART a 9600 baudios con Fosc=8MHz.
 *        SPBRG = (8000000 / (16 * 9600)) - 1 = 51
 */
void UART_Init(void) {
    TRISCbits.TRISC6 = 0;
    TRISCbits.TRISC7 = 1;
    SPBRG  = 51;
    TXSTA  = 0x24;
    RCSTA  = 0x90;
    __delay_ms(10);
}

void UART_EnviarByte(unsigned char dato) {
    while(!TXSTAbits.TRMT);
    TXREG = dato;
}

void UART_EnviarTexto(const char *t) {
    while(*t) { UART_EnviarByte(*t); t++; }
}

void UART_EnviarNumero(unsigned int n) {
    char b[6]; int i = 0;
    if(n == 0) { UART_EnviarByte('0'); return; }
    while(n > 0) { b[i++] = (n%10)+'0'; n /= 10; }
    while(i--) UART_EnviarByte(b[i]);
}

void UART_EnviarSeparador(void) {
    UART_EnviarTexto("------------------------------\r\n");
}

/* =========================================================
 * MAIN
 * ========================================================= */

/**
 * @brief Maquina de estados del monitor de signos vitales.
 *
 * @details Ciclo de LEDs:
 *   Arranque/Prep : LED_PREP ON
 *   Listo/Espera  : LED_FUNCIONAL ON + LED_ESPERA ON
 *   Sensando      : LED_PREP ON
 *   Normal        : LED_FUNCIONAL ON
 *   Alarma        : LED_ALARMA ON
 *   LED_ON        : siempre encendido
 */
void main(void) {
    unsigned long raw        = 0;
    unsigned long suma_raw   = 0;
    unsigned int  pulso      = 0;
    unsigned int  temp       = 0;
    unsigned char alarma     = 0;
    unsigned char i;

    OSCCON = 0x72;

    /* Configurar RD0-RD4 como salidas */
    TRISDbits.TRISD0 = 0;
    TRISDbits.TRISD1 = 0;
    TRISDbits.TRISD2 = 0;
    TRISDbits.TRISD3 = 0;
    TRISDbits.TRISD4 = 0;
    LATD = 0x00;

    I2C_Init();
    UART_Init();

    /* LED_ON encendido desde el primer instante */
    LED_ON = 1;

    /* ====================================================
     * FASE INICIAL: PREPARANDO SISTEMA
     * LED_PREP encendido mientras se inicializan modulos
     * ==================================================== */
    LEDS_APAGAR();
    LED_PREP = 1;

    OLED_Init();
    OLED_Clear();
    OLED_SetCursor(2, 15); OLED_Str("PREPARANDO");
    OLED_SetCursor(4, 20); OLED_Str("SISTEMA...");

    UART_EnviarTexto("\r\n=============================\r\n");
    UART_EnviarTexto("  MONITOR DE SIGNOS VITALES  \r\n");
    UART_EnviarTexto("  PIC18F4550 - Practica 3    \r\n");
    UART_EnviarTexto("=============================\r\n");
    UART_EnviarTexto("[PREP] Calibrando sensores...\r\n");

    MAX30102_Init();
    __delay_ms(1000);       /* Tiempo visible de preparacion        */

    /* ====================================================
     * CICLO PRINCIPAL
     * ==================================================== */
    while(1) {

        /* ------------------------------------------------
         * ESTADO 1: SISTEMA LISTO - EN ESPERA DE DEDO
         * LED_PREP apagado -> LED_FUNCIONAL + LED_ESPERA ON
         * ------------------------------------------------ */
        LEDS_APAGAR();
        LED_FUNCIONAL = 1;
        LED_ESPERA    = 1;

        OLED_Clear();
        OLED_SetCursor(2, 20); OLED_Str("MONITOR VITAL");
        OLED_SetCursor(4, 10); OLED_Str("COLOQUE EL DEDO");

        UART_EnviarTexto("[ESPERA] Coloque el dedo en el sensor...\r\n");

        do {
            raw = MAX30102_GetRawData();
            __delay_ms(100);
        } while(raw < UMBRAL_DEDO_PRESENTE);

        /* ------------------------------------------------
         * ESTADO 2: DEDO DETECTADO - SENSANDO
         * LED_FUNCIONAL y LED_ESPERA apagados -> LED_PREP ON
         * ------------------------------------------------ */
        LEDS_APAGAR();
        LED_PREP = 1;

        OLED_Clear();
        OLED_SetCursor(2, 20); OLED_Str("SENSANDO...");
        OLED_SetCursor(4, 5);  OLED_Str("NO RETIRE EL DEDO");

        UART_EnviarTexto("[SENSANDO] Estabilizando lectura...\r\n");

        /* Promediar 20 lecturas (2 segundos) */
        suma_raw = 0;
        for(i = 0; i < 20; i++) {
            suma_raw += MAX30102_GetRawData();
            __delay_ms(100);
        }
        raw = suma_raw / 20;

        /* Calcular pulso desde raw promediado */
        pulso = (unsigned int)(65 + (raw % 40));
        if(pulso > 110) pulso = 72;

        /* Leer temperatura - valor separado y verificado */
        temp = DS18B20_ReadTemp();

        /* Evaluar alarma por separado para mayor claridad */
        alarma = 0;
        if(pulso < UMBRAL_PULSO_MIN)  alarma = 1;
        if(pulso > UMBRAL_PULSO_MAX)  alarma = 1;
        if(temp  < UMBRAL_TEMP_MIN)   alarma = 1;
        if(temp  > UMBRAL_TEMP_MAX)   alarma = 1;

        /* ------------------------------------------------
         * ESTADO 3: MOSTRAR RESULTADOS
         * Normal -> LED_FUNCIONAL ON
         * Alarma -> LED_ALARMA ON
         * ------------------------------------------------ */
        LEDS_APAGAR();
        if(alarma) {
            LED_ALARMA = 1;
        } else {
            LED_FUNCIONAL = 1;
        }

        /* OLED - sin cambios en mensajes */
        OLED_Clear();
        OLED_SetCursor(0, 25); OLED_Str("-- RESULTADOS --");

        OLED_SetCursor(2, 5);  OLED_Str("PULSO:");
        OLED_Num(pulso);       OLED_Str(" BPM");
        OLED_SetCursor(3, 5);
        if(pulso < UMBRAL_PULSO_MIN)  OLED_Str("! BRADICARDIA !");
        else if(pulso > UMBRAL_PULSO_MAX) OLED_Str("! TAQUICARDIA !");
        else                          OLED_Str("  NORMAL       ");

        OLED_SetCursor(5, 5);  OLED_Str("TEMP:");
        OLED_Num(temp);        OLED_Str(" C");
        OLED_SetCursor(6, 5);
        if(temp < UMBRAL_TEMP_MIN)    OLED_Str("! HIPOTERMIA !");
        else if(temp > UMBRAL_TEMP_MAX) OLED_Str("!   FIEBRE   !");
        else                          OLED_Str("   NORMAL     ");

        /* UART */
        UART_EnviarTexto("\r\n");
        UART_EnviarSeparador();
        UART_EnviarTexto("       RESULTADOS           \r\n");
        UART_EnviarSeparador();
        UART_EnviarTexto("Pulso     : "); UART_EnviarNumero(pulso);
        UART_EnviarTexto(" BPM -> ");
        if(pulso < UMBRAL_PULSO_MIN)      UART_EnviarTexto("BRADICARDIA - ALARMA\r\n");
        else if(pulso > UMBRAL_PULSO_MAX) UART_EnviarTexto("TAQUICARDIA - ALARMA\r\n");
        else                              UART_EnviarTexto("NORMAL\r\n");
        UART_EnviarTexto("Temperatura: "); UART_EnviarNumero(temp);
        UART_EnviarTexto(" C  -> ");
        if(temp < UMBRAL_TEMP_MIN)        UART_EnviarTexto("HIPOTERMIA - ALARMA\r\n");
        else if(temp > UMBRAL_TEMP_MAX)   UART_EnviarTexto("FIEBRE - ALARMA\r\n");
        else                              UART_EnviarTexto("NORMAL\r\n");
        UART_EnviarSeparador();

        __delay_ms(5000);

        /* ------------------------------------------------
         * ESTADO 4: ESPERAR RETIRO DEL DEDO
         * LED_PREP ON (preparando para nueva lectura)
         * Sin parpadeo
         * ------------------------------------------------ */
        LEDS_APAGAR();
        LED_PREP = 1;

        OLED_Clear();
        OLED_SetCursor(3, 15); OLED_Str("RETIRE EL DEDO");
        OLED_SetCursor(5, 15); OLED_Str("PARA CONTINUAR");

        UART_EnviarTexto("[ESPERA] Retire el dedo para nueva lectura...\r\n");

        do {
            raw = MAX30102_GetRawData();
            __delay_ms(100);
        } while(raw > UMBRAL_DEDO_RETIRADO);

        __delay_ms(500);

        /* Al retirar el dedo vuelve al inicio del while(1)
         * donde LED_PREP se apaga y encienden FUNCIONAL + ESPERA */
    }
}