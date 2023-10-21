#if defined(HAS_GYRO)
#include "targets.h"
#include "elrs_eeprom.h"
#include "config.h"
#include "mode_safe.h"
#include "gyro.h"
#include "mixer.h"
#include "pid.h"
#include "gyro_types.h"

/**
 * Airplane Safe Mode
 *
 * This allows normal flying, but tries to stop the plane going past set angles.
*/

#define PI_180 0.0174532925199

const float kP_ail = 2.00;       // Proportional gain
const float kI_ail = 0.00;       // Integral gain
const float kD_ail = 0.00;       // Derivative gain

const float kP_ele = 2.00;       // Proportional gain
const float kI_ele = 0.00;       // Integral gain
const float kD_ele = 0.00;       // Derivative gain

const float kP_yaw = 0.01;       // Proportional gain
const float kI_yaw = 0.00;       // Integral gain
const float kD_yaw = 0.00;       // Derivative gain

const float max_angle_roll = 30 * PI_180; // Convert degrees to radians
const float max_angle_pitch= 30 * PI_180; // Convert degrees to radians

void safe_controller_initialize()
{
    // Set limits to two to be able to fully override a full stick input command
    pid_roll.configure(kP_ail, kI_ail, kD_ail, 2.0, -2.0);
    pid_roll.reset();
    pid_pitch.configure(kP_ele, kI_ele, kD_ele, 2.0, -2.0);
    pid_pitch.reset();

    // For yaw we use standard angular rate
    pid_yaw.configure(kP_yaw, kI_yaw, kD_yaw, 1.0, -1.0);
    pid_yaw.reset();
}

void _calculate_pid(PID *pid, float angle, float max_angle)
{
    if (abs(angle) < max_angle) {
        pid->reset();
    } else {
        float setpoint = angle > 0 ? max_angle : - max_angle;
        pid->calculate(setpoint, angle);
    }
}

void safe_controller_calculate_pid()
{
    _calculate_pid(&pid_roll, gyro.ypr[2], max_angle_roll);
    _calculate_pid(&pid_pitch, gyro.ypr[1], max_angle_pitch);

    pid_yaw.calculate(0, gyro.f_gyro[2]);
}

float safe_controller_out(
    gyro_output_channel_function_t channel_function,
    uint16_t us
) {
    float command = us_command_to_float(us);
    float correction = 0.0;

    if (channel_function == FN_AILERON)
        correction = pid_roll.output;
    else if (channel_function == FN_ELEVATOR)
        correction = pid_pitch.output;
    else if (channel_function == FN_RUDDER)
        correction = pid_yaw.output;

    correction *= gyro.gain;

    return command + correction;
}
#endif
