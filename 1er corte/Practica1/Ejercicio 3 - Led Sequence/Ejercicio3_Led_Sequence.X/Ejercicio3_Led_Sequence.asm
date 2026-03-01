; C?digo en Assembler para PIC18F4550
; Materia: Microcontroladores 2026.1 Universidad del Cauca
; Presentado por: Jhon Alexander Perez Arango
; Descripci?n: Generacion de 4 secuencias o efectos distintosusando 4 o m?s leds, 
;              este desarrollo debe contar con una entrada externa (pulsador) para 
;              el cambio de secuencia y una entrada externa que permita modificar
;              la velocidad de ejecuci?n dela secuencia.
; Frecuencia: Oscilador interno de 8 MHz 
; Ensamblador: MPLAB X IDE v6.30
;=========================================================

#include <xc.inc>   ; Incluir definiciones del ensamblador para PIC18F4550
; Configuración de bits de configuración (Fuses)
CONFIG  FOSC = INTOSC_EC   ; Usa el oscilador interno a 8 MHz
CONFIG  WDT = OFF            ; Deshabilitar el Watchdog Timer
CONFIG  LVP = OFF            ; Deshabilitar la programación en bajo voltaje
CONFIG  PBADEN = OFF         ; Configurar los pines de PORTB como digitales
CONFIG  MCLRE  = OFF          ; Pin de reset externo desactivado
CONFIG  XINST  = OFF          ; Juego de instrucciones extendido OFF
CONFIG  PWRT   = ON           ; Espera un momento al encender antes de arrancar
    
; Vectores de Inicio e Interrupción
    PSECT  resetVec, class=CODE, reloc=2
    ORG     0b00000000          ; 0x00 Reset, cuando el micro se enciende o se resetea se va directico pa 0x0000
    GOTO    INIT
	 
    PSECT  intVec, class=CODE, reloc=2
    ORG     0b00001000          ; 0x08 Vector de interrupción, al pisarse el boton se esto lo q hace es pausar lo que este haciendo el micro en ese momento para directamente a la nueva instruccion
    GOTO    ISR

; Inicio
INIT:
    ; OSCCON:
    ; IRCF2:IRCF0 = 111 -> 8 MHz
    ; SCS1:SCS0   = 10  -> reloj interno
    ; Valor: 0b01110010
    MOVLW   0b01110010
    MOVWF   OSCCON, a
	
    ; LEDs como salidas, apagados
    CLRF    TRISD, a        ; todo PORTD = salida (osea que son 0 bb, 0 es salida)
    CLRF    LATD,  a        ; apaga todos los LEDs

    ; Botones como entradas
    BSF     TRISB, 0, a     ; RB0 = entrada (botón secuencia)
    BSF     TRISB, 1, a     ; RB1 = entrada (botón velocidad)
    
    ;Al presionarse el botón va de 5V a GND
    BCF     INTCON2, 6, a   ; pone en 0 el bit 6 (INTEDG0)
    BCF     INTCON2, 5, a   ; pone en 0 el bit 5 (INTEDG1)
    
    ;Limpiar banderas
    BCF     INTCON,  1, a   ; limpia bandera de INT0
    BCF     INTCON3, 0, a   ; limpia bandera de INT1
    
    ;Habilitamos interrupciones
    BSF     INTCON,  4, a   ; =1, habilita INT0
    BSF     INTCON3, 3, a   ; =1, habilita INT1
    BSF     INTCON,  7, a   ; GIE=1, habilita interrupciones globales
    
MAIN:
    MOVF    SEQ, W, a       ; carga SEQ en la mesa de trabajo (W)
    XORLW   0               ; ¿SEQ es 0 o no mi so?
    BZ      SECUENCIA_0     ; si sí, bien pueda siga a SECUENCIA_0

    MOVF    SEQ, W, a       ; vuelve a cargar SEQ (XORLW lo modifica)
    XORLW   1               ; ¿SEQ es 1?
    BZ      SECUENCIA_1

    MOVF    SEQ, W, a
    XORLW   2               ; ¿SEQ es 2?
    BZ      SECUENCIA_2

    GOTO    SECUENCIA_3     ; si no fue 0,1,2 entonces es 3
    
    SECUENCIA_0:            ; Desplazamiento izq ? der
	MOVLW   0x01
	MOVWF   LATD, a     ; enciende RD0
	CALL    DELAY
	MOVLW   0x02
	MOVWF   LATD, a     ; enciende RD1
	CALL    DELAY
	MOVLW   0x04
	MOVWF   LATD, a     ; enciende RD2
	CALL    DELAY
	MOVLW   0x08
	MOVWF   LATD, a     ; enciende RD3
	CALL    DELAY
	GOTO    MAIN

    SECUENCIA_1:            ; Parpadeo total
	MOVLW   0x0F
	MOVWF   LATD, a     ; enciende todos
	CALL    DELAY
	CLRF    LATD, a     ; apaga todos
	CALL    DELAY
	GOTO    MAIN

    SECUENCIA_2:            ; Relleno progresivo
	MOVLW   0x01
	MOVWF   LATD, a     ; RD0
	CALL    DELAY
	MOVLW   0x03
	MOVWF   LATD, a     ; RD0+RD1
	CALL    DELAY
	MOVLW   0x07
	MOVWF   LATD, a     ; RD0+RD1+RD2
	CALL    DELAY
	MOVLW   0x0F
	MOVWF   LATD, a     ; todos
	CALL    DELAY
	GOTO    MAIN

    SECUENCIA_3:            ; Mitad interior vs esquinas
	MOVLW   0x06
	MOVWF   LATD, a     ; RD1 y RD2 (mitad)
	CALL    DELAY
	MOVLW   0x09
	MOVWF   LATD, a     ; RD0 y RD3 (esquinas)
	CALL    DELAY
	GOTO    MAIN