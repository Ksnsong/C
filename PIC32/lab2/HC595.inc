;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;HC595.inc;;;;;;;;;;;;;; ;;;;;;;;;;;;;;;;;
;Thanks to Nebojsa Matic of mikroElektronika for idea
;Var variable whose contents is transferred to outputs of shift register.
;Var1 loop counter
;Here 8bit data is used, so VAR1 gets decimal 8 to continue the loop for 8 times
 
HC595 MACRO VAR,VAR1
 
    LOCAL LOOP
    MOVLW .8
    MOVWF VAR1
LOOP 
 
    RLF VAR,F
    BTFSS STATUS,C
    BCF DATA1
    BTFSC STATUS,C
    BSF DATA1
 
    BSF CLOCK
    NOP
    BCF CLOCK
 
    DECFSZ VAR1,F
    GOTO LOOP
 
    BSF LATCH
    NOP
    BCF LATCH
 
    ENDM