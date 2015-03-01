/* ###################################################################
**     Filename    : main.c
**     Project     : IOM_C
**     Processor   : MC9S08PA4VTJ
**     Version     : Driver 01.12
**     Compiler    : CodeWarrior HCS08 C Compiler
**     Date/Time   : 2015-01-31, 11:51, # CodeGen: 0
**     Abstract    :
**         Main module.
**         This module contains user's application code.
**     Settings    :
**     Contents    :
**         No public methods
**
** ###################################################################*/
/*!
** @file main.c
** @version 01.12
** @brief
**         Main module.
**         This module contains user's application code.
*/         
/*!
**  @addtogroup main_module main module documentation
**  @{
*/         
/* MODULE main */


/* Including needed modules to compile this module/procedure */
#include "Cpu.h"
#include "Events.h"
#include "AS1.h"
#include "Din.h"
#include "Din.h"
#include "Dout.h"
#include "Timer.h"
#include "AD1.h"
/* Include shared modules, which are used for whole project */
#include "PE_Types.h"
#include "PE_Error.h"
#include "PE_Const.h"
#include "IO_Map.h"

/* User includes (#include below this line is not maintained by Processor Expert) */

uint8_t UART_TXBuf[32]={0};
uint8_t UART_RXBuf[32]={0};
uint8_t TXData[20]={0};

uint8_t AD_Result[8]={0};
uint16_t AD_Last_Val[4]={0};
uint16_t AD_Val[4]={0};
uint8_t Din_Last_Val[2]={0};

uint8_t Self_Short_Addr[2]={0};
uint8_t MAC_Addr[8]={0};
uint8_t Device_Num[2]={0};
uint8_t UT_SN=0;
uint8_t Compare_Flag=0;

uint16_t MainCycle=0;
uint8_t CheckTime=0;

const uint8_t Read_Short_Addr[7]={0xFC, 0x00, 0x91, 0x04, 0xC4, 0xD4, 0x29};
const uint8_t Read_Dev_Command[7]={0xFC, 0x00, 0x91, 0x6A, 0xBA, 0xDA, 0x8B};
const uint8_t Read_Mac_Command[7]={0xFC, 0x00, 0x91, 0x08, 0xA8, 0xB8, 0xF5};

void TX_Report(uint8_t * Data, uint8_t Count);
uint8_t RX_Frame(void);
uint8_t Get_Device_Num(void);
uint8_t Get_Mac(void);
uint8_t Get_Short_Addr(void);
uint8_t RX_Poll(void);
void AD_Update(void);

#define Report_Cycle 500					//This defines periodical report interval. 500x7ms=3.5s.
#define Check_Interval 4					//This defines input change detection interval. 4x7ms=28ms.
#define AD_Change_Threshold 0x20			//This defines analog input change threshold to trigger reporting.

void main(void)
{
  /* Write your local variable definition here */

	uint8_t i, j;
	uint16_t tmp;
	
  /*** Processor Expert internal initialization. DON'T REMOVE THIS CODE!!! ***/
  PE_low_level_init();
  /*** End of Processor Expert internal initialization.                    ***/

  /* Write your code here */
  /* For example: for(;;) { } */
  
  (void) AS1_Disable();							//Disable UART.
  Cpu_Delay100US(15000);					//Delay 1.5s.
  Dout_PutVal(0);							//Clear all DO output to 0.
  (void) AS1_Enable();								//Enable UART.
  Cpu_Delay100US(100);						//Delay 10ms
  
  //Cpu_Delay100US(1000);						//Delay 100ms
  
  
  for(;;){
	  i=Get_Device_Num();
	  if(i==ERR_OK)break;
	  Cpu_Delay100US(1000);					//Get device number.
	  (void) AS1_ClearRxBuf();
  }
  
  Cpu_Delay100US(100);
  
  for(;;){
	  i=Get_Mac();
	  if(i==ERR_OK)break;
  	  Cpu_Delay100US(1000);					//Get MAC address.
  	  (void) AS1_ClearRxBuf();
  }

  Cpu_Delay100US(100);						//Delay 10ms

  for(;;){
	  while(Get_Short_Addr()!=ERR_OK);			//Get Wireless short address.
	  if(Self_Short_Addr[0]!=0xFF || Self_Short_Addr[0]!=0xFE)break;
	  Cpu_Delay100US(100);
	  (void) AS1_ClearRxBuf();
  }

  Cpu_Delay100US(50000);						//Delay 5s.
//  Cpu_Delay100US(25000);						//Delay 2.5s.
  
  //Start to send 5 times MAC_Addr + Device_Num report frame.
  for(j=0;j<5;j++){
	  for(i=0;i<8;i++){
		  TXData[i]=MAC_Addr[i];
	  }
	  TXData[i++]=Device_Num[0];
	  TXData[i]=Device_Num[1];
	  (void) AS1_SendBlock(TXData, 10, &tmp);
	  while(AS1_GetCharsInTxBuf()!=0);						//Wait until TX buffer empty.
	  Cpu_Delay100US(2000);									//Wait 200ms.
  }
  
  //Start main loop...
  
  for(;;){
	  
	  CheckTime=0;
	  while(RX_Frame()!=ERR_OK){
		  
		  MainCycle++;
		  CheckTime++;
		  
		  if(CheckTime==Check_Interval){
			  CheckTime=0;
			  
			  AD_Update();
			  
			  Compare_Flag=0;
			  //Compare AD data to see if big changes.
			  for(i=0;i<4;i++){
				  if(AD_Val[i]>AD_Last_Val[i]){
					  tmp=AD_Val[i]-AD_Last_Val[i];
				  }else{
					  tmp=AD_Last_Val[i]-AD_Val[i];
				  }
				  if(tmp > AD_Change_Threshold){
					  Compare_Flag=1;
					  break;
				  }
			  }
			  
			  if(Compare_Flag==1 || (bool)Din_Last_Val[0]!=Din_GetBit(0) || (bool)Din_Last_Val[1]!=Din_GetBit(1)){
				  // Value changed, need to report new value.
				  for(i=0;i<8;i++){
					  TXData[i]=AD_Result[i];						//Copy AD data to TX data.
				  }
				  for(i=0;i<4;i++){
					  AD_Last_Val[i]=AD_Val[i];						//Save current AD values.
				  }
				  for(i=0;i<2;i++){
					  TXData[i+8]=Din_GetBit(i);					//Get Din.
					  Din_Last_Val[i]=TXData[i+8];					//Save current Din values.
				  }
				  for(i=0;i<2;i++){
					  TXData[i+10]=Dout_GetBit(i);					//Get Dout.
				  }
				  TX_Report(TXData, 12);								//Send report frame to UART.
			  }
		  }
		  
		  if(MainCycle >= Report_Cycle){								//Check if Report cycle is reached.
			  MainCycle=0;
			  CheckTime=0;

			  AD_Update();
			  
			  for(i=0;i<8;i++){
				  TXData[i]=AD_Result[i];						//Copy AD data to TX data.
			  }
			  for(i=0;i<4;i++){
				  AD_Last_Val[i]=AD_Val[i];						//Save current AD values.
			  }
			  for(i=0;i<2;i++){
				  TXData[i+8]=Din_GetBit(i);					//Get Din.
				  Din_Last_Val[i]=TXData[i+8];					//Save current Din values.
			  }
			  for(i=0;i<2;i++){
				  TXData[i+10]=Dout_GetBit(i);					//Get Dout.
			  }
			  TX_Report(TXData, 12);								//Send report frame to UART.
		  }
	  }
	  
	  //Valid incoming frame got.
	  if(UART_RXBuf[9]==0x03){
		  //CMD 0x03
		  AD_Update();
		  for(i=0;i<8;i++){
			  TXData[i]=AD_Result[i];						//Copy AD data to TX data.
		  }
		  for(i=0;i<4;i++){
			  AD_Last_Val[i]=AD_Val[i];						//Save current AD values.
		  }
		  for(i=0;i<2;i++){
			  TXData[i+8]=Din_GetBit(i);					//Get Din.
			  Din_Last_Val[i]=TXData[i+8];					//Save current Din values.
		  }
		  for(i=0;i<2;i++){
			  TXData[i+10]=Dout_GetBit(i);					//Get Dout.
		  }
		  TX_Report(TXData, 12);								//Send report frame to UART.
	  }else if(UART_RXBuf[9]==0x33){
		  //CMD 0x33
		  for(i=0;i<2;i++){
			  Dout_PutBit(i, UART_RXBuf[11+i]);				//Control DO output.
		  }
		  AD_Update();
		  for(i=0;i<8;i++){
			  TXData[i]=AD_Result[i];						//Copy AD data to TX data.
		  }
		  for(i=0;i<4;i++){
			  AD_Last_Val[i]=AD_Val[i];						//Save current AD values.
		  }
		  for(i=0;i<2;i++){
			  TXData[i+8]=Din_GetBit(i);					//Get Din.
			  Din_Last_Val[i]=TXData[i+8];					//Save current Din values.
		  }
		  for(i=0;i<2;i++){
			  TXData[i+10]=Dout_GetBit(i);					//Get Dout.
		  }
		  TX_Report(TXData, 12);								//Send report frame to UART.
	  }else if(UART_RXBuf[9]==0xF0){
		  //CMD 0xF0
		  if(UART_RXBuf[11]==Device_Num[0] && UART_RXBuf[12]==Device_Num[1]){
			  for(i=0;i<8;i++){
				  TXData[i]=MAC_Addr[i];
			  }
			  TX_Report(TXData, 8);							//Send MAC address.
		  }
	  }else {
		  Cpu_Delay100US(10);
	  }
	  
  }
  
  /*** Don't write any code pass this line, or it will be deleted during code generation. ***/
  /*** RTOS startup code. Macro PEX_RTOS_START is defined by the RTOS component. DON'T MODIFY THIS CODE!!! ***/
  #ifdef PEX_RTOS_START
    PEX_RTOS_START();                  /* Startup of the selected RTOS. Macro is defined by the RTOS component. */
  #endif
  /*** End of RTOS startup code.  ***/
  /*** Processor Expert end of main routine. DON'T MODIFY THIS CODE!!! ***/
  for(;;){}
  /*** Processor Expert end of main routine. DON'T WRITE CODE BELOW!!! ***/
} /*** End of main routine. DO NOT MODIFY THIS TEXT!!! ***/

/* END main */

//***********************************************************************
// TX_Report function
// Outcoming frame structure:
//     0xFD -- 1 byte length (not include self and 0xFD and next 2 bytes) -
//     - 0x00, 0x00 (2 bytes Destination short address) -
//     - 2 bytes Self_Short_Addr -- 1 byte sequence number -- data bytes -- 1 byte checksum
//***********************************************************************
void TX_Report(uint8_t * Data, uint8_t Count)
{
	word SentBytes;
	uint8_t i,j;
	
	if(Count > 20)return;					//Maximum payload is limited to 20 bytes.
	
	UART_TXBuf[0]=0xFD;
	UART_TXBuf[1]=Count+4;
	UART_TXBuf[2]=0;
	UART_TXBuf[3]=0;
	UART_TXBuf[4]=Self_Short_Addr[0];
	UART_TXBuf[5]=Self_Short_Addr[1];
	UART_TXBuf[6]=UT_SN++;
	
	//Copy payload data to UART_TXBuf[];
	for(i=7, j=0; j<Count; i++, j++){
		UART_TXBuf[i] = Data[j];
	}
	
	//Calculate checksum;
	UART_TXBuf[i] = 0;						//Clear checksum byte.
	for(j=0; j<i; j++){
		UART_TXBuf[i]+=UART_TXBuf[j];
	}
	
	(void) AS1_SendBlock(UART_TXBuf, i+1, &SentBytes);
	
	return;
}

//***********************************************************************
// RX_Frame function
//     Receive command frame via UART
//     Check if it is broadcast (MAC= all 0xFF)
//     Check if MAC address matches 
//     Check if frame length too long
//     RX time-out check
//     Checksum verification
// Success condition:
//     Frame MAC is broadcast or match with local MAC address
//     Frame length OK
//     Checksum correct
// 
// Incoming frame structure:
// 4 byte header (will be ignored) -- 8 bytes MAC -- 1 byte length (not include MAC+self) -
// - 1 byte command -- 1 byte sequence number -- data bytes -- 1 byte checksum
//    
// Return: 
//		ERR_OK: Succeeded. Valid frame received.
//		ERR_EMPTY: Nothing received.
//		ERR_COMMON: Something received but not valid. Ignore.
//***********************************************************************
uint8_t RX_Frame(void)
{
	uint8_t tmp, i, j;
	
/*	//Check if any byte received. If nothing received, return.
	//Ignore first 4 bytes.
	if(AS1_RecvChar(&tmp) != ERR_OK)return ERR_RXEMPTY;
	for(i=0; i<3; i++){
		if(RX_Poll()==ERR_RXEMPTY)return ERR_COMMON;
		if(AS1_RecvChar(&tmp) != ERR_OK)return ERR_COMMON;
	}  */
	//Receive 8 bytes MAC address
	for(i=0; i<8; i++){
		if(RX_Poll()==ERR_RXEMPTY)return ERR_COMMON;
		if(AS1_RecvChar(&tmp) != ERR_OK)return ERR_COMMON;
		UART_RXBuf[i]=tmp;
	}
	//Compare MAC
	tmp=0;
	for(i=0; i<8; i++){
		if(UART_RXBuf[i]!=MAC_Addr[i]){
			tmp++;
		}
	}
	if(tmp!=0){
		for(i=0; i<8; i++){
			if(UART_RXBuf[i]!=0xFF)return ERR_COMMON;		//MAC compare failed, return error.
		}
	}
	
	//MAC compare OK now. Receive length byte.
	if(RX_Poll()==ERR_RXEMPTY)return ERR_COMMON;
	if(AS1_RecvChar(&tmp) != ERR_OK)return ERR_COMMON;
	if(tmp<4 || tmp>10)return ERR_COMMON;								//Length cannot <4 or >10.
	UART_RXBuf[8]=tmp;
	
	//Length OK, receive command byte.
	if(RX_Poll()==ERR_RXEMPTY)return ERR_COMMON;
	if(AS1_RecvChar(&tmp) != ERR_OK)return ERR_COMMON;
	UART_RXBuf[9]=tmp;
	
	//Command byte got, receive sequence number byte.
	if(RX_Poll()==ERR_RXEMPTY)return ERR_COMMON;
	if(AS1_RecvChar(&tmp) != ERR_OK)return ERR_COMMON;
	UART_RXBuf[10]=tmp;
	
	//Receive data payload bytes
	for(i=UART_RXBuf[8]-3, j=11; i>0; i--, j++){
		if(RX_Poll()==ERR_RXEMPTY)return ERR_COMMON;
		if(AS1_RecvChar(&tmp) != ERR_OK)return ERR_COMMON;
		UART_RXBuf[j]=tmp;
	}
	
	//Receive checksum byte
	if(RX_Poll()==ERR_RXEMPTY)return ERR_COMMON;
	if(AS1_RecvChar(&tmp) != ERR_OK)return ERR_COMMON;
	UART_RXBuf[j]=tmp;
	
	//Calculate & compare checksum
	tmp=0;
	for(i=0; i<j; i++){
		tmp+=UART_RXBuf[i];
	}
	if(tmp!=UART_RXBuf[j])return ERR_COMMON;						//Checksum error.
	
	return ERR_OK;
	
}

//***********************************************************************
// Check if any bytes received. ~100uS timeout.
//***********************************************************************
uint8_t RX_Poll(void)
{
	word tmp;
	
	Timer_Reset();
	tmp=0;

	//Check RX until 100uS timeout.
/*	while(tmp<7){
		if(AS1_GetCharsInRxBuf()!=0)return ERR_OK;
		Timer_GetCounterValue(&tmp);
	}*/
	
	while(tmp<70){
		if(AS1_GetCharsInRxBuf()!=0)return ERR_OK;
		Cpu_Delay100US(1);
		tmp++;
	}
	
	return ERR_RXEMPTY;
}


//***********************************************************************
// Subroutine Get_Mac
//     Send command to wireless module via UART
//     Receive 8 byte MAC address and store to MAC_Addr
// Return: ERR_OK, ERR_FAILED
//***********************************************************************
uint8_t Get_Mac(void)
{
	uint8_t i, j;
	word tmp;
	
	for(i=0; i<7; i++){
		UART_TXBuf[i]=Read_Mac_Command[i];
	}
	
	(void) AS1_SendBlock(UART_TXBuf, 7, &tmp);						//Send Read MAC command.
	
	while(AS1_GetCharsInTxBuf()!=0);						//Wait until TX buffer empty.
	
	for(i=0; i<8; i++){
		if(RX_Poll()==ERR_RXEMPTY)return ERR_FAILED;
		if(AS1_RecvChar(&j) != ERR_OK)return ERR_FAILED;
		MAC_Addr[i]=j;
	}
	
	while(RX_Poll()!=ERR_RXEMPTY){
		(void) AS1_RecvChar(&j);
	}
	
	return ERR_OK;
}


//***********************************************************************
// Subroutine Get_Device_Num
//     Send command to wireless module via UART
//     Receive 9 byte, store to UART_RXBuf
//     Device serial number is UART_RXBuf[5],[6]
//     Put UART_RXBuf[5],[6] to Device_Num
// Return: ERR_OK, ERR_FAILED
//***********************************************************************
uint8_t Get_Device_Num(void)
{
	uint8_t i, j;
	word tmp;
	
	for(i=0; i<7; i++){
		UART_TXBuf[i]=Read_Dev_Command[i];
	}
	
	(void) AS1_SendBlock(UART_TXBuf, 7, &tmp);						//Send Read Dev command.
	
	while(AS1_GetCharsInTxBuf()!=0);						//Wait until TX buffer empty.
	
	for(i=0; i<8; i++){
		if(RX_Poll()==ERR_RXEMPTY){
			return ERR_FAILED;
		}
		if(AS1_RecvChar(&j) != ERR_OK){
			return ERR_FAILED;
		}
		UART_RXBuf[i]=j;
	}
	
	Device_Num[0]=UART_RXBuf[5];
	Device_Num[1]=UART_RXBuf[6];
	
	while(RX_Poll()!=ERR_RXEMPTY){
		(void) AS1_RecvChar(&j);
	}
	
	return ERR_OK;
}


//***********************************************************************
// Subroutine Get_Short_Addr
//     Send command to wireless module via UART
//     Receive 2 byte, store to UART_RXBuf
//     Device serial number is UART_RXBuf[0],[1]
//     Put UART_RXBuf[0],[1] to Self_Short_Addr
// Return: ERR_OK, ERR_FAILED
//***********************************************************************
uint8_t Get_Short_Addr(void)
{
	uint8_t i, j;
	word tmp;
	
	for(i=0; i<7; i++){
		UART_TXBuf[i]=Read_Short_Addr[i];
	}
	
	(void) AS1_SendBlock(UART_TXBuf, 7, &tmp);						//Send Read Dev command.
	
	while(AS1_GetCharsInTxBuf()!=0);						//Wait until TX buffer empty.
	
	for(i=0; i<2; i++){
		if(RX_Poll()==ERR_RXEMPTY)return ERR_FAILED;
		if(AS1_RecvChar(&j) != ERR_OK)return ERR_FAILED;
		Self_Short_Addr[i]=j;
	}

	while(RX_Poll()!=ERR_RXEMPTY){
		(void) AS1_RecvChar(&j);
	}

	return ERR_OK;
}


//***********************************************************************
// Subroutine AD_Update
// Do AD convert on 4 channels and update result to AD_Val[] and AD_Result[].
//***********************************************************************
void AD_Update(void)
{
	uint8_t i;
	uint16_t tmp;
	
	  if(AD1_Measure(1)==ERR_OK){
		  (void) AD1_GetChanValue16(0, &tmp);
		  tmp=tmp>>6;
		  AD_Val[0]=tmp;
		  AD_Result[0]= (byte) (tmp>>8);
		  AD_Result[1]= (byte) (tmp&0x00FF);
		  (void) AD1_GetChanValue16(1, &tmp);
		  tmp=tmp>>6;
		  AD_Val[1]=tmp;
		  AD_Result[2]= (byte) (tmp>>8);
		  AD_Result[3]= (byte) (tmp&0x00FF);
		  (void) AD1_GetChanValue16(2, &tmp);
		  tmp=tmp>>6;
		  AD_Val[2]=tmp;
		  AD_Result[4]= (byte) (tmp>>8);
		  AD_Result[5]= (byte) (tmp&0x00FF);
		  (void) AD1_GetChanValue16(3, &tmp);
		  tmp=tmp>>6;
		  AD_Val[3]=tmp;
		  AD_Result[6]= (byte) (tmp>>8);
		  AD_Result[7]= (byte) (tmp&0x00FF);
	  }else{
		  for(i=0;i<8;i++){
			  AD_Result[i]=0xFF;						//Indicate ADC fails.
		  }
		  for(i=0;i<4;i++){
			  AD_Val[i]=0xFFFF;							//Indicate ADC fails.
		  }
	  }

}

/*!
** @}
*/
/*
** ###################################################################
**
**     This file was created by Processor Expert 10.3 [05.09]
**     for the Freescale HCS08 series of microcontrollers.
**
** ###################################################################
*/
