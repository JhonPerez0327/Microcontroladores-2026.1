;========================================================================
; PIC18F4550 - LED con 1s encendido, 2s apagado
; Descripsion:Programa que realiza el parpadeo de un led:1s encendido 2s apagado
; Oscilador interno a 8 MHz
; Requisitos:
;  - PIC18F4550
;  - FUENTE DE C.C,PROTOBOARD,RESISTENCIA,LED,CABLES
;========================================================================
	#include <xc.inc>
    
; Config 
	CONFIG FOSC  = INTOSC_EC    ;USA RELOJ INTERNO DE 8MHZ
	CONFIG WDT   = OFF	    ;NO PERMITE CICLOS ON;PERMITE OFF    
	CONFIG LVP   = OFF	    ;NO PERMITE PROG CON VOLTAJES BAJOS
    	CONFIG PBADEN = OFF	    ;PUERTO B CONFIG COMO SALIDA DIGITAL
	CONFIG MCLRE = OFF	    ;TENGO QUE CONECTAR VOLTAJE 
	CONFIG XINST = OFF	    ;OFF ENTRADAS AVANZADAS
	CONFIG PWRT  = ON	    ;ESPERA MILISEGUNDOS PARA ESTABILIZARSE
	CONFIG DEBUG = OFF	    ;NO RESERVA RECURSOS
; VECTORES DE INICIO 
	PSECT	resetVec, class=code, reloc=2
	ORG	0b00000000	    ;vector de inicializacion hexadec seria 0x00
	GOTO	INICIO		    ;VOY A FUNCION DE INICIO
	PSECT  main_code, class=CODE, reloc=2  ; Sección de código principal

INICIO:
    ;OSCON
    ;IRCF2:IRCF0=111 8MHZ
    ;BIT7 0 PARA Q CUANDO ENVIE UN SLEEP LO EJECUTE LO ACTIVO CON EL PIN MCLR
    ;BIT6-4 FRECEUNCAI DEL OCILADOR
    ;BIT3 VAMOS A ESPERAR EL OCILADOR PRIMARIO 
    ;BIT2 FRECUENCIA ESTABLE
    ;BIT1-0 OCILADOR INTERNO
    ;VALOR: Ob01110010
    MOVLW 0b01110010
    MOVWF OSCCON, a  
    BCF	  TRISB, 0	;PUERTO B COMO SALIDA
    BCF    LATB,0,a	;COLOCO EN 0 PINES DE B AL INICIO	
LOOP:
    ;Led encendido
    BSF	    LATB,0
    CALL RETARDO_1S
    ;Led apagado
    BCF	    LATB,0
    CALL    RETARDO_1S
    ;RETARDO1S  
RETARDO_1S:
    MOVLW   10
    MOVWF   CONT1
LOOP1:
    MOVLW   250
    MOVWF   CONT2
LOOP2:
    MOVLW    250
    MOVWF   CONT3
LOOP3:
    DECFSZ  CONT3,F
    GOTO    LOOP3
    
    DECFSZ  CONT2,F
    GOTO    LOOP2
    
    DECFSZ  CONT1,F
    GOTO    LOOP1
    
    RETURN
;========================================================
; DEFINICION DE VARIABLES
;========================================================
        PSECT udata
CONT1:     DS 1
CONT2:     DS 1
CONT3:     DS 1

        END 