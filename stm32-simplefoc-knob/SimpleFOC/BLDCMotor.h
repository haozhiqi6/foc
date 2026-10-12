#ifndef BLDCMotor_H
#define BLDCMotor_H

/******************************************************************************/
/**
 *  Direction structure
 */
typedef enum
{
    CW      = 1,  //clockwise
    CCW     = -1, // counter clockwise
    UNKNOWN = 0   //not yet known or invalid state
} Direction;

typedef struct
{
	unsigned long magic;
	unsigned long version;
	unsigned long attempt;
	unsigned long stage;
	unsigned long error;
	unsigned long success;
	float align_voltage;
	float mid_angle;
	float end_angle;
	float moved;
} MotorAlignDiag;

/******************************************************************************/
extern long sensor_direction;
extern float voltage_power_supply;
extern float voltage_limit;
extern float voltage_sensor_align;
extern int  pole_pairs;
extern unsigned long open_loop_timestamp;
extern float velocity_limit;
extern volatile MotorAlignDiag motor_align_diag;
/******************************************************************************/
void Motor_init(void);
int Motor_initFOC(void);
void Motor_disable(void);
void loopFOC(void);
void move(float new_target);
void setPhaseVoltage(float Uq, float Ud, float angle_el);
/******************************************************************************/

#endif
