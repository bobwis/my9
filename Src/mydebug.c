/*
 * mydebug.c
 *
 *  Created on: 22Dec.,2017
 *      Author: bob
 */
#include "stm32f7xx_hal.h"
#include <stdio.h>
#include "mydebug.h"


/* We need to implement own __FILE struct */
/* FILE struct is used from __FILE */
struct __FILE {
    int dummy;
};

/* You need this if you want use printf */
/* Struct FILE is implemented in stdio.h */
FILE __stdout;

int fputc(int ch, FILE *f) {
    /* Do your stuff here */
    /* Send your custom byte */
    /* Send byte to USART */
 //   TM_USART_Putc(USART1, ch);
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 10);
    /* If everything is OK, you have to return character written */
    return ch;
    /* If character is not correct, you can return EOF (-1) to stop writing */
    //return -1;
}

#ifdef __GNUC__
  /* With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf
     set to 'Yes') calls __io_putchar() */
  #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
  #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */
/**
  * @brief  Retargets the C library printf function to the USART.
  * @param  None
  * @retval None
  */
PUTCHAR_PROTOTYPE
{
  /* Place your implementation of fputc here */
  /* e.g. write a character to the EVAL_COM1 and Loop until the end of transmission */
	if (ch == '\n')
		  HAL_UART_Transmit(&huart2, "\r\n", 2, 10);
	else
	      HAL_UART_Transmit(&huart2, &ch, 1, 10);
    return ch;
}




void myhexDump (char *desc, void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        printf ("%s\n", desc);

    if (len == 0) {
        printf("  ZERO LENGTH\n");
        return;
    }
    if (len < 0) {
        printf("  NEGATIVE LENGTH: %i\n",len);
        return;
    }

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf ("  %s\r\n", buff);

            // Output the offset.
            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf ("  %s\n", buff);
}
