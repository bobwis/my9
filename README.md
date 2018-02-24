# Lightning tracker STM32


Hardware configuration, GPS pinout etc here

GPS Neo7M, uses serial port

GPS Tx -> STM32 USART6 Rx, D0/PG9 on STM32 board
GPS Rx -> STM32 USART6 Tx, D16/PC6 on STM32 Board
GPS 1PPS -> STM32 Timer2 CH3 Capture input, D36/PB10 on STM32

ADC Input (Antenna / LowPass etc) -> ADC PA3/A0 on STM32

DAC Output (used for test signal generation)  STM32 PA4/D24 Out -> Posible ADC input as above

 