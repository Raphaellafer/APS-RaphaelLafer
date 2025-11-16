; Assembly gerado para VM Semaforos
; Total de instruções: 51

   0: LOAD         30
   1: STORE        0
   2: LOAD         2
   3: STORE        1
   4: LOAD         20
   5: STORE        2
   6: READ_SENSOR  0
   7: STORE        3
   8: LOADM        3
   9: PUSH         0
  10: LOAD         10
  11: CMP         
  12: JGE          L0
  13: LOAD         0
  14: JMP          L1
  15: NOP          ; L0
  16: LOAD         1
  17: NOP          ; L1
  18: PUSH         0
  19: LOADM        3
  20: PUSH         0
  21: LOAD         20
  22: CMP         
  23: JLE          L2
  24: LOAD         0
  25: JMP          L3
  26: NOP          ; L2
  27: LOAD         1
  28: NOP          ; L3
  29: POP          0
  30: STORE        4
  31: POP          0
  32: JZ           L4
  33: LOADM        4
  34: JMP          L5
  35: NOP          ; L4
  36: LOAD         0
  37: NOP          ; L5
  38: PUSH         0
  39: MUDAR        0
  40: LOADM        0
  41: ESPERAR     
  42: MUDAR        1
  43: LOADM        1
  44: ESPERAR     
  45: MUDAR        2
  46: LOADM        2
  47: ESPERAR     
  48: READ_SENSOR  0
  49: STORE        3
  50: HALT        
