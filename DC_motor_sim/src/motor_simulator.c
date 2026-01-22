#include "motor_simulator.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <math.h>

LOG_MODULE_REGISTER(motor_sim, LOG_LEVEL_DBG);

static struct motor_state motor = {
    .position = 400.0f,  
    .velocity = 0.0f,
    .pwm_value = 0,
    .pwm_updated = false
};

static const struct motor_params params = {
    .gain = 0.009804f,       
    .friction = 0.5f,        
    .mass = 10.0f,          
    .max_velocity = 5.0f,   
    .motor_delay_ms = 0.0f,  //TODO, halndling motor delay
    .dt_ms = 10.0f          
};

int motor_simulator_init(void)
{
    memset(&motor, 0, sizeof(motor));
    motor.position = 400.0f;  
    
    LOG_INF("Motor simulator initialized (with inertia)");
    LOG_INF("  Start position: %.1f", (double)motor.position);
    LOG_INF("  Gain: %.6f, Friction: %.3f, Mass: %.1f", 
            (double)params.gain, (double)params.friction, (double)params.mass);
    LOG_INF("  Max velocity at PWM=255: %.2f", (double)(255.0f * params.gain / params.friction));
    
    return 0;
}

void motor_simulator_set_pwm(int16_t pwm)
{
    if (pwm > 255) {
        pwm = 255;
    } else if (pwm < -255) {
        pwm = -255;
    }
    
    motor.pwm_value = pwm;
    
    LOG_DBG("PWM set to: %d", pwm);
}

void motor_simulator_update(float dt_ms)
{
    
    float drive_force = (float)motor.pwm_value * params.gain;
    float friction_force = motor.velocity * params.friction;
    
    
    float net_force = drive_force - friction_force;

    float acceleration = net_force / params.mass;
    
    motor.velocity += acceleration * (dt_ms / params.dt_ms);
    
    if (motor.velocity > params.max_velocity) {
        motor.velocity = params.max_velocity;
    } else if (motor.velocity < -params.max_velocity) {
        motor.velocity = -params.max_velocity;
    }
    
    motor.position += motor.velocity * (dt_ms / params.dt_ms);
    
    if (motor.position > 1000.0f) {
        motor.position = 1000.0f;
        if (motor.velocity > 0.0f) {
            motor.velocity = 0.0f;
        }
    } else if (motor.position < 0.0f) {
        motor.position = 0.0f;
        if (motor.velocity < 0.0f) {
            motor.velocity = 0.0f;
        }
    }
}

float motor_simulator_get_position(void)
{
    return motor.position;
}

float motor_simulator_get_velocity(void)
{
    return motor.velocity;
}

int16_t motor_simulator_get_pwm(void)
{
    return motor.pwm_value;
}

void motor_simulator_get_state(struct motor_state *state)
{
    if (state != NULL) {
        memcpy(state, &motor, sizeof(struct motor_state));
    }
}

