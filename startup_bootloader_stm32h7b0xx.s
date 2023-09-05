.syntax unified
.cpu cortex-m7
.fpu softvfp
.thumb

.global  g_pfnVectors_bootloader

.word __payload_start__
.word __payload_end__

.section  .text.Reset_Handler_bootloader
.weak  Reset_Handler_bootloader
.type  Reset_Handler_bootloader, %function
Reset_Handler_bootloader:  
/* Copy the application payload from flash to SRAM */
  movs  r1, #0
  b  LoopCopyPayload

CopyPayload:
  movs  r4, #0x24000000
  ldr  r3, [r0, r1]
  str  r3, [r4, r1]
  adds  r1, r1, #4

LoopCopyPayload:
  ldr  r0, =__payload_start__
  ldr  r3, =__payload_end__
  adds  r2, r0, r1
  cmp  r2, r3
  bcc  CopyPayload

/* Hand over to payload code */
  movs r3, #0x24000000
  ldr  r1, [r3]
  add  r3, #4
  ldr  r0, [r3]
  msr msp, r1 /* load r1 into MSP */
  bx r0       /* branch to the address at r0 */

  .size  Reset_Handler_bootloader, .-Reset_Handler_bootloader

/******************************************************************************
*
* The minimal vector table for a Cortex M. Note that the proper constructs
* must be placed on this to ensure that it ends up at physical address
* 0x0000.0000.
* 
*******************************************************************************/
   .section  .isr_vector_bootloader,"a",%progbits
  .type  g_pfnVectors_bootloader, %object
  .size  g_pfnVectors_bootloader, .-g_pfnVectors_bootloader


g_pfnVectors_bootloader:
  .word  _estack
  .word  Reset_Handler_bootloader

  .word  0
  .word  0
  .word  0
  .word  0
  .word  0
  .word  0
  .word  0
  .word  0
  .word  0
  .word  0
  .word  0
  .word  0
  .word  0
  .word  0