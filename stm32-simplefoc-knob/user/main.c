#include "stm32f10x.h"
#include "MyProject.h"

#define SPEED_SAMPLE_PERIOD_MS  1U
#define TELEMETRY_PERIOD_MS     10U
#define RAD_S_TO_RPM            9.5492966f
#define RPM_TO_RAD_S            0.10471976f
#define MOTOR_SUPPLY_VOLTS      12.0f
#define TORQUE_VOLTAGE_LIMIT    9.9f//0.23
#define FOC_ALIGN_VOLTAGE_LIMIT 6.0f
#define MOTOR_ALIGN_VOLTS       2.5f
#define MOTOR_POLE_PAIRS        7

#define DETENT_STEP_RAD          0.17453293f
#define DETENT_HYSTERESIS_RAD    0.00872665f
#define DETENT_POSITION_KP       6.0f
#define DETENT_DAMPING_KD        0.02f
#define DETENT_VOLTAGE_LIMIT     9.8f

#define SPIN_ENTER_RPM           100.0f
#define SPIN_ENTER_CONFIRM_MS    10U
#define SPIN_EXIT_RPM            20.0f
#define SPIN_EXIT_CONFIRM_MS     50U
#define SPIN_ENTRY_RAMP_MS       50U
#define SPIN_MONITOR_DELAY_MS   3000U
#define SPIN_TREND_WINDOW_MS    1000U
#define SPIN_MIN_DROP_RPM       5.0f
#define SPIN_FADE_TIME_MS       20000U

typedef enum
{
	KNOB_DETENT = 0,
	KNOB_SPIN
} KnobMode;

typedef enum
{
	SPIN_ENTRY_RAMP = 0,
	SPIN_MONITOR_DELAY,
	SPIN_MONITOR_TREND,
	SPIN_FADE,
	SPIN_WAIT_STOP
} SpinState;

/* Start with the power stage disabled; Motor_init() enables it for FOC alignment. */
static void Motor_EnablePin_Init(void)
{
	GPIO_InitTypeDef gpio;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	gpio.GPIO_Pin = GPIO_Pin_9;
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_Init(GPIOB, &gpio);
	GPIO_ResetBits(GPIOB, GPIO_Pin_9);
}

/* Runtime values and voltage-torque gain, visible in the debugger. */
volatile float motor_speed_rpm = 0.0f;
volatile float uq_gain_volts_per_rpm = 0.0032f;
volatile float uq_target_volts = 0.0f;

static KnobMode knob_mode = KNOB_DETENT;
static SpinState spin_state = SPIN_ENTRY_RAMP;
static uint32_t spin_state_start_ms;
static float spin_trend_start_rpm;
static uint32_t mode_transition_start_ms;
static unsigned char mode_transition_pending;
static float detent_origin_angle_rad;
static float detent_target_angle_rad;

/* Required by the existing SimpleFOC module; this program writes voltage.q directly. */
float target = 0.0f;

static float DetentTarget_Nearest(float angle_rad)
{
	float detent_position;
	long detent_index;

	detent_position =
		(angle_rad - detent_origin_angle_rad) / DETENT_STEP_RAD;
	if(detent_position >= 0.0f)
		detent_index = (long)(detent_position + 0.5f);
	else
		detent_index = (long)(detent_position - 0.5f);

	return detent_origin_angle_rad + (float)detent_index * DETENT_STEP_RAD;
}

static float DetentControl_Update(float angle_rad, float speed_rpm)
{
	float switch_angle_rad;
	float angle_error_rad;
	float uq;

	switch_angle_rad = DETENT_STEP_RAD * 0.5f + DETENT_HYSTERESIS_RAD;
	while(angle_rad > detent_target_angle_rad + switch_angle_rad)
		detent_target_angle_rad += DETENT_STEP_RAD;
	while(angle_rad < detent_target_angle_rad - switch_angle_rad)
		detent_target_angle_rad -= DETENT_STEP_RAD;

	angle_error_rad = detent_target_angle_rad - angle_rad;
	uq = DETENT_POSITION_KP * angle_error_rad -
	     DETENT_DAMPING_KD * speed_rpm * RPM_TO_RAD_S;

	if(uq > DETENT_VOLTAGE_LIMIT) uq = DETENT_VOLTAGE_LIMIT;
	if(uq < -DETENT_VOLTAGE_LIMIT) uq = -DETENT_VOLTAGE_LIMIT;
	return uq;
}

static float SpinAssistScale_Update(float speed_rpm, uint32_t now)
{
	float abs_speed_rpm;
	float speed_drop_rpm;
	uint32_t elapsed_ms;

	abs_speed_rpm = speed_rpm >= 0.0f ? speed_rpm : -speed_rpm;
	elapsed_ms = (uint32_t)(now - spin_state_start_ms);

	switch(spin_state)
	{
		case SPIN_ENTRY_RAMP:
			if(elapsed_ms >= SPIN_ENTRY_RAMP_MS)
			{
				spin_state_start_ms = now;
				spin_state = SPIN_MONITOR_DELAY;
				return 1.0f;
			}
			return (float)elapsed_ms / (float)SPIN_ENTRY_RAMP_MS;

		case SPIN_MONITOR_DELAY:
			if(elapsed_ms >= SPIN_MONITOR_DELAY_MS)
			{
				spin_state_start_ms = now;
				spin_trend_start_rpm = abs_speed_rpm;
				spin_state = SPIN_MONITOR_TREND;
			}
			return 1.0f;

		case SPIN_MONITOR_TREND:
			if(elapsed_ms >= SPIN_TREND_WINDOW_MS)
			{
				speed_drop_rpm = spin_trend_start_rpm - abs_speed_rpm;
				spin_state_start_ms = now;
				spin_trend_start_rpm = abs_speed_rpm;

				if(speed_drop_rpm < SPIN_MIN_DROP_RPM)
					spin_state = SPIN_FADE;
			}
			return 1.0f;

		case SPIN_FADE:
			if(elapsed_ms >= SPIN_FADE_TIME_MS)
			{
				spin_state = SPIN_WAIT_STOP;
				return 0.0f;
			}
			return 1.0f - (float)elapsed_ms / (float)SPIN_FADE_TIME_MS;

		case SPIN_WAIT_STOP:
		default:
			return 0.0f;
	}
}

static float KnobControl_Update(float angle_rad, float speed_rpm, uint32_t now)
{
	float abs_speed_rpm;
	float assist_scale;

	abs_speed_rpm = speed_rpm >= 0.0f ? speed_rpm : -speed_rpm;

	if(knob_mode == KNOB_DETENT)
	{
		if(abs_speed_rpm >= SPIN_ENTER_RPM)
		{
			if(!mode_transition_pending)
			{
				mode_transition_pending = 1U;
				mode_transition_start_ms = now;
			}
			else if((uint32_t)(now - mode_transition_start_ms) >=
			        SPIN_ENTER_CONFIRM_MS)
			{
				mode_transition_pending = 0U;
				spin_state = SPIN_ENTRY_RAMP;
				spin_state_start_ms = now;
				knob_mode = KNOB_SPIN;
			}
		}
		else
		{
			mode_transition_pending = 0U;
		}

		if(knob_mode == KNOB_DETENT)
			return DetentControl_Update(angle_rad, speed_rpm);
	}

	if(abs_speed_rpm <= SPIN_EXIT_RPM)
	{
		if(!mode_transition_pending)
		{
			mode_transition_pending = 1U;
			mode_transition_start_ms = now;
		}
		else if((uint32_t)(now - mode_transition_start_ms) >=
		        SPIN_EXIT_CONFIRM_MS)
		{
			mode_transition_pending = 0U;
			detent_target_angle_rad = DetentTarget_Nearest(angle_rad);
			knob_mode = KNOB_DETENT;
			return DetentControl_Update(angle_rad, speed_rpm);
		}
	}
	else
	{
		mode_transition_pending = 0U;
	}

	assist_scale = SpinAssistScale_Update(speed_rpm, now);
	return uq_gain_volts_per_rpm * speed_rpm * assist_scale;
}

int main(void)
{
	uint32_t sample_tick;
	uint32_t telemetry_tick;
	uint32_t now;

	Motor_EnablePin_Init();
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	uart_init(115200);
	I2C_Init_();
	TIM2_PWM_Init();
	TIM4_1ms_Init();
	delay_ms(10);

	MagneticSensor_Init();
	if(magnetic_sensor_fault)
	{
		while(1)
		{
		}
	}
	voltage_power_supply = MOTOR_SUPPLY_VOLTS;
	voltage_limit = FOC_ALIGN_VOLTAGE_LIMIT;
	voltage_sensor_align = MOTOR_ALIGN_VOLTS;
	pole_pairs = MOTOR_POLE_PAIRS;
	torque_controller = Type_voltage;
	controller = Type_torque;
	voltage.q = 0.0f;
	voltage.d = 0.0f;

	Motor_init();
	if(!Motor_initFOC())
	{
		Motor_disable();
		while(1)
		{
		}
	}
	voltage_limit = TORQUE_VOLTAGE_LIMIT;

	/* SysTick provides the precise time interval used by getVelocity(). */
	systick_CountMode();
	MagneticSensor_ResetVelocity();
	LPF_velocity_reset(0.0f);
	shaft_angle = shaftAngle();
	detent_origin_angle_rad = shaft_angle;
	detent_target_angle_rad = shaft_angle;

	sample_tick = motor_uptime_ms;
	telemetry_tick = sample_tick;

	while(1)
	{
		now = motor_uptime_ms;

		if((uint32_t)(now - sample_tick) >= SPEED_SAMPLE_PERIOD_MS)
		{
			sample_tick = now;
			motor_speed_rpm =
				LPF_velocity(sensor_direction * getVelocity()) * RAD_S_TO_RPM;

			if(magnetic_sensor_fault)
			{
				motor_speed_rpm = 0.0f;
				uq_target_volts = 0.0f;
				Motor_disable();
			}
			else
			{
				uq_target_volts =
					KnobControl_Update(shaft_angle, motor_speed_rpm, now);
				if(uq_target_volts > voltage_limit)
					uq_target_volts = voltage_limit;
				if(uq_target_volts < -voltage_limit)
					uq_target_volts = -voltage_limit;

				voltage.q = uq_target_volts;
				voltage.d = 0.0f;
			}
		}

		if(!magnetic_sensor_fault) loopFOC();

		if((uint32_t)(now - telemetry_tick) >= TELEMETRY_PERIOD_MS)
		{
			telemetry_tick = now;
			printf("%.3f,%.3f\n", motor_speed_rpm, uq_target_volts);
		}
	}
}
