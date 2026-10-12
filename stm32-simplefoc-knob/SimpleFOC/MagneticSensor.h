#ifndef MAGNETICSENSOR_LIB_H
#define MAGNETICSENSOR_LIB_H

/******************************************************************************/
extern long  cpr;
extern float full_rotation_offset;
extern long  angle_data_prev;
extern unsigned long velocity_calc_timestamp;
extern float angle_prev;
extern volatile unsigned char magnetic_sensor_fault;
extern volatile unsigned char magnetic_sensor_status;
extern volatile unsigned char magnetic_field_warning;
/******************************************************************************/
void MagneticSensor_Init(void);
int MagneticSensor_CheckHealth(void);
void MagneticSensor_ResetVelocity(void);
float getAngle(void);
float getVelocity(void);
/******************************************************************************/

#endif
