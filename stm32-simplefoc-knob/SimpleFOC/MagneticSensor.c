

#include "MyProject.h"


/************************************************
本程序仅供学习，引用代码请标明出处
使用教程：https://blog.csdn.net/loop222/article/details/120471390
创建日期：20210925
作    者：loop222 @郑州
************************************************/
/******************************************************************************/
long  cpr;
float full_rotation_offset;
long  angle_data_prev;
unsigned long velocity_calc_timestamp;
float angle_prev;
volatile unsigned char magnetic_sensor_fault;
volatile unsigned char magnetic_sensor_status;
volatile unsigned char magnetic_field_warning;
/******************************************************************************/

/******************************************************************************/
#define  AS5600_Address  0x36
#define  AS5600_Status   0x0B
#define  RAW_Angle_Hi    0x0C   //V2.1.1 bugfix
//#define  RAW_Angle_Lo    0x0D
#define  AS5600_CPR      4096
#define  AS5600_STATUS_MD 0x20
#define  AS5600_STATUS_ML 0x10
#define  AS5600_STATUS_MH 0x08
#define  AS5600_I2C_TIMEOUT_MS    2U
#define  AS5600_I2C_TIMEOUT_LOOPS 50000UL
#define  AS5600_I2C_ERRORS (I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_AF | \
                            I2C_SR1_OVR | I2C_SR1_TIMEOUT)
/******************************************************************************/
static int AS5600_WaitSR1(I2C_TypeDef *I2Cx, uint16_t flag)
{
	uint32_t start_ms;
	uint32_t loops;
	uint16_t sr1;

	start_ms = motor_uptime_ms;
	loops = AS5600_I2C_TIMEOUT_LOOPS;
	for(;;)
	{
		sr1 = (uint16_t)I2Cx->SR1;
		if((sr1 & AS5600_I2C_ERRORS) != 0U) return 0;
		if((sr1 & flag) == flag) return 1;
		if((uint32_t)(motor_uptime_ms - start_ms) >= AS5600_I2C_TIMEOUT_MS)
			return 0;
		if(loops-- == 0U) return 0;
	}
}

static int AS5600_WaitStopClear(I2C_TypeDef *I2Cx)

{
	uint32_t start_ms;
	uint32_t loops;
	uint16_t sr1;

	start_ms = motor_uptime_ms;
	loops = AS5600_I2C_TIMEOUT_LOOPS;
	for(;;)
	{
		sr1 = (uint16_t)I2Cx->SR1;
		if((sr1 & AS5600_I2C_ERRORS) != 0U) return 0;
		if((I2Cx->CR1 & I2C_CR1_STOP) == 0U) return 1;
		if((uint32_t)(motor_uptime_ms - start_ms) >= AS5600_I2C_TIMEOUT_MS)
			return 0;
		if(loops-- == 0U) return 0;
	}
}

static unsigned short AS5600_I2CFail(I2C_TypeDef *I2Cx)

{
	Motor_disable();
	magnetic_sensor_fault = 1;
	I2Cx->CR1 |= I2C_CR1_ACK;
	I2Cx->CR1 &= (uint16_t)~I2C_CR1_POS;
	I2C_GenerateSTOP(I2Cx, ENABLE);
	I2C_Init_();
	return (unsigned short)angle_data_prev & 0x0FFFU;
}

static int AS5600_ReadRegister8(I2C_TypeDef *I2Cx, unsigned char reg,
	                             unsigned char *value)

{
	uint32_t primask;
	unsigned short temp;

	if(magnetic_sensor_fault) return 0;
	I2Cx->CR1 |= I2C_CR1_ACK;
	I2Cx->CR1 &= (uint16_t)~I2C_CR1_POS;
	I2Cx->CR1 |= I2C_CR1_START;
	if(!AS5600_WaitSR1(I2Cx, I2C_SR1_SB))
	{
		AS5600_I2CFail(I2Cx);
		return 0;
	}
	I2Cx->DR = AS5600_Address << 1;
	if(!AS5600_WaitSR1(I2Cx, I2C_SR1_ADDR))
	{
		AS5600_I2CFail(I2Cx);
		return 0;
	}
	temp = (unsigned short)I2Cx->SR2;
	I2Cx->DR = reg;
	if(!AS5600_WaitSR1(I2Cx, I2C_SR1_BTF))
	{
		AS5600_I2CFail(I2Cx);
		return 0;
	}

	I2Cx->CR1 |= I2C_CR1_START;
	if(!AS5600_WaitSR1(I2Cx, I2C_SR1_SB))
	{
		AS5600_I2CFail(I2Cx);
		return 0;
	}
	I2Cx->DR = (AS5600_Address << 1) + 1;
	if(!AS5600_WaitSR1(I2Cx, I2C_SR1_ADDR))
	{
		AS5600_I2CFail(I2Cx);
		return 0;
	}

	primask = __get_PRIMASK();
	__disable_irq();
	I2Cx->CR1 &= (uint16_t)~I2C_CR1_ACK;
	temp = (unsigned short)I2Cx->SR2;
	I2C_GenerateSTOP(I2Cx, ENABLE);
	if(primask == 0U) __enable_irq();

	if(!AS5600_WaitSR1(I2Cx, I2C_SR1_RXNE))
	{
		AS5600_I2CFail(I2Cx);
		return 0;
	}
	*value = (unsigned char)I2Cx->DR;
	if(!AS5600_WaitStopClear(I2Cx))
	{
		AS5600_I2CFail(I2Cx);
		return 0;
	}
	I2Cx->CR1 |= I2C_CR1_ACK;
	(void)temp;
	return 1;
}

unsigned short I2C_getRawCount(I2C_TypeDef *I2Cx)

{
	uint32_t primask;
	unsigned short temp;
	unsigned char dh;
	unsigned char dl;

	if(magnetic_sensor_fault)
		return (unsigned short)angle_data_prev & 0x0FFFU;

	I2Cx->CR1 |= I2C_CR1_ACK;
	I2Cx->CR1 &= (uint16_t)~I2C_CR1_POS;
	I2Cx->CR1 |= I2C_CR1_START;
	if(!AS5600_WaitSR1(I2Cx, I2C_SR1_SB))
		return AS5600_I2CFail(I2Cx);
	I2Cx->DR = AS5600_Address << 1;
	if(!AS5600_WaitSR1(I2Cx, I2C_SR1_ADDR))
		return AS5600_I2CFail(I2Cx);
	temp = (unsigned short)I2Cx->SR2;
	I2Cx->DR = RAW_Angle_Hi;
	if(!AS5600_WaitSR1(I2Cx, I2C_SR1_BTF))
		return AS5600_I2CFail(I2Cx);

	I2Cx->CR1 |= I2C_CR1_POS;
	I2Cx->CR1 |= I2C_CR1_START;
	if(!AS5600_WaitSR1(I2Cx, I2C_SR1_SB))
		return AS5600_I2CFail(I2Cx);
	I2Cx->DR = (AS5600_Address << 1) + 1;
	if(!AS5600_WaitSR1(I2Cx, I2C_SR1_ADDR))
		return AS5600_I2CFail(I2Cx);

	primask = __get_PRIMASK();
	__disable_irq();
	temp = (unsigned short)I2Cx->SR2;
	I2Cx->CR1 &= (uint16_t)~I2C_CR1_ACK;
	if(primask == 0U) __enable_irq();
	if(!AS5600_WaitSR1(I2Cx, I2C_SR1_BTF))
		return AS5600_I2CFail(I2Cx);

	primask = __get_PRIMASK();
	__disable_irq();
	I2C_GenerateSTOP(I2Cx, ENABLE);
	dh = (unsigned char)I2Cx->DR;
	if(primask == 0U) __enable_irq();
	dl = (unsigned char)I2Cx->DR;

	if(!AS5600_WaitStopClear(I2Cx))
		return AS5600_I2CFail(I2Cx);
	I2Cx->CR1 |= I2C_CR1_ACK;
	I2Cx->CR1 &= (uint16_t)~I2C_CR1_POS;
	(void)temp;
	return (unsigned short)(((unsigned short)dh << 8) | dl) & 0x0FFFU;
}
/******************************************************************************/

int MagneticSensor_CheckHealth(void)

{
	unsigned char status;

#if M1_AS5600
	if(!AS5600_ReadRegister8(I2C1, AS5600_Status, &status)) return 0;
	magnetic_sensor_status = status;
	magnetic_field_warning =
		(status & (AS5600_STATUS_ML | AS5600_STATUS_MH)) != 0U;
	if((status & AS5600_STATUS_MD) == 0U)
	{
		Motor_disable();
		magnetic_sensor_fault = 1;
		return 0;
	}
#else
	status = 0;
	(void)status;
#endif
	return 1;
}




/******************************************************************************/
//TLE5012B
#define READ_ANGLE_VALUE  0x8020
#define TLE5012B_CPR      32768
/******************************************************************************/
#define SPI2_CS1_L  GPIO_ResetBits(GPIOB, GPIO_Pin_8)      //CS1_L
#define SPI2_CS1_H  GPIO_SetBits(GPIOB, GPIO_Pin_8)        //CS1_H

#define SPI2_TX_OFF {GPIOB->CRH&=0x0FFFFFFF;GPIOB->CRH|=0x40000000;}  //把PB15(MOSI)配置为浮空输入模式
#define SPI2_TX_ON  {GPIOB->CRH&=0x0FFFFFFF;GPIOB->CRH|=0xB0000000;}  //把PB15(MOSI)复用推挽输出(50MHz)
/******************************************************************************/
unsigned short SPIx_ReadWriteByte(unsigned short byte)
{
	unsigned short retry = 0;
	
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET)
	{
		if(++retry>200)return 0;
	}
	SPI_I2S_SendData(SPI2, byte);
	retry = 0;
	while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET) 
	{
		if(++retry>200)return 0;
	}
	return SPI_I2S_ReceiveData(SPI2);
}
/******************************************************************************/
unsigned short ReadTLE5012B_1(unsigned short Comm)
{
	unsigned short u16Data;
	
	SPI2_CS1_L;
	SPIx_ReadWriteByte(Comm);
	SPI2_TX_OFF;
	__nop();__nop();__nop();__nop();__nop();__nop();__nop();__nop();  //Twr_delay=130ns min
	u16Data = SPIx_ReadWriteByte(0xffff);
	
	SPI2_CS1_H;
	SPI2_TX_ON;
	return(u16Data);
}
/******************************************************************************/
//#endif

/******************************************************************************/
void MagneticSensor_Init(void)
{
	magnetic_sensor_fault = 0;
	magnetic_sensor_status = 0;
	magnetic_field_warning = 0;
	full_rotation_offset = 0;
	velocity_calc_timestamp = 0;
#if M1_AS5600
	cpr=AS5600_CPR;
	angle_data_prev = I2C_getRawCount(I2C1);
	if(magnetic_sensor_fault || !MagneticSensor_CheckHealth()) return;
#elif M1_TLE5012B
	cpr=TLE5012B_CPR;
	angle_data_prev = ReadTLE5012B_1(READ_ANGLE_VALUE)&0x7FFF;
#endif
}
/******************************************************************************/
float getAngle(void)
{
	float angle_data,d_angle;
	
#if M1_AS5600
	angle_data = I2C_getRawCount(I2C1);
#elif M1_TLE5012B
	angle_data = ReadTLE5012B_1(READ_ANGLE_VALUE)&0x7FFF;
#endif
	if(magnetic_sensor_fault)
		return full_rotation_offset + ((float)angle_data_prev / (float)cpr) * _2PI;
	
	// tracking the number of rotations 
	// in order to expand angle range form [0,2PI] to basically infinity
	d_angle = angle_data - angle_data_prev;
	// if overflow happened track it as full rotation
	if(fabs(d_angle) > (0.8*cpr) ) full_rotation_offset += d_angle > 0 ? -_2PI : _2PI; 
	// save the current angle value for the next steps
	// in order to know if overflow happened
	angle_data_prev = angle_data;
	// return the full angle 
	// (number of full rotations)*2PI + current sensor angle 
	return  (full_rotation_offset + ( angle_data / (float)cpr) * _2PI) ;
}
/******************************************************************************/
void MagneticSensor_ResetVelocity(void)
{
	float current_angle;

	current_angle = getAngle();
	if(!magnetic_sensor_fault) angle_prev = current_angle;
	velocity_calc_timestamp = SysTick->VAL;
}
/******************************************************************************/
// Shaft velocity calculation
float getVelocity(void)
{
	unsigned long now_us;
	float Ts, angle_c, vel;

	angle_c = getAngle();
	now_us = SysTick->VAL;
	if(magnetic_sensor_fault)
	{
		angle_prev = angle_c;
		velocity_calc_timestamp = now_us;
		return 0.0f;
	}

	if(now_us<velocity_calc_timestamp)Ts = (float)(velocity_calc_timestamp - now_us)/9*1e-6;
	else
		Ts = (float)(0xFFFFFF - now_us + velocity_calc_timestamp)/9*1e-6;
	if(Ts <= 0.0f || Ts > 0.05f)
	{
		angle_prev = angle_c;
		velocity_calc_timestamp = now_us;
		return 0.0f;
	}

	vel = (angle_c - angle_prev)/Ts;

	// save variables for future pass
	angle_prev = angle_c;
	velocity_calc_timestamp = now_us;
	return vel;
}
/******************************************************************************/



