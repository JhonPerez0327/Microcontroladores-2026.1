;=========================================================
; Código en Assembler para PIC18F4550
; Encendido de un led durante 5 segundos y apagado durante 2 segundos
; Usa retardos sin interrupciones ni Timer0
; Frecuencia: 8 MHz (Oscilador Interno)
; Ensamblador: MPLAB XC8 3.0
;=========================================================
    
CONTADOR1 EQU 0x20  ;CONTADOR vive en esa dirección
CONTADOR2 EQU 0X21
CONTADOR3 EQU 0X22

INICIO:
    
    BSF PORTB, 0
    ;CONTADOR DE 5 SEGUNDOS
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
    
    BCF PORTB, 0
    ;CONTADOR DE 2 SEGUNDOS
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
    
GOTO INICIO