;=========================================================
; Código en Assembler para PIC18F4550
; Materia: Microcontroladores 2026.1 Universidad del Cauca
; Presentado por: Jhon Alexander Pérez Arango y Julián David Muñoz Ledezma
; Descripción: Un LED debe tener parpadeos de 1 s cada uno en un espacio temporal de 10 
;	       segundos, luego de esto debe realizar 2 parpadeos de 2 segundos y reiniciar la secuencia.
; Frecuencia: Oscilador interno de 8 MHz 
; Ensamblador: MPLAB X IDE v6.30
;=========================================================
    
#include <xc.inc>   ; Incluir definiciones del ensamblador para PIC18F4550
; Configuración de bits de configuración (Fuses)
CONFIG  FOSC = INTOSC_EC   ; Usa el oscilador interno a 8 MHz
CONFIG  WDT = OFF            ; Deshabilitar el Watchdog Timer
CONFIG  LVP = OFF            ; Deshabilitar la programación en bajo voltaje
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
    MOVLW   0x70        ; Configurar oscilador interno a 8 MHz
    MOVWF   OSCCON
    CLRF    TRISB       ; Configurar PORTB como salida (0 = salida, 1 = entrada)
    CLRF    LATB        ; Apagar todos los pines de PORTB (LED apagado inicialmente)

    ;CICLO ENCENDER LED 1 SEGUNDO
    Inicio1:
	BSF     LATB, 0     ;Cambio a 1 el valor del puerto 0
	MOVLW   1           ;Asignar 5 a w
	MOVWF   Cont	    ;guardar 5 en la variable cont
	Loop:
	    CALL Retardo_1s	    ;Voy a funcion retardo
	    DECFSZ Cont, F	    ;disminuyo hasta 0
	GOTO Loop		

    ;CICLO APAGAR LED 2 SEGUNDOS
	BCF LATB, 0		    ;Cambio a 0 el valor del puerto 0
	MOVLW 2		        ;Asignar 2 a w
	MOVWF Cont		    ;guardar 2 en la variable cont
	Loop1:
	    CALL Retardo_1s
	    DECFSZ Cont, F
	GOTO Loop1
    GOTO Inicio1

    Retardo_1s:
	MOVLW   8                        ; Contador mega para ajuste fino
	MOVWF   ContadorMega             ; Guardar valor en ContadorMega
	LoopMega:
	    MOVLW   250                      ; Cargar el valor 250 en el registro W (contador externo)
	    MOVWF   ContadorExterno          ; Guardar el valor en la variable ContadorExterno
	    LoopExterno:
		MOVLW   250                      ; Cargar el valor 250 en el registro W (contador interno)
		MOVWF   ContadorInterno          ; Guardar el valor en la variable ContadorInterno
		LoopInterno:
		    DECFSZ  ContadorInterno, F   ; Decrementar ContadorInterno, si es cero salta la siguiente instrucción
                GOTO    LoopInterno          ; Si no es cero, repetir el bucle interno
		DECFSZ  ContadorExterno, F       ; Decrementar ContadorExterno, si es cero salta la siguiente instrucción
            GOTO    LoopExterno              ; Si no es cero, repetir el bucle externo
	    DECFSZ  ContadorMega, F          ; Decrementar ContadorMega, si es cero salta la siguiente instrucción
        GOTO    LoopMega                 ; Si no es cero, repetir el bucle mega
    RETURN              ; Retornar al programa principal después del retardo

;===============================================
; Definición de Variables
;===============================================
PSECT udata  ; Sección de datos sin inicializar (variables en RAM)
ContadorMega:      DS 1   ; Reserva 1 byte de memoria para el contador mega
ContadorExterno:   DS 1   ; Reserva 1 byte de memoria para el contador externo
ContadorInterno:   DS 1   ; Reserva 1 byte de memoria para el contador interno
Cont:              DS 1   ; Reserva 1 byte de memoria para el contador de segundos
END                       ; Fin del código