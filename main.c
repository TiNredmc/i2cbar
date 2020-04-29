/* Bi color LED bargraph dislaying CPU workload 
 * coded by TinLethax
 */
#include <stm8l.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <usart.h>
#include <delay.h>

#define devID (uint8_t)0x32 // slave address for the stm8l
volatile uint16_t Event = 0x00;
uint8_t cpuload = 0x00;

uint16_t REMAP_Pin = 0x011C; 


int putchar(int c){
	usart_write(c);
	return 0;
}

int get_char() {
	return usart_read();
}

// For BI-color bargraph LED
//////////////////////////////////////////////////
void BGinit(){ // GPIO init for bargraph 
	PB_DDR |= 0xFF; // PB0-PB7 as output
	PB_CR1 |= 0xFF; // all at push-pull mode normal speed
}

void BGUpdate(uint8_t per){
	PB_ODR = 0x00;
	if (per < 14){
	PB_ODR = 0x02;
	}else if ((per > 13) && (per < 26)){
	PB_ODR = 0x01;
	}else if ((per > 25) && (per < 38)){
	PB_ODR = 0x09;
	}else if ((per > 37) && (per < 51)){
	PB_ODR = 0x05;	
	}else if ((per > 50) && (per < 63)){
	PB_ODR = 0x25;
	}else if ((per > 62) && (per < 76)){
	PB_ODR = 0x15;
	}else if ((per > 75) && (per < 78)){
	PB_ODR = 0x95;
	}else if (per > 77){
	PB_ODR = 0x55;
	}

}
// For I2C
//////////////////////////////////////////////////
uint16_t SCLSpeed = 0x0050;
void i2c_init() {
    I2C1_FREQR |= 16;// 16MHz/10^6
	
    I2C1_CR1 &= ~0x01;// cmd disable for i2c configurating

    I2C1_TRISER |= (uint8_t)(17);// Riser time  

    I2C1_CCRL = (uint8_t)SCLSpeed;
    I2C1_CCRH = (uint8_t)((SCLSpeed >> 8) & 0x0F);

    I2C1_CR1 |= (0x00 | 0x01);// i2c mode not SMBus
	
    I2C1_OARL = (uint8_t)(devID);
    I2C1_OARH = (uint8_t)((uint8_t)(0x00 | 0x40 ) | (uint8_t)((uint16_t)( (uint16_t)devID &  (uint16_t)0x0300) >> 7)); 

    I2C1_CR2 = (uint8_t)(1 << 2);

    I2C1_CR1 |= (1 << 0);// cmd enable

    I2C1_ITR |= (1 << 0) | (1 << 1) | (1 << 2);// enable interrupt (buffer, event an error interrupt)
}

uint8_t i2c_read()
{
  /* Return the data present in the DR register */
  return ((uint8_t)I2C1_DR);
}

// i2c interrupt, for incoming data
//////////////////////////////////////////////////

void i2c_slaveInt(void) __interrupt(29){
	printf("data packet received\n");
	PB_ODR = 0;
	/* Dummy reaing the SR2 */
	volatile uint8_t dummyread = 0x00;
	dummyread = I2C1_SR2;
	I2C1_SR2 = 0;
	/*event reading*/
	volatile uint16_t eviic = 0x0000;
	uint8_t flag1 = 0x00;
	uint8_t flag2 = 0x00;
	flag1 = I2C1_SR1;
	flag2 = I2C1_SR3;
	eviic = ((uint16_t)((uint16_t)flag2 << (uint16_t)8) | (uint16_t)flag1);
	printf("The event number is :%X\n",eviic);
	switch(eviic){

    	case 0x0202 : //I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED 
	printf("someone calling me!\n");
      	break;

      /* Check on EV2*/
    	case 0x0240 :// I2C_EVENT_SLAVE_BYTE_RECEIVED 
      	cpuload = i2c_read();
	printf("I've got %x\n",cpuload);
	BGUpdate(cpuload);
      	break;

    	default:
	break;
	}

}


// Main stuffs
//////////////////////////////////////////////////
uint8_t r = 0;
void main(){
CLK_CKDIVR = 0x00;
usart_init(19200); // usart using baudrate at 9600
SYSCFG_RMPCR1 &= (uint8_t)((uint8_t)((uint8_t)REMAP_Pin << 4) | (uint8_t)0x0F); //remap the non-exit pin of Tx and Rx of the UFQFPN20 package to the exit one.
SYSCFG_RMPCR1 |= (uint8_t)((uint16_t)REMAP_Pin & (uint16_t)0x00F0);
CLK_PCKENR1 |= (uint8_t)(1 << 0x03);// enable the I2C clock 
	BGinit(); // init all port B 
	i2c_init();// init i2c as slave having address 0x65
	printf(" starting...\n");
	PB_ODR = 0;	
	__asm__("rim");// enble interrupt
	while(1){
	__asm__("wfi");
	}

}// main 
