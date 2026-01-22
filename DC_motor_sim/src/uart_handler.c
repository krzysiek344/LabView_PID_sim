#include "uart_handler.h"
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

LOG_MODULE_REGISTER(uart_handler, LOG_LEVEL_DBG);


/* UART4 */
#define UART_DEVICE_NODE DT_NODELABEL(uart4)
static const struct device *const uart_dev = DEVICE_DT_GET(UART_DEVICE_NODE);

#define UART_RX_BUF_SIZE 64
#define UART_RX_TIMEOUT_MS 200  // Timeout 
static char uart_rx_buf[UART_RX_BUF_SIZE];
static int uart_rx_index = 0;
static int64_t last_rx_char_time = 0;  

/* Received PWM value */
static struct {
    int16_t value;
    bool new_value;
    struct k_mutex lock;
} pwm_rx = {
    .value = 0,
    .new_value = false
};

#ifdef UART_ECHO_DEBUG //debug feature

static void uart_echo_pwm(int16_t pwm)
{
    char echo_buf[16];
    int len;
    
    len = snprintf(echo_buf, sizeof(echo_buf), "ECHO:%d\n", pwm);
    
    if (len > 0 && len < sizeof(echo_buf)) {
        for (int i = 0; i < len; i++) {
            uart_poll_out(uart_dev, echo_buf[i]);
        }
        LOG_DBG("Echoed PWM: %d", pwm);
    }
}
#endif /* UART_ECHO_DEBUG */


static void uart_interrupt_handler(const struct device *dev, void *user_data)
{
    uint8_t c;
    
    if (!uart_irq_update(dev)) {
        return;
    }
    
    if (!uart_irq_rx_ready(dev)) {
        return;
    }
    
    int64_t current_time = k_uptime_get();
    
    while (uart_fifo_read(dev, &c, 1) == 1) {
        LOG_INF("RX char: 0x%02x ('%c')", c, (c >= 32 && c < 127) ? c : '?');
        last_rx_char_time = current_time;
        
        if (c == '\n') {
            if (uart_rx_index > 0) {
                uart_rx_buf[uart_rx_index] = '\0';
                
                LOG_INF("RX buffer: '%s'", uart_rx_buf);
                
                char *endptr;
                long pwm_long = strtol(uart_rx_buf, &endptr, 10);
                
                if (endptr == uart_rx_buf) {
                
                    LOG_INF("Failed to parse PWM from '%s' - no number", uart_rx_buf);
                } else if (*endptr != '\0' && *endptr != ' ' && *endptr != '\t' && *endptr != '\r') {
                    
                    LOG_INF("Failed to parse PWM from '%s' - extra chars: '%s'", uart_rx_buf, endptr);
                } else {
                    int16_t pwm = (int16_t)pwm_long;
                    
                    if (pwm > 255) pwm = 255;
                    if (pwm < -255) pwm = -255;
                    
                    k_mutex_lock(&pwm_rx.lock, K_FOREVER);
                    pwm_rx.value = pwm;
                    pwm_rx.new_value = true;
                    k_mutex_unlock(&pwm_rx.lock);
                    
                    LOG_INF("Received PWM: %d", pwm);   
                    
#ifdef UART_ECHO_DEBUG
                    uart_echo_pwm(pwm);
#endif /* UART_ECHO_DEBUG */
                }
            }

            uart_rx_index = 0;
        } else if (c == '\r') {
        } else if (uart_rx_index < (UART_RX_BUF_SIZE - 1)) {
            if ((c >= '0' && c <= '9') || c == '-' || c == '+') {
                uart_rx_buf[uart_rx_index++] = c;
            } else if (c == ' ' || c == '\t') {
                if (uart_rx_index > 0) {
                }
            } else {
                if (uart_rx_index == 0) {
                    LOG_INF("Invalid char '%c' (0x%02x) at start, ignoring", 
                            (c >= 32 && c < 127) ? c : '?', c);
                } else {
                    LOG_INF("Invalid char '%c' (0x%02x), resetting buffer", 
                            (c >= 32 && c < 127) ? c : '?', c);
                    uart_rx_index = 0;
                }
            }
        } else {
            LOG_INF("Buffer overflow, resetting");
            uart_rx_index = 0;
        }
    }
}

int uart_handler_init(void)
{
    if (!device_is_ready(uart_dev)) {
        LOG_ERR("UART device not ready");
        return -ENODEV;
    }
    
    LOG_INF("UART device ready: %p", uart_dev);
    
    k_mutex_init(&pwm_rx.lock);
    
    uart_irq_callback_user_data_set(uart_dev, uart_interrupt_handler, NULL);
    uart_irq_rx_enable(uart_dev);
    
    LOG_INF("UART handler initialized (UART4, 115200 baud), RX interrupt enabled");
    
    return 0;
}

int uart_send_position(float position)
{
    char tx_buf[32];
    int len;
    
    len = snprintf(tx_buf, sizeof(tx_buf), "POS:%.1f\n", (double)position);
    
    if (len < 0 || len >= sizeof(tx_buf)) {
        LOG_ERR("Failed to format position string");
        return -EINVAL;
    }
    
    for (int i = 0; i < len; i++) {
        uart_poll_out(uart_dev, tx_buf[i]);
    }
    
    return 0;
}

bool uart_get_pwm(int16_t *pwm)
{
    bool new_value = false;
    
    if (pwm == NULL) {
        return false;
    }
    
    k_mutex_lock(&pwm_rx.lock, K_FOREVER);
    if (pwm_rx.new_value) {
        *pwm = pwm_rx.value;
        pwm_rx.new_value = false;
        new_value = true;
    }
    k_mutex_unlock(&pwm_rx.lock);
    
    return new_value;
}

void uart_check_timeout(void)
{
    if (uart_rx_index > 0) {
        int64_t now = k_uptime_get();
        if (last_rx_char_time > 0 && (now - last_rx_char_time) > UART_RX_TIMEOUT_MS) {
            LOG_INF("RX timeout after %d ms, resetting buffer (had %d chars)", 
                    UART_RX_TIMEOUT_MS, uart_rx_index);
            uart_rx_index = 0;
            last_rx_char_time = 0;
        }
    }
}

