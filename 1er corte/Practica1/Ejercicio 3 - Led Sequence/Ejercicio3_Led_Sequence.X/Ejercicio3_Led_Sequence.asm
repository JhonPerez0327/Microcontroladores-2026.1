; Código en Assembler para PIC18F4550
; Materia: Microcontroladores 2026.1 Universidad del Cauca
; Presentado por: Jhon Alexander Perez Arango
; Descripción: Generacion de 4 secuencias o efectos distintos usando 4 o más leds, 
;              este desarrollo debe contar con una entrada externa (pulsador) para 
;              el cambio de secuencia y una entrada externa que permita modificar
;              la velocidad de ejecución de la secuencia.
; Frecuencia: Oscilador interno de 8 MHz 
; Ensamblador: MPLAB X IDE v6.30
;=========================================================

#include <xc.inc>   ; Incluir definiciones del ensamblador para PIC18F4550

; Configuración de bits de configuración (Fuses)
CONFIG  FOSC = INTOSC_EC   ; Usa el oscilador interno a 8 MHz
CONFIG  WDT = OFF            ; Deshabilitar el Watchdog Timer
CONFIG  LVP = OFF            ; Deshabilitar la programación en bajo voltaje
CONFIG  PBADEN = OFF         ; Configurar los pines de PORTB como digitales
CONFIG  MCLRE  = OFF         ; Pin de reset externo desactivado
CONFIG  XINST  = OFF         ; Juego de instrucciones extendido OFF
CONFIG  PWRT   = ON          ; Espera un momento al encender antes de arrancar

; Variables en RAM (van primero para que el ensamblador las conozca antes de usarlas)
PSECT udata
SEQ:    DS 1        ; secuencia actual (0 a 3)
VEL:    DS 1        ; velocidad actual (0 a 3)
CNT1:   DS 1        ; contador interno del delay
CNT2:   DS 1        ; contador externo del delay
DB_CNT: DS 1        ; contador del antirrebote

; Vectores de Inicio e Interrupción
    PSECT  resetVec, class=CODE, reloc=2
    ORG     0x0000              ; 0x00 Reset, cuando el micro se enciende o se resetea se va directico pa 0x0000
    GOTO    INIT

    PSECT  intVec, class=CODE, reloc=2
    ORG     0x0008              ; 0x08 Vector de interrupción, al pisarse el boton esto lo q hace es pausar lo que este haciendo el micro en ese momento para ir directamente a la nueva instruccion
    GOTO    ISR

; Inicio
INIT:
    ; OSCCON:
    ; IRCF2:IRCF0 = 110 -> 8 MHz
    ; SCS1:SCS0   = 10  -> reloj interno explícito
    ; Valor: 0b01100010
    MOVLW   0b01100010
    MOVWF   OSCCON, a

    ; LEDs como salidas, apagados
    CLRF    TRISD, a        ; todo PORTD = salida (osea que son 0 bb, 0 es salida)
    CLRF    LATD,  a        ; apaga todos los LEDs

    ; Botones como entradas
    BSF     TRISB, 0, a     ; RB0 = entrada (botón secuencia)
    BSF     TRISB, 1, a     ; RB1 = entrada (botón velocidad)

    ; Al presionarse el botón va de 5V a GND
    BCF     INTCON2, 6, a   ; pone en 0 el bit 6 (INTEDG0)
    BCF     INTCON2, 5, a   ; pone en 0 el bit 5 (INTEDG1)

    ; Limpiar banderas
    BCF     INTCON,  1, a   ; limpia bandera de INT0
    BCF     INTCON3, 0, a   ; limpia bandera de INT1

    ; Habilitamos interrupciones
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

SECUENCIA_0:                ; Desplazamiento izq ? der
    MOVLW   0x01
    MOVWF   LATD, a         ; enciende RD0
    CALL    DELAY
    MOVLW   0x02
    MOVWF   LATD, a         ; enciende RD1
    CALL    DELAY
    MOVLW   0x04
    MOVWF   LATD, a         ; enciende RD2
    CALL    DELAY
    MOVLW   0x08
    MOVWF   LATD, a         ; enciende RD3
    CALL    DELAY
    GOTO    MAIN

SECUENCIA_1:                ; Parpadeo total
    MOVLW   0x0F
    MOVWF   LATD, a         ; enciende todos
    CALL    DELAY
    CLRF    LATD, a         ; apaga todos
    CALL    DELAY
    GOTO    MAIN

SECUENCIA_2:                ; Relleno progresivo
    MOVLW   0x01
    MOVWF   LATD, a         ; RD0
    CALL    DELAY
    MOVLW   0x03
    MOVWF   LATD, a         ; RD0+RD1
    CALL    DELAY
    MOVLW   0x07
    MOVWF   LATD, a         ; RD0+RD1+RD2
    CALL    DELAY
    MOVLW   0x0F
    MOVWF   LATD, a         ; todos
    CALL    DELAY
    GOTO    MAIN

SECUENCIA_3:                ; Mitad interior vs esquinas
    MOVLW   0x06
    MOVWF   LATD, a         ; RD1 y RD2 (mitad)
    CALL    DELAY
    MOVLW   0x09
    MOVWF   LATD, a         ; RD0 y RD3 (esquinas)
    CALL    DELAY
    GOTO    MAIN

;=========================================================================
; DELAY: retardo variable según VEL
;=========================================================================
DELAY:
    MOVF    VEL, W, a       ; carga velocidad en W

    XORLW   0               ; ¿lenta?
    BZ      D_LENTA

    MOVF    VEL, W, a
    XORLW   1               ; ¿media?
    BZ      D_MEDIA

    MOVF    VEL, W, a
    XORLW   2               ; ¿rápida?
    BZ      D_RAPIDA

    GOTO    D_MUY_RAPIDA    ; si no fue 0,1,2 entonces es 3

D_LENTA:
    CALL    DELAY_BASE      ; 4 llamadas ? 250ms (cascada hacia abajo)
    CALL    DELAY_BASE
D_MEDIA:
    CALL    DELAY_BASE      ; 2 llamadas ? 125ms (cascada hacia abajo)
D_RAPIDA:
    CALL    DELAY_BASE      ; 1 llamada ? 62ms
    RETURN

D_MUY_RAPIDA:
    CALL    DELAY_BASE      ; 1 llamada ? 30ms
    RETURN

; ~62ms a 8MHz
DELAY_BASE:
    MOVLW   250
    MOVWF   CNT1, a
LOOP1:
    MOVLW   250
    MOVWF   CNT2, a
LOOP2:
    DECFSZ  CNT2, f, a      ; decrementa CNT2, salta si llega a 0
    GOTO    LOOP2
    DECFSZ  CNT1, f, a      ; decrementa CNT1, salta si llega a 0
    GOTO    LOOP1
    RETURN

;=========================================================================
; DEBOUNCE: antirrebote ~20ms
; No está en los CONFIG, se hace por software
; Espera ~20ms y vuelve a verificar si el botón sigue presionado
; Si sigue presionado = pulsación real, si ya soltó = fue rebote
;=========================================================================
DEBOUNCE:
    MOVLW   80              ; 80 x 250 = 20,000 ciclos ? 20ms
    MOVWF   DB_CNT, a       ; (un registro de 8 bits solo aguanta hasta 255, por eso dos contadores)
DB_LOOP:
    MOVLW   250
    MOVWF   CNT1, a
DB_INNER:
    DECFSZ  CNT1, f, a
    GOTO    DB_INNER
    DECFSZ  DB_CNT, f, a
    GOTO    DB_LOOP
    RETURN

;=========================================================================
; ISR: rutina de interrupción
;=========================================================================
ISR:
    BTFSC   INTCON,  1, a       ; ¿INT0IF=1? (botón secuencia)
    CALL    PROC_INT0

    BTFSC   INTCON3, 0, a       ; ¿INT1IF=1? (botón velocidad)
    CALL    PROC_INT1

    RETFIE                      ; regresa y reactiva interrupciones (GIE=1)
                                ; (si usaramos RETURN las interrupciones quedarian apagadas para siempre)

; --- INT0: cambio de secuencia ---
PROC_INT0:
    BCF     INTCON, 1, a        ; limpia bandera INT0IF
    CALL    DEBOUNCE            ; espera antirrebote
    BTFSC   PORTB, 0, a         ; ¿RB0 sigue en 0? (presionado)
    RETURN                      ; no, fue rebote, ignora
    INCF    SEQ, f, a           ; incrementa secuencia
    MOVF    SEQ, W, a
    XORLW   4                   ; ¿llegó a 4?
    BNZ     FIN_INT0
    CLRF    SEQ, a              ; reinicia a 0
FIN_INT0:
    RETURN

; --- INT1: cambio de velocidad ---
PROC_INT1:
    BCF     INTCON3, 0, a       ; limpia bandera INT1IF
    CALL    DEBOUNCE            ; espera antirrebote
    BTFSC   PORTB, 1, a         ; ¿RB1 sigue en 0? (presionado)
    RETURN                      ; no, fue rebote, ignora
    INCF    VEL, f, a           ; incrementa velocidad
    MOVF    VEL, W, a
    XORLW   4                   ; ¿llegó a 4?
    BNZ     FIN_INT1
    CLRF    VEL, a              ; reinicia a 0
FIN_INT1:
    RETURN

    END