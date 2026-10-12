
#include "iic.h"


/******************************************************************************/
static void I2C_BusDelay(void)
{
	volatile unsigned long count;

	for(count = 0; count < 80UL; count++) __nop();
}

static void I2C_BusClear(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	unsigned char pulse;

	GPIO_SetBits(GPIOB, GPIO_Pin_6 | GPIO_Pin_7);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	I2C_BusDelay();

	for(pulse = 0; pulse < 9U &&
	    GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_7) == Bit_RESET; pulse++)
	{
		GPIO_ResetBits(GPIOB, GPIO_Pin_6);
		I2C_BusDelay();
		GPIO_SetBits(GPIOB, GPIO_Pin_6);
		I2C_BusDelay();
	}

	GPIO_ResetBits(GPIOB, GPIO_Pin_7);
	I2C_BusDelay();
	GPIO_SetBits(GPIOB, GPIO_Pin_6);
	I2C_BusDelay();
	GPIO_SetBits(GPIOB, GPIO_Pin_7);
	I2C_BusDelay();
}

void I2C_Init_(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	I2C_InitTypeDef I2C_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);//GPIOB
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
	I2C_DeInit(I2C1);
	I2C_BusClear();
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
	I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
	I2C_InitStructure.I2C_OwnAddress1 = 0;     //I2C_OAR1
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStructure.I2C_ClockSpeed = 400000;    //400KHz
	I2C_Init(I2C1, &I2C_InitStructure);
	I2C_Cmd(I2C1, ENABLE);
}
/******************************************************************************/


