
#include "MyProject.h"


/************************************************
main中调用的接口函数都在当前文件中
=================================================
本程序仅供学习，引用代码请标明出处
使用教程：https://blog.csdn.net/loop222/article/details/120471390
创建日期：20210925
作    者：loop222 @郑州
************************************************/
/******************************************************************************/
extern float target;
/******************************************************************************/
long sensor_direction;
float voltage_power_supply;
float voltage_limit;
float voltage_sensor_align;
int  pole_pairs;
unsigned long open_loop_timestamp;
float velocity_limit;
volatile MotorAlignDiag motor_align_diag;

#define ALIGN_DIAG_MAGIC          0x414C474EUL
#define ALIGN_DIAG_VERSION        0x00010001UL
#define ALIGN_MAX_ATTEMPTS        3U
#define ALIGN_RETRY_VOLTAGE_STEP  0.5f
#define ALIGN_DRIVER_WAKE_MS      20U
#define ALIGN_RETRY_DELAY_MS      250U
/******************************************************************************/
int alignSensor(void);
float velocityOpenloop(float target_velocity);
float angleOpenloop(float target_angle);
/******************************************************************************/
void Motor_disable(void)
{
	M1_Disable;
	TIM2->CCR1 = 0;
	TIM2->CCR2 = 0;
	TIM2->CCR3 = 0;
	TIM2->EGR = TIM_EGR_UG;
}
/******************************************************************************/
void Motor_init(void)
{
	printf("MOT: Init\r\n");
	
//	new_voltage_limit = current_limit * phase_resistance;
//	voltage_limit = new_voltage_limit < voltage_limit ? new_voltage_limit : voltage_limit;
	if(voltage_sensor_align > voltage_limit) voltage_sensor_align = voltage_limit;
	
	sensor_direction=UNKNOWN;
	
	M1_Enable;
	printf("MOT: Enable driver.\r\n");
}
/******************************************************************************/
int Motor_initFOC(void)
{
	unsigned char attempt;
	int aligned;
	int configured_pole_pairs;
	float configured_align_voltage;

	motor_align_diag.magic = ALIGN_DIAG_MAGIC;
	motor_align_diag.version = ALIGN_DIAG_VERSION;
	motor_align_diag.attempt = 0U;
	motor_align_diag.stage = 0U;
	motor_align_diag.error = 0U;
	motor_align_diag.success = 0U;
	motor_align_diag.align_voltage = 0.0f;
	motor_align_diag.mid_angle = 0.0f;
	motor_align_diag.end_angle = 0.0f;
	motor_align_diag.moved = 0.0f;

	if(!MagneticSensor_CheckHealth())
	{
		motor_align_diag.error = 1U;
		Motor_disable();
		return 0;
	}

	configured_align_voltage = voltage_sensor_align;
	configured_pole_pairs = pole_pairs;
	aligned = 0;
	for(attempt = 0U; attempt < ALIGN_MAX_ATTEMPTS; attempt++)
	{
		voltage_sensor_align = configured_align_voltage +
		                       (float)attempt * ALIGN_RETRY_VOLTAGE_STEP;
		if(voltage_sensor_align > voltage_limit)
			voltage_sensor_align = voltage_limit;
		pole_pairs = configured_pole_pairs;
		sensor_direction = UNKNOWN;
		zero_electric_angle = 0.0f;
		motor_align_diag.attempt = (unsigned long)attempt + 1UL;
		motor_align_diag.align_voltage = voltage_sensor_align;
		motor_align_diag.stage = 1U;
		motor_align_diag.error = 0U;

		M1_Enable;
		delay_ms(ALIGN_DRIVER_WAKE_MS);
		if(alignSensor() && !magnetic_sensor_fault)
		{
			aligned = 1;
			break;
		}

		Motor_disable();
		if(magnetic_sensor_fault) break;
		delay_ms(ALIGN_RETRY_DELAY_MS);
	}
	voltage_sensor_align = configured_align_voltage;
	if(!aligned || magnetic_sensor_fault)
	{
		if(motor_align_diag.error == 0U) motor_align_diag.error = 7U;
		Motor_disable();
		return 0;
	}

	angle_prev = getAngle();
	if(magnetic_sensor_fault)
	{
		motor_align_diag.error = 8U;
		Motor_disable();
		return 0;
	}
	delay_ms(5);
	shaft_velocity = shaftVelocity();
	if(magnetic_sensor_fault)
	{
		motor_align_diag.error = 9U;
		Motor_disable();
		return 0;
	}
	delay_ms(5);
	shaft_angle = shaftAngle();
	if(magnetic_sensor_fault)
	{
		motor_align_diag.error = 10U;
		Motor_disable();
		return 0;
	}
	if(controller==Type_angle) target=shaft_angle;
	delay_ms(200);
	motor_align_diag.stage = 7U;
	motor_align_diag.error = 0U;
	motor_align_diag.success = 1U;
	return 1;
}
/******************************************************************************/
int alignSensor(void)
{
	long i;
	float angle;
	float mid_angle,end_angle;
	float moved;
	
	printf("MOT: Align sensor.\r\n");
	motor_align_diag.stage = 2U;
	
	// find natural direction
	// move one electrical revolution forward
	for(i=0; i<=500; i++)
	{
		angle = _3PI_2 + _2PI * i / 500.0;
		setPhaseVoltage(voltage_sensor_align, 0,  angle);
		delay_ms(2);
	}
	mid_angle=getAngle();
	motor_align_diag.mid_angle = mid_angle;
	if(magnetic_sensor_fault || !MagneticSensor_CheckHealth())
	{
		motor_align_diag.error = 2U;
		Motor_disable();
		return 0;
	}
	
	motor_align_diag.stage = 3U;
	for(i=500; i>=0; i--) 
	{
		angle = _3PI_2 + _2PI * i / 500.0 ;
		setPhaseVoltage(voltage_sensor_align, 0,  angle);
		delay_ms(2);
	}
	end_angle=getAngle();
	motor_align_diag.end_angle = end_angle;
	if(magnetic_sensor_fault || !MagneticSensor_CheckHealth())
	{
		motor_align_diag.error = 3U;
		Motor_disable();
		return 0;
	}
	setPhaseVoltage(0, 0, 0);
	delay_ms(200);
	
	printf("mid_angle=%.4f\r\n",mid_angle);
	printf("end_angle=%.4f\r\n",end_angle);
	
	moved =  fabs(mid_angle - end_angle);
	motor_align_diag.moved = moved;
	motor_align_diag.stage = 4U;
	if((mid_angle == end_angle)||(moved < 0.02))  //相等或者几乎没有动
	{
		printf("MOT: Failed to notice movement loop222.\r\n");
		motor_align_diag.error = 4U;
		Motor_disable();
		return 0;
	}
	else if(mid_angle < end_angle)
	{
		printf("MOT: sensor_direction==CCW\r\n");
		sensor_direction=CCW;
	}
	else
	{
		printf("MOT: sensor_direction==CW\r\n");
		sensor_direction=CW;
	}
	
	
	printf("MOT: PP check: ");    //计算Pole_Pairs
	if( fabs(moved*pole_pairs - _2PI) > 0.5 )  // 0.5 is arbitrary number it can be lower or higher!
	{
		printf("fail - configured pp mismatch.\r\n");
		motor_align_diag.error = 6U;
		Motor_disable();
		return 0;
  }
	else
		printf("OK!\r\n");
	
	
	setPhaseVoltage(voltage_sensor_align, 0,  _3PI_2);  //计算零点偏移角度
	motor_align_diag.stage = 5U;
	delay_ms(700);
	angle = getAngle();
	if(magnetic_sensor_fault || !MagneticSensor_CheckHealth())
	{
		motor_align_diag.error = 5U;
		Motor_disable();
		return 0;
	}
	zero_electric_angle = _normalizeAngle(_electricalAngle(sensor_direction*angle, pole_pairs));
	delay_ms(20);
	printf("MOT: Zero elec. angle:");
	printf("%.4f\r\n",zero_electric_angle);
	
	setPhaseVoltage(0, 0, 0);
	delay_ms(200);
	motor_align_diag.stage = 6U;
	motor_align_diag.error = 0U;
	
	return 1;
}
/******************************************************************************/
void loopFOC(void)
{
	if( controller==Type_angle_openloop || controller==Type_velocity_openloop ) return;
	
	shaft_angle = shaftAngle();// shaft angle
	if(magnetic_sensor_fault)
	{
		Motor_disable();
		return;
	}
	electrical_angle = electricalAngle();// electrical angle - need shaftAngle to be called first
	
	switch(torque_controller)
	{
		case Type_voltage:  // no need to do anything really
			break;
		case Type_dc_current:
			break;
		case Type_foc_current:
			break;
		default:
			printf("MOT: no torque control selected!");
			break;
	}
	// set the phase voltage - FOC heart function :)
  setPhaseVoltage(voltage.q, voltage.d, electrical_angle);
}
/******************************************************************************/
void move(float new_target)
{
	shaft_velocity = shaftVelocity();
	
	switch(controller)
	{
		case Type_torque:
			if(torque_controller==Type_voltage)voltage.q = new_target;  // if voltage torque control
		  else
				current_sp = new_target; // if current/foc_current torque control
			break;
		case Type_angle:
			// angle set point
      shaft_angle_sp = new_target;
      // calculate velocity set point
      shaft_velocity_sp = PID_angle( shaft_angle_sp - shaft_angle );
      // calculate the torque command
      current_sp = PID_velocity(shaft_velocity_sp - shaft_velocity); // if voltage torque control
      // if torque controlled through voltage  
      if(torque_controller == Type_voltage)
			{
				voltage.q = current_sp;
        voltage.d = 0;
      }
			break;
		case Type_velocity:
			// velocity set point
      shaft_velocity_sp = new_target;
      // calculate the torque command
      current_sp = PID_velocity(shaft_velocity_sp - shaft_velocity); // if current/foc_current torque control
      // if torque controlled through voltage control 
      if(torque_controller == Type_voltage)
			{
        voltage.q = current_sp;  // use voltage if phase-resistance not provided
        voltage.d = 0;
      }
			break;
		case Type_velocity_openloop:
			// velocity control in open loop
      shaft_velocity_sp = new_target;
      voltage.q = velocityOpenloop(shaft_velocity_sp); // returns the voltage that is set to the motor
      voltage.d = 0;
			break;
		case Type_angle_openloop:
			// angle control in open loop
      shaft_angle_sp = new_target;
      voltage.q = angleOpenloop(shaft_angle_sp); // returns the voltage that is set to the motor
      voltage.d = 0;
			break;
	}
}
/******************************************************************************/
void setPhaseVoltage(float Uq, float Ud, float angle_el)
{
	float Uout;
	uint32_t sector;
	float T0,T1,T2;
	float Ta,Tb,Tc;
	
	if(Ud) // only if Ud and Uq set 
	{// _sqrt is an approx of sqrt (3-4% error)
		Uout = _sqrt(Ud*Ud + Uq*Uq) / voltage_power_supply;
		// angle normalisation in between 0 and 2pi
		// only necessary if using _sin and _cos - approximation functions
		angle_el = _normalizeAngle(angle_el + atan2(Uq, Ud));
	}
	else
	{// only Uq available - no need for atan2 and sqrt
		Uout = Uq / voltage_power_supply;
		// angle normalisation in between 0 and 2pi
		// only necessary if using _sin and _cos - approximation functions
		angle_el = _normalizeAngle(angle_el + _PI_2);
	}
	if(Uout> 0.577)Uout= 0.577;
	if(Uout<-0.577)Uout=-0.577;
	
	sector = (angle_el / _PI_3) + 1;
	T1 = _SQRT3*_sin(sector*_PI_3 - angle_el) * Uout;
	T2 = _SQRT3*_sin(angle_el - (sector-1.0)*_PI_3) * Uout;
	T0 = 1 - T1 - T2;
	
	// calculate the duty cycles(times)
	switch(sector)
	{
		case 1:
			Ta = T1 + T2 + T0/2;
			Tb = T2 + T0/2;
			Tc = T0/2;
			break;
		case 2:
			Ta = T1 +  T0/2;
			Tb = T1 + T2 + T0/2;
			Tc = T0/2;
			break;
		case 3:
			Ta = T0/2;
			Tb = T1 + T2 + T0/2;
			Tc = T2 + T0/2;
			break;
		case 4:
			Ta = T0/2;
			Tb = T1+ T0/2;
			Tc = T1 + T2 + T0/2;
			break;
		case 5:
			Ta = T2 + T0/2;
			Tb = T0/2;
			Tc = T1 + T2 + T0/2;
			break;
		case 6:
			Ta = T1 + T2 + T0/2;
			Tb = T0/2;
			Tc = T1 + T0/2;
			break;
		default:  // possible error state
			Ta = 0;
			Tb = 0;
			Tc = 0;
	}
	
	TIM_SetCompare1(TIM2,Ta*PWM_Period);
	TIM_SetCompare2(TIM2,Tb*PWM_Period);
	TIM_SetCompare3(TIM2,Tc*PWM_Period);
}
/******************************************************************************/
float velocityOpenloop(float target_velocity)
{
	unsigned long now_us;
	float Ts,Uq;
	
	now_us = SysTick->VAL; //_micros();
	if(now_us<open_loop_timestamp)Ts = (float)(open_loop_timestamp - now_us)/9*1e-6;
	else
		Ts = (float)(0xFFFFFF - now_us + open_loop_timestamp)/9*1e-6;
	open_loop_timestamp=now_us;  //save timestamp for next call
  // quick fix for strange cases (micros overflow)
  if(Ts == 0 || Ts > 0.5) Ts = 1e-3; 
	
	// calculate the necessary angle to achieve target velocity
  shaft_angle = _normalizeAngle(shaft_angle + target_velocity*Ts); 
	
	Uq = voltage_limit;
	// set the maximal allowed voltage (voltage_limit) with the necessary angle
  setPhaseVoltage(Uq,  0, _electricalAngle(shaft_angle, pole_pairs));
	
	return Uq;
}
/******************************************************************************/
float angleOpenloop(float target_angle)
{
	unsigned long now_us;
	float Ts,Uq;
	
	now_us = SysTick->VAL; //_micros();
	if(now_us<open_loop_timestamp)Ts = (float)(open_loop_timestamp - now_us)/9*1e-6;
	else
		Ts = (float)(0xFFFFFF - now_us + open_loop_timestamp)/9*1e-6;
  open_loop_timestamp = now_us;  //save timestamp for next call
  // quick fix for strange cases (micros overflow)
  if(Ts == 0 || Ts > 0.5) Ts = 1e-3; 
	
	// calculate the necessary angle to move from current position towards target angle
  // with maximal velocity (velocity_limit)
  if(fabs( target_angle - shaft_angle ) > velocity_limit*Ts)
	{
    shaft_angle += _sign(target_angle - shaft_angle) * velocity_limit * Ts;
    //shaft_velocity = velocity_limit;
  }
	else
	{
    shaft_angle = target_angle;
    //shaft_velocity = 0;
  }
	
	Uq = voltage_limit;
	// set the maximal allowed voltage (voltage_limit) with the necessary angle
	setPhaseVoltage(Uq,  0, _electricalAngle(shaft_angle, pole_pairs));
	
  return Uq;
}
/******************************************************************************/



