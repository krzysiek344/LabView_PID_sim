/******************************************************/
/*                                                    */
/*             (c) AGH Solar Boat 2025                */
/*          DC Motor Simulator - Testbench           */
/*                                                    */
/* Simulates DC motor with encoder for PID testing   */
/* in LabVIEW. Sends position every ~70ms via UART.  */
/*                                                    */
/******************************************************/

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>

#include "motor_simulator.h"
#include "uart_handler.h"

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

/* Thread priorities */
#define SIMULATION_THREAD_PRIORITY 5
#define TX_THREAD_PRIORITY 6

/* Thread stack sizes */
#define SIMULATION_STACK_SIZE 1024
#define TX_STACK_SIZE 1024

/* Timing constants */
#define SIMULATION_PERIOD_MS 10    // 10ms simulation step (balanced)
#define POSITION_TX_PERIOD_MS 100  // 100ms status transmission

/* Thread definitions */
K_THREAD_STACK_DEFINE(simulation_stack, SIMULATION_STACK_SIZE);
K_THREAD_STACK_DEFINE(tx_stack, TX_STACK_SIZE);

static struct k_thread simulation_thread_data;
static struct k_thread tx_thread_data;

/* Simulation thread - updates motor physics */
static void simulation_thread(void *arg1, void *arg2, void *arg3)
{
    int64_t last_time = k_uptime_get();
    
    LOG_INF("Simulation thread started");
    
    while (1) {
        int64_t current_time = k_uptime_get();
        float dt = (float)(current_time - last_time);
        last_time = current_time;
        
        /* Update motor simulation */
        motor_simulator_update(dt);
        
        /* Sleep for simulation period */
        k_msleep(SIMULATION_PERIOD_MS);
    }
}

/* Position transmission thread - sends position every ~70ms */
static void position_tx_thread(void *arg1, void *arg2, void *arg3)
{
    int64_t last_tx_time = k_uptime_get();
    
    LOG_INF("Position TX thread started");
    
    while (1) {
        int64_t current_time = k_uptime_get();
        int64_t elapsed = current_time - last_tx_time;
        
        if (elapsed >= POSITION_TX_PERIOD_MS) {
            float position = motor_simulator_get_position();
            uart_send_position(position);
            last_tx_time = current_time;
        }
        
        /* Small sleep to prevent busy waiting */
        k_msleep(10);
    }
}

/* Main thread - handles PWM reception */
int main(void)
{
    LOG_INF("========================================");
    LOG_INF("  DC Motor Simulator - Testbench");
    LOG_INF("========================================");
    LOG_INF("Board: Nucleo F446RE");
    LOG_INF("UART: UART4 (PC10/PC11)");
    LOG_INF("Baud Rate: 115200");
    LOG_INF("Status TX: ~100ms, Simulation: ~10ms");
    LOG_INF("========================================");
    
    /* Initialize motor simulator */
    if (motor_simulator_init() < 0) {
        LOG_ERR("Failed to initialize motor simulator");
        return -1;
    }
    
    /* Initialize UART handler */
    if (uart_handler_init() < 0) {
        LOG_ERR("Failed to initialize UART handler");
        return -1;
    }
    
    LOG_INF("Initialization complete, starting threads...");
    
    /* Start simulation thread */
    k_thread_create(&simulation_thread_data, simulation_stack,
                    K_THREAD_STACK_SIZEOF(simulation_stack),
                    simulation_thread,
                    NULL, NULL, NULL,
                    SIMULATION_THREAD_PRIORITY, 0, K_NO_WAIT);
    
    /* Start position TX thread */
    k_thread_create(&tx_thread_data, tx_stack,
                    K_THREAD_STACK_SIZEOF(tx_stack),
                    position_tx_thread,
                    NULL, NULL, NULL,
                    TX_THREAD_PRIORITY, 0, K_NO_WAIT);
    
    LOG_INF("All threads started, entering main loop");
    
    /* Main loop - check for new PWM values and handle RX timeout */
    while (1) {
        int16_t pwm;
        
        if (uart_get_pwm(&pwm)) {
            motor_simulator_set_pwm(pwm);
        }
        
        /* Check for RX timeout (handled in uart_handler) */
        uart_check_timeout();
        
        k_msleep(10);
    }
}

