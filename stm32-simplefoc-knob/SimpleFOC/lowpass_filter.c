
#include "MyProject.h"

/******************************************************************************/
#define DEF_CURR_FILTER_Tf            0.005f //!< default current filter time constant
#define SPEED_LPF_TIME_CONSTANT_S     0.010f
/******************************************************************************/
static unsigned long lpf_vel_timestamp;
float y_vel_prev=0;
/******************************************************************************/
/*
float LPF_velocity(float x)
{
	unsigned long now_us;
	float Ts, alpha, y;
	
	now_us = SysTick->VAL;
	if(now_us<lpf_vel_timestamp)Ts = (float)(lpf_vel_timestamp - now_us)/9*1e-6f;
	else
		Ts = (float)(0xFFFFFF - now_us + lpf_vel_timestamp)/9*1e-6f;
	
	lpf_vel_timestamp = now_us;
	if(Ts == 0 || Ts > 0.5) Ts = 1e-3f; 
	
	alpha = DEF_VEL_FILTER_Tf/(DEF_VEL_FILTER_Tf + Ts);
	y = alpha*y_prev + (1.0f - alpha)*x;
	y_prev = y;
	
	return y;
}
*/
float LPF_velocity(float x)
{
	unsigned long now_us;
	float Ts;
	float alpha;
	float y;

	now_us = SysTick->VAL;
	if(now_us < lpf_vel_timestamp)
		Ts = (float)(lpf_vel_timestamp - now_us) / 9000000.0f;
	else
		Ts = (float)(0xFFFFFFUL - now_us + lpf_vel_timestamp) / 9000000.0f;
	lpf_vel_timestamp = now_us;
	if(Ts <= 0.0f || Ts > 0.1f) Ts = 0.002f;

	alpha = SPEED_LPF_TIME_CONSTANT_S / (SPEED_LPF_TIME_CONSTANT_S + Ts);
	y = alpha*y_vel_prev + (1.0f - alpha)*x;
	y_vel_prev = y;
	return y;
}

void LPF_velocity_reset(float value)
{
	y_vel_prev = value;
	lpf_vel_timestamp = SysTick->VAL;
}
/******************************************************************************/
