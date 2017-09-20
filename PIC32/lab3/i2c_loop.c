#include "NU32.h"          // config bits, constants, funcs for startup and UART
#include "i2c_slave.h"
#include "i2c_master_noint.h"
#include <stdio.h>
#include <stdint.h>

//LCD
#include "LCD.h"
#define MSG_LEN 20


// Demonstrate I2C by having the I2C1 talk to I2C5 on the same PIC32
// Master will use SDA1 (D9) and SCL1 (D10).  Connect these through resistors to
// Vcc (3.3 V) (2.4k resistors recommended, but around that should be good enough)
// Slave will use SDA5 (F4) and SCL5 (F5)
// SDA5 -> SDA1
// SCL5 -> SCL1
// Two bytes will be written to the slave and then read back to the slave.
#define SLAVE_ADDR 0x32
#define DELAYTIME 1000000000


int getUserNum(void) {
	char msg[100] = {};
	int f = 0;

	sprintf(msg, "Enter a number: from (0 to 255)\r\n");
	NU32_WriteUART3(msg);

	NU32_ReadUART3(msg,10);
	sscanf(msg, "%d", &f);
	return f;
}



int main() {
	char buf[100] = {};                    // buffer for sending messages to the user
	unsigned char master_write0 = 0;       // first byte that master writes
	unsigned char master_read0  = 0;       // first received byte
	char msg[MSG_LEN];
	int nreceived = 1;
	int numRead;
	NU32_Startup();
	LCD_Setup();
	
	
	__builtin_disable_interrupts();
	i2c_slave_setup(SLAVE_ADDR);              // init I2C5, which we use as a slave 
											//  (comment out if slave is on another pic)

	i2c_master_setup();                       // init I2C2, which we use as a master
	__builtin_enable_interrupts();
	

	
	while(1) {  		
		int num = getUserNum();
		master_write0 = (char)num;
		i2c_master_start();                     // Begin the start sequence
		i2c_master_send(SLAVE_ADDR << 1);       // send the slave address, left shifted by 1, 
												// which clears bit 0, indicating a write
		i2c_master_send(master_write0);         // send a byte to the slave       
		i2c_master_restart();                   // send a RESTART so we can begin reading 
		i2c_master_send((SLAVE_ADDR << 1) | 1); // send slave address, left shifted by 1,
												// and then a 1 in lsb, indicating read
		master_read0 = i2c_master_recv();       // receive a byte from the bus
		i2c_master_ack(1);                      // send NACK (1):  master needs no more bytes
		i2c_master_stop();                      // send STOP:  end transmission, give up bus

		sprintf(buf,"Master Wrote: %d\r\n", master_write0);
		NU32_WriteUART3(buf);
		sprintf(buf,"Master Read: %d\r\n", master_read0);
		NU32_WriteUART3(buf);

		sprintf(buf, "0x%02x", master_read0);
		LCD_Clear();                              // clear LCD screen
		LCD_Move(0,0);
		LCD_WriteString(buf);                     // write msg at row 0 col 0
		LCD_Move(1,3);
		//LCD_WriteString(msg);                     // write new msg at row 1 col 3
		//NU32_WriteUART3("\r\n"); 

		
	}
	return 0;
}



