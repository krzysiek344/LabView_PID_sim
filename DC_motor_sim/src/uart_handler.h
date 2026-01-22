#ifndef UART_HANDLER_H
#define UART_HANDLER_H

#include <stdint.h>
#include <stdbool.h>

int uart_handler_init(void);

/* Send position: "POS:<position>\n" */
int uart_send_position(float position);

/* Check if new PWM value received  */
bool uart_get_pwm(int16_t *pwm);

/* Check for RX timeout */
void uart_check_timeout(void);

#endif /* UART_HANDLER_H */

