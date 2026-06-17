 #include <xc.inc>   ; Incluir definiciones del ensamblador para PIC18F4550

    ; Configuración de bits de configuración (Fuses)
    CONFIG  FOSC = INTOSCIO_EC   ; Usa el oscilador interno a 8 MHz
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
    CLRF    TRISB       ; Configurar PORTB como salida (0 = salida, 1 = entrada)
    CLRF    LATB        ; Apagar todos los pines de PORTB (LED apagado inicialmente)
; encender led 5s
Inicio1:
    BSF     LATB, 0     ;Cambio a 1 el valor del puerto 0
    MOVLW   6           ;Asignar 5 a w
    MOVWF   Cont	;guardar 5 en la variables cont_1
Loop:
    CALL Retardo_1s	;Voy a funcion retardo
    DECFSZ Cont,F	;disminuyo hasta 0 debo añadir line abajo?
    
    GOTO Loop		;
    
; apagar led
    BCF LATB,0		;Cambio a 0 el valor del puerto 0
    MOVLW 2		;Asignar 2 a w
    MOVWF Cont		;guardar 5 en la variables cont_1
Loop1:
    CALL Retardo_1s
    DECFSZ Cont,F
    
    GOTO Loop1
    GOTo Inicio1
    
Retardo_1s:
    MOVLW   125          ; Cargar el valor 25 en el registro W (contador externo)
    MOVWF   ContadorExterno  ; Guardar el valor en la variable ContadorExterno

LoopExterno:
    MOVLW   250         ; Cargar el valor 250 en el registro W (contador interno)
    MOVWF   ContadorInterno  ; Guardar el valor en la variable ContadorInterno

LoopInterno:
    NOP                 ; No hacer nada (consume un ciclo de instrucción)
    NOP                 ; No hacer nada (consume otro ciclo)
    NOP                 ; No hacer nada (consume otro ciclo)
   
    DECFSZ  ContadorInterno, F  ; Decrementar ContadorInterno, si es cero, salta la siguiente instrucción
    GOTO    LoopInterno         ; Si no es cero, repetir el bucle interno

    DECFSZ  ContadorExterno, F  ; Decrementar ContadorExterno, si es cero, salta la siguiente instrucción
    GOTO    LoopExterno         ; Si no es cero, repetir el bucle externo

    RETURN              ; Retornar al programa principal después del retardo    
    ;===============================================
    ; Definición de Variables
    ;===============================================

    PSECT udata  ; Sección de datos sin inicializar (variables en RAM)
ContadorExterno:   DS 1   ; Reserva 1 byte de memoria para el contador externo
ContadorInterno:   DS 1   ; Reserva 1 byte de memoria para el contador interno
Cont:   DS 1   ; Reserva 1 byte de memoria para el contador interno estos influyen en algo?

    END            ; Fin del código   
    