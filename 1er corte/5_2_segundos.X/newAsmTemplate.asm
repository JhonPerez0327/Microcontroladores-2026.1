;=========================================================
; Código en Assembler para PIC18F4550
; Encendido de un led durante 5 segundos y apagado durante 2 segundos
; Usa retardos sin interrupciones ni Timer0
; Frecuencia: 4 MHz (Oscilador Interno)
; Ensamblador: MPLAB XC8 3.0
;=========================================================
    
#include <xc.inc>   ; Incluir definiciones del ensamblador para PIC18F4550

    ; Configuración de bits de configuración (Fuses)
    CONFIG  FOSC = INTOSCIO_EC   ; Usa el oscilador interno a 4 MHz
    CONFIG  WDT = OFF            ; Deshabilitar el Watchdog Timer. Si queremos trabajar con loops tenemos q tener esto en OFF
    CONFIG  LVP = OFF            ; Deshabilitar la programación en bajo voltaje, para evitar ruida se OFF
    CONFIG  PBADEN = OFF         ; Configurar los pines de PORTB como digitales
    
;===============================================
; Vectores de Inicio
;===============================================

PSECT  resetVec, class=CODE, reloc=2  ; Sección para el vector de reinicio
ORG     0x00                          ; Dirección de inicio
GOTO    Inicio                         ; Saltar a la rutina de inicio
    
;===============================================
; Código Principal
;===============================================  
    
PSECT  main_code, class=CODE, reloc=2  ; Sección de código principal

Inicio:
    CLRF    TRISB       ; Configurar PORTB como salida (0 = salida, 1 = entrada)
    CLRF    LATB        ; Apagar todos los pines de PORTB (LED apagado inicialmente)

Loop:
    BTG     LATB, 0     ; Alternar el estado del LED en RB0 (si está encendido, lo apaga y viceversa)
    CALL    Retardo_1s  ; Llamar a la rutina de retardo de 1 segundo
    GOTO    Loop        ; Repetir el proceso de parpadeo en bucle infinito 
;------------------------------------------------------------------------------------------------------
    Retardo_1s:
    
    ;CONTADOR DE 5 SEGUNDOS -------
    
    MOVLW 250 ;255 se va pa W
    MOVWF CONTADOR1 ;255 se mueve a CONTADOR
 
    BUCLE_EXT_5s:
	MOVLW 200
	MOVWF CONTADOR2
    
	BUCLE_INT1_5s:
	    DECFSZ CONTADOR2, F ;Contador de 200 a 0
	    GOTO BUCLE_INT1_5s ; Si no está en 0 te vas otra vez pa BUCLE_INT1, si sí, puede seguir tranquilo mi papacho
    
	MOVLW 100
	MOVWF CONTADOR3
    
	BUCLE_INT2_5s:
	    DECFSZ CONTADOR3, F
	    GOTO BUCLE_INT2_5s

    DECFSZ CONTADOR1, F
    GOTO BUCLE_EXT_5s
    
    ;CONTADOR DE 2 SEGUNDOS -------
    
    MOVLW 250 ;255 se va pa W
    MOVWF CONTADOR1 ;255 se mueve a CONTADOR
 
    BUCLE_EXT_2s:
	MOVLW 200
	MOVWF CONTADOR2
    
	BUCLE_INT1_2s:
	    DECFSZ CONTADOR2, F ;Contador de 200 a 0
	    GOTO BUCLE_INT1_2s ; Si no está en 0 te vas otra vez pa BUCLE_INT1, si sí, puede seguir tranquilo mi papacho
    
	MOVLW 40
	MOVWF CONTADOR3
    
	BUCLE_INT2_2s:
	    DECFSZ CONTADOR3, F
	    GOTO BUCLE_INT2_2s

    DECFSZ CONTADOR1, F
    GOTO BUCLE_EXT_2s
    
    RETURN ;Retornar al programa principal después del retardo
;===============================================
    ; Definición de Variables
    ;===============================================

    PSECT udata  ; Sección de datos sin inicializar (variables en RAM)
    CONTADOR1:   DS 1   ; Reserva 1 byte de memoria para el contador externo
    CONTADOR2:   DS 1   ; Reserva 1 byte de memoria para el contador interno
    CONTADOR3:   DS 1   ; Reserva 1 byte de memoria para el contador interno


END    ; Fin del código