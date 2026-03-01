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
        ; IRCF2:IRCF0 = 110 -> 4 MHz
        ; SCS1:SCS0   = 10  -> reloj interno
        ; Valor: 0b01100010
        MOVLW   0b01100010
        MOVWF   OSCCON, a
	
	; LED RD0 salida, apagado
	BCF     TRISD, 0, a
        BCF     LATD,  0, a

	; Botón RB0/INT0 como entrada
        BSF     TRISB, 0, a
	
	; Pull-ups internos PORTB deshabilitados (RBPU=1)
        ; (Con botón a VDD + pull-down externo NO se usan pull-ups)
        BSF     INTCON2, 7, a

	; IINT0 por flanco de BAJADA (INTEDG0=0) porque botón va a GND (1->0)
        BCF     INTCON2, 6, a

        ; Habilitar INT0 + GIE
        BCF     INTCON, 1, a	; INT0IF = 0
        BSF     INTCON, 4, a	; INT0IE = 1
        BSF     INTCON, 7, a	; GIE   = 1