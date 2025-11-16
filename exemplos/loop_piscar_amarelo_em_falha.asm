; Assembly gerado para VM Semaforos
; Total de instruções: 27

   0: LOAD         0
   1: STORE        0
   2: NOP          ; L0
   3: LOADM        0
   4: PUSH         0
   5: LOAD         10
   6: CMP         
   7: JLT          L1
   8: LOAD         0
   9: JMP          L2
  10: NOP          ; L1
  11: LOAD         1
  12: NOP          ; L2
  13: PUSH         0
  14: POP          0
  15: JZ           L4
  16: LOAD         1
  17: PISCAR       1, 1
  18: ESPERAR      1
  19: LOADM        0
  20: PUSH         0
  21: LOAD         1
  22: ADD         
  23: STORE        0
  24: JMP          L0
  25: NOP          ; L4
  26: HALT        
