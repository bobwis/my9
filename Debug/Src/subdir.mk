################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/adcstream.c \
../Src/eeprom.c \
../Src/ethernetif.c \
../Src/fatfs.c \
../Src/freertos.c \
../Src/httpclient.c \
../Src/lwip.c \
../Src/main.c \
../Src/mydebug.c \
../Src/neo7m.c \
../Src/stm32f7xx_hal_msp.c \
../Src/stm32f7xx_hal_timebase_TIM.c \
../Src/stm32f7xx_it.c \
../Src/syscalls.c \
../Src/system_stm32f7xx.c \
../Src/udpstream.c \
../Src/usb_device.c \
../Src/usbd_cdc_if.c \
../Src/usbd_conf.c \
../Src/usbd_desc.c \
../Src/user_diskio.c \
../Src/www.c 

OBJS += \
./Src/adcstream.o \
./Src/eeprom.o \
./Src/ethernetif.o \
./Src/fatfs.o \
./Src/freertos.o \
./Src/httpclient.o \
./Src/lwip.o \
./Src/main.o \
./Src/mydebug.o \
./Src/neo7m.o \
./Src/stm32f7xx_hal_msp.o \
./Src/stm32f7xx_hal_timebase_TIM.o \
./Src/stm32f7xx_it.o \
./Src/syscalls.o \
./Src/system_stm32f7xx.o \
./Src/udpstream.o \
./Src/usb_device.o \
./Src/usbd_cdc_if.o \
./Src/usbd_conf.o \
./Src/usbd_desc.o \
./Src/user_diskio.o \
./Src/www.o 

C_DEPS += \
./Src/adcstream.d \
./Src/eeprom.d \
./Src/ethernetif.d \
./Src/fatfs.d \
./Src/freertos.d \
./Src/httpclient.d \
./Src/lwip.d \
./Src/main.d \
./Src/mydebug.d \
./Src/neo7m.d \
./Src/stm32f7xx_hal_msp.d \
./Src/stm32f7xx_hal_timebase_TIM.d \
./Src/stm32f7xx_it.d \
./Src/syscalls.d \
./Src/system_stm32f7xx.d \
./Src/udpstream.d \
./Src/usb_device.d \
./Src/usbd_cdc_if.d \
./Src/usbd_conf.d \
./Src/usbd_desc.d \
./Src/user_diskio.d \
./Src/www.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o: ../Src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16 '-D__weak=__attribute__((weak))' '-D__packed=__attribute__((__packed__))' -DUSE_HAL_DRIVER -DSTM32F767xx -I"C:/projects/lightning/my9/Inc" -I"C:/projects/lightning/my9/Middlewares/Third_Party/LwIP/src/include" -I"C:/projects/lightning/my9/Middlewares/Third_Party/LwIP/system" -I"C:/projects/lightning/my9/Drivers/STM32F7xx_HAL_Driver/Inc" -I"C:/projects/lightning/my9/Drivers/STM32F7xx_HAL_Driver/Inc/Legacy" -I"C:/projects/lightning/my9/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1" -I"C:/projects/lightning/my9/Middlewares/Third_Party/LwIP/src/include/netif/ppp" -I"C:/projects/lightning/my9/Middlewares/ST/STM32_USB_Device_Library/Core/Inc" -I"C:/projects/lightning/my9/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc" -I"C:/projects/lightning/my9/Drivers/CMSIS/Device/ST/STM32F7xx/Include" -I"C:/projects/lightning/my9/Middlewares/Third_Party/FreeRTOS/Source/include" -I"C:/projects/lightning/my9/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS" -I"C:/projects/lightning/my9/Middlewares/Third_Party/LwIP/src/include/lwip" -I"C:/projects/lightning/my9/Middlewares/Third_Party/LwIP/src/include/lwip/apps" -I"C:/projects/lightning/my9/Middlewares/Third_Party/LwIP/src/include/lwip/priv" -I"C:/projects/lightning/my9/Middlewares/Third_Party/LwIP/src/include/lwip/prot" -I"C:/projects/lightning/my9/Middlewares/Third_Party/LwIP/src/include/netif" -I"C:/projects/lightning/my9/Middlewares/Third_Party/LwIP/src/include/posix" -I"C:/projects/lightning/my9/Middlewares/Third_Party/LwIP/src/include/posix/sys" -I"C:/projects/lightning/my9/Middlewares/Third_Party/LwIP/system/arch" -I"C:/projects/lightning/my9/Drivers/CMSIS/Include" -I"C:/projects/lightning/my9/Middlewares/Third_Party/FatFs/src" -I"C:/projects/lightning/my9/Middlewares/Third_Party/LwIP/src/apps/httpd"  -O2 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


