/* GCC startup for STM32F103C8 (Cortex-M3, medium density). */
.syntax unified
.cpu cortex-m3
.thumb
.extern SystemInit
.extern main
.section .isr_vector, "a", %progbits
.align 2
.global g_pfnVectors
g_pfnVectors:
  .word _estack
  .word Reset_Handler
  .word NMI_Handler
  .word HardFault_Handler
  .word MemManage_Handler
  .word BusFault_Handler
  .word UsageFault_Handler
  .word 0, 0, 0, 0
  .word SVC_Handler
  .word DebugMon_Handler
  .word 0
  .word PendSV_Handler
  .word SysTick_Handler
  .word WWDG_IRQHandler, PVD_IRQHandler, TAMPER_IRQHandler, RTC_IRQHandler
  .word FLASH_IRQHandler, RCC_IRQHandler
  .word EXTI0_IRQHandler, EXTI1_IRQHandler, EXTI2_IRQHandler, EXTI3_IRQHandler, EXTI4_IRQHandler
  .word DMA1_Channel1_IRQHandler, DMA1_Channel2_IRQHandler, DMA1_Channel3_IRQHandler
  .word DMA1_Channel4_IRQHandler, DMA1_Channel5_IRQHandler, DMA1_Channel6_IRQHandler, DMA1_Channel7_IRQHandler
  .word ADC1_2_IRQHandler, USB_HP_CAN1_TX_IRQHandler, USB_LP_CAN1_RX0_IRQHandler
  .word CAN1_RX1_IRQHandler, CAN1_SCE_IRQHandler, EXTI9_5_IRQHandler
  .word TIM1_BRK_IRQHandler, TIM1_UP_IRQHandler, TIM1_TRG_COM_IRQHandler, TIM1_CC_IRQHandler
  .word TIM2_IRQHandler, TIM3_IRQHandler, TIM4_IRQHandler
  .word I2C1_EV_IRQHandler, I2C1_ER_IRQHandler, I2C2_EV_IRQHandler, I2C2_ER_IRQHandler
  .word SPI1_IRQHandler, SPI2_IRQHandler, USART1_IRQHandler, USART2_IRQHandler, USART3_IRQHandler
  .word EXTI15_10_IRQHandler, RTCAlarm_IRQHandler, USBWakeUp_IRQHandler
.section .text.Reset_Handler, "ax", %progbits
.align 2
.global Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
  ldr r0, =_sidata
  ldr r1, =_sdata
  ldr r2, =_edata
1: cmp r1, r2
   bcc 2f
   b 3f
2: ldr r3, [r0]
   str r3, [r1]
   adds r0, r0, #4
   adds r1, r1, #4
   b 1b
3: ldr r1, =_sbss
   ldr r2, =_ebss
   movs r3, #0
4: cmp r1, r2
   bcc 5f
   b 6f
5: str r3, [r1]
   adds r1, r1, #4
   b 4b
6: bl SystemInit
   bl main
7: b 7b
.section .text.Default_Handler, "ax", %progbits
.align 2
.weak NMI_Handler, HardFault_Handler, MemManage_Handler, BusFault_Handler, UsageFault_Handler
.weak SVC_Handler, DebugMon_Handler, PendSV_Handler, SysTick_Handler
.weak WWDG_IRQHandler, PVD_IRQHandler, TAMPER_IRQHandler, RTC_IRQHandler, FLASH_IRQHandler, RCC_IRQHandler
.weak EXTI0_IRQHandler, EXTI1_IRQHandler, EXTI2_IRQHandler, EXTI3_IRQHandler, EXTI4_IRQHandler
.weak DMA1_Channel1_IRQHandler, DMA1_Channel2_IRQHandler, DMA1_Channel3_IRQHandler
.weak DMA1_Channel4_IRQHandler, DMA1_Channel5_IRQHandler, DMA1_Channel6_IRQHandler, DMA1_Channel7_IRQHandler
.weak ADC1_2_IRQHandler, USB_HP_CAN1_TX_IRQHandler, USB_LP_CAN1_RX0_IRQHandler
.weak CAN1_RX1_IRQHandler, CAN1_SCE_IRQHandler, EXTI9_5_IRQHandler
.weak TIM1_BRK_IRQHandler, TIM1_UP_IRQHandler, TIM1_TRG_COM_IRQHandler, TIM1_CC_IRQHandler
.weak TIM2_IRQHandler, TIM3_IRQHandler, TIM4_IRQHandler
.weak I2C1_EV_IRQHandler, I2C1_ER_IRQHandler, I2C2_EV_IRQHandler, I2C2_ER_IRQHandler
.weak SPI1_IRQHandler, SPI2_IRQHandler, USART1_IRQHandler, USART2_IRQHandler, USART3_IRQHandler
.weak EXTI15_10_IRQHandler, RTCAlarm_IRQHandler, USBWakeUp_IRQHandler
.thumb_set NMI_Handler, Default_Handler
.thumb_set HardFault_Handler, Default_Handler
.thumb_set MemManage_Handler, Default_Handler
.thumb_set BusFault_Handler, Default_Handler
.thumb_set UsageFault_Handler, Default_Handler
.thumb_set SVC_Handler, Default_Handler
.thumb_set DebugMon_Handler, Default_Handler
.thumb_set PendSV_Handler, Default_Handler
.thumb_set SysTick_Handler, Default_Handler
.thumb_set WWDG_IRQHandler, Default_Handler
.thumb_set PVD_IRQHandler, Default_Handler
.thumb_set TAMPER_IRQHandler, Default_Handler
.thumb_set RTC_IRQHandler, Default_Handler
.thumb_set FLASH_IRQHandler, Default_Handler
.thumb_set RCC_IRQHandler, Default_Handler
.thumb_set EXTI0_IRQHandler, Default_Handler
.thumb_set EXTI1_IRQHandler, Default_Handler
.thumb_set EXTI2_IRQHandler, Default_Handler
.thumb_set EXTI3_IRQHandler, Default_Handler
.thumb_set EXTI4_IRQHandler, Default_Handler
.thumb_set DMA1_Channel1_IRQHandler, Default_Handler
.thumb_set DMA1_Channel2_IRQHandler, Default_Handler
.thumb_set DMA1_Channel3_IRQHandler, Default_Handler
.thumb_set DMA1_Channel4_IRQHandler, Default_Handler
.thumb_set DMA1_Channel5_IRQHandler, Default_Handler
.thumb_set DMA1_Channel6_IRQHandler, Default_Handler
.thumb_set DMA1_Channel7_IRQHandler, Default_Handler
.thumb_set ADC1_2_IRQHandler, Default_Handler
.thumb_set USB_HP_CAN1_TX_IRQHandler, Default_Handler
.thumb_set USB_LP_CAN1_RX0_IRQHandler, Default_Handler
.thumb_set CAN1_RX1_IRQHandler, Default_Handler
.thumb_set CAN1_SCE_IRQHandler, Default_Handler
.thumb_set EXTI9_5_IRQHandler, Default_Handler
.thumb_set TIM1_BRK_IRQHandler, Default_Handler
.thumb_set TIM1_UP_IRQHandler, Default_Handler
.thumb_set TIM1_TRG_COM_IRQHandler, Default_Handler
.thumb_set TIM1_CC_IRQHandler, Default_Handler
.thumb_set TIM2_IRQHandler, Default_Handler
.thumb_set TIM3_IRQHandler, Default_Handler
.thumb_set TIM4_IRQHandler, Default_Handler
.thumb_set I2C1_EV_IRQHandler, Default_Handler
.thumb_set I2C1_ER_IRQHandler, Default_Handler
.thumb_set I2C2_EV_IRQHandler, Default_Handler
.thumb_set I2C2_ER_IRQHandler, Default_Handler
.thumb_set SPI1_IRQHandler, Default_Handler
.thumb_set SPI2_IRQHandler, Default_Handler
.thumb_set USART1_IRQHandler, Default_Handler
.thumb_set USART2_IRQHandler, Default_Handler
.thumb_set USART3_IRQHandler, Default_Handler
.thumb_set EXTI15_10_IRQHandler, Default_Handler
.thumb_set RTCAlarm_IRQHandler, Default_Handler
.thumb_set USBWakeUp_IRQHandler, Default_Handler
.global Default_Handler
.type Default_Handler, %function
Default_Handler: b .
