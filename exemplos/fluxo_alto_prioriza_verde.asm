; Assembly gerado para VM Semaforos
; Total de instruções: 27

   0: READ_SENSOR  2
   1: STORE        0
   2: LOADM        0
   3: PUSH         0
   4: LOAD         30
   5: CMP         
   6: JGE          L0
   7: LOAD         0
   8: JMP          L1
   9: NOP          ; L0
  10: LOAD         1
  11: NOP          ; L1
  12: PUSH         0
  13: POP          0
  14: JZ           L3
  15: MUDAR        0
  16: ESPERAR      20
  17: JMP          L4
  18: NOP          ; L3
  19: MUDAR        0
  20: ESPERAR      10
  21: NOP          ; L4
  22: MUDAR        1
  23: ESPERAR      2
  24: MUDAR        2
  25: ESPERAR      10
  26: HALT        
