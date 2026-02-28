;=========================================================
; Código en Assembler para PIC18F45507
; Materia: Microcontroladores 2026.1 Universidad del Cauca
; Presentado por:Julian David Muñoz Ledezma
; Descripción: Generacion de 4 secuencias con 4 leds con boton para cambiar secuecia y velocidad de la secuencia 
; Usando timer0 8MHz/4 1/2MHz= 0.5u*256=128us 1/128us=7812.5  65536-7813= "57723" valor inicio para 1 s 
; Frecuencia: Oscilador interno de 8 MHz 
; Ensamblador: MPLAB X IDE v6.25
;=========================================================
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
	PSECT udata  ; Sección de datos sin inicializar (variables en RAM)
CNT1:   DS 1   ; Reserva 1 byte de memoria para el contador externo
CNT2:   DS 1   ; Reserva 1 byte de memoria para el contador externo
	; VECTORES DE INICIO 
	PSECT	resetVec, class=code, reloc=2
	ORG	0b00000000	    ;vector de inicializacion hexadec seria 0x00
	GOTO	INICIO		    ;VOY A FUNCION DE INICIO
	PSECT  main_code, class=CODE, reloc=2  ; Sección de código principal

INICIO:
	;CONFIGURACION OSCON
	;8MHZ
	;VALOR: Ob01110010
	MOVLW	0b01110010
	MOVWF	OSCCON
	CLRF	TRISD	    ;RD SALIDAS
	CLRF	LATD	    ;RD APAGADO
INICIO1:
	CALL	SEC1
	GOTO	INICIO1
SEC1:
	MOVLW	0b00000001  ;MUEVO 1 A W EL VALOR PARA ENCENDER RD0
	MOVWF	LATD	    ;MUEVO W AL PUERTO 
	CALL	RETARDO	    ;RETARDO PARA STE SECUENCIA
	MOVLW	0b00000010  ;MUEVO 2 A W EL VALOR PARA ENCENDER RD1
	MOVWF	LATD	    ;MUEVO W AL PUERTO 
	CALL	RETARDO	    
	MOVLW	0b00000100  ;MUEVO W AL PUERTO
	MOVWF	LATD	    ;MUEVO 4 A W EL VALOR PARA ENCENDER RD2
	CALL	RETARDO	    
	MOVLW	0b00001000  ;MUEVO W AL PUERTO 
	MOVWF	LATD	    ;MUEVO 8 A W EL VALOR PARA ENCENDER RD3
	CALL	RETARDO
	RETURN
;RETARDO PARA 250MS 250X250 CICLOS X0.5 INSTRUCCION A 8MHZ FOSC/4	
RETARDO:
	MOVLW	250
	MOVWF	CNT1
REP1:
	MOVLW	250
	MOVWF	CNT2
REP2:	
	DECFSZ	CNT2, F
	GOTO	REP2
	DECFSZ	CNT1, F
	GOTO	REP1	
	RETURN
	
	END