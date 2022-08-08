/* Bi color LED bargraph dislaying CPU workload 
 * coded by TinLethax Update 2022/08/08
 */
#include <stm8l.h>
#include <stdint.h>
#include <usart.h>
#include <delay.h>

#define devID (uint8_t)0x19 // slave address for the stm8l
volatile uint16_t Event = 0x00;
uint8_t cpuload = 0x00;



#ifdef DEBUF
void prntf(char *txt){
	while(*txt)
		usart_write(*txt++);
}
#endif
// For BI-color bargraph LED
//////////////////////////////////////////////////
void BGinit(){ // GPIO init for bargraph 
	PB_DDR |= 0xFF; // PB0-PB7 as output
	PB_CR1 |= 0xFF; // all at push-pull mode normal speed
}

void BGUpdate(uint8_t per){
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
void i2cinit() {
	CLK_PCKENR1 |= 1 << 3;// enable the I2C clock 
    I2C1_FREQR |= 16;// 16MHz/10^6
	
    I2C1_CR1 &= ~0x01;// cmd disable for i2c configurating

    I2C1_TRISER |= (uint8_t)(17);// Riser time  

    I2C1_CCRL = 0x50;
    I2C1_CCRH = 0x00;

    I2C1_CR1 |= (0x00 | 0x01);// i2c mode not SMBus
	
    I2C1_OARL = devID << 1;
    I2C1_OARH = 0x40; 

    I2C1_CR2 = (1 << 2);

    I2C1_CR1 |= (1 << 0);// cmd enable

    I2C1_ITR |= (1 << 0) | (1 << 1) | (1 << 2);// enable interrupt (buffer, event an error interrupt)
}

uint8_t i2cread()
{
  /* Return the data present in the DR register */
  return ((uint8_t)I2C1_DR);
}

// i2c interrupt, for incoming data
//////////////////////////////////////////////////

void i2c_slaveInt(void) __interrupt(29){
#ifdef DEBUG
	prntf("data packet received\n");
#endif
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

	switch(eviic){

    	case 0x0202 : //I2C_EVENT_SLAVE_RECEIVER_ADDRESS_MATCHED 
			while(!(I2C1_SR1 & 0x40));
			cpuload = I2C1_DR;// read data	
			
      	break;

		case 0x0010:// I2C_EVENT_SLAVE_STOP_RECEIVED	
			I2C1_CR2 |= (1 << I2C1_CR2_ACK);// send ack 
			
		break;

    	default:
	break;
	}
	
	// Clear Interrupt flag
	if(I2C1_SR1 & 0x10)
		I2C1_CR2 = 0x00;

}


// Main stuffs
//////////////////////////////////////////////////
uint8_t r = 0;
void main(){
	CLK_CKDIVR = 0x00;
	usart_init(9600); // usart using baudrate at 9600
	SYSCFG_RMPCR1 |= 0x10;// USART remapped to PA2(TX) and PA3(RX).
	usart_write(0x0C);
	usart_write(0x0C);
	
	BGinit(); // init all port B 
	i2cinit();// init i2c as slave with 7bit address == 0x19
#ifdef DEBUG
	prntf("i2cbar starting...\n");
#endif
	PB_ODR = 0;	
	__asm__("rim");// enble interrupt
	
	while(1){
		BGUpdate(cpuload);
		//delay_ms(1000);
	}

}// main 
