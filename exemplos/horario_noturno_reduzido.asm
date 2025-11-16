; Assembly gerado para VM Semaforos
; Total de instruções: 52

   0: READ_SENSOR  0
   1: STORE        0
   2: LOADM        0
   3: PUSH         0
   4: LOAD         6
   5: CMP         
   6: JLT          L0
   7: LOAD         0
   8: JMP          L1
   9: NOP          ; L0
  10: LOAD         1
  11: NOP          ; L1
  12: PUSH         0
  13: LOADM        0
  14: PUSH         0
  15: LOAD         22
  16: CMP         
  17: JGT          L2
  18: LOAD         0
  19: JMP          L3
  20: NOP          ; L2
  21: LOAD         1
  22: NOP          ; L3
  23: PUSH         0
  24: POP          0
  25: STORE        1
  26: POP          0
  27: JNZ          L4
  28: LOADM        1
  29: JMP          L5
  30: NOP          ; L4
  31: LOAD         1
  32: NOP          ; L5
  33: PUSH         0
  34: POP          0
  35: JZ           L7
  36: MUDAR        0
  37: ESPERAR      5
  38: MUDAR        1
  39: ESPERAR      2
  40: MUDAR        2
  41: ESPERAR      5
  42: JMP          L8
  43: NOP          ; L7
  44: MUDAR        0
  45: ESPERAR      15
  46: MUDAR        1
  47: ESPERAR      3
  48: MUDAR        2
  49: ESPERAR      12
  50: NOP          ; L8
  51: HALT        
