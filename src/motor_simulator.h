#ifndef MOTOR_SIMULATOR_H
#define MOTOR_SIMULATOR_H

#include <stdint.h>
#include <stdbool.h>

struct motor_state {
    float position;      // 0.0 - 1000.0
    float velocity;      
    int16_t pwm_value;   // -255 to 255
    bool pwm_updated;    
};

/* Motor simulation parameters */
struct motor_params {
    float gain;              // PWM to force conversion
    float friction;          // friction 
    float mass;              // motor mass 
    float max_velocity;      // maximum velocity
    float motor_delay_ms;    
    float dt_ms;             // simulation timestep in ms
};

int motor_simulator_init(void);

void motor_simulator_update(float dt_ms);

void motor_simulator_set_pwm(int16_t pwm);

float motor_simulator_get_position(void);

float motor_simulator_get_velocity(void);

int16_t motor_simulator_get_pwm(void);

void motor_simulator_get_state(struct motor_state *state);

#endif /* MOTOR_SIMULATOR_H */




