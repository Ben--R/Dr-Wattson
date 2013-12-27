#include <avr/io.h>
#include <util/delay.h>
#include <stdint.h>

#include "config.h"
#include "hal.h"
#include "phy.h"
#include "sys.h"
#include "nwk.h"
#include "sysTimer.h"

#include "uart.h"

#include "protocol.h"

static uint16_t counter = 0;
static NWK_DataReq_t txPacket;

uint8_t uart_tx_buf[100];
uint8_t uart_rx_buf[100];
uint8_t packet_buf[100];

static void packetTxConf(NWK_DataReq_t *req) {
	// We don't really care about if a packet was received or not
	// But we have to have a function here, because the library does not
	// handle null function pointers for the confirm callback.
}
		
static void APP_TaskHandler(void) {
	if (counter == 0) { 
		PORTB = (~PORTB)&(1<<4);
	}
	counter = (counter+((uint16_t)1))%400;
  // Put your application code here
}

static bool rfReceivePacket(NWK_DataInd_t *ind) {
	rxHeader_t packetHeader;
	packetHeader.size = ind->size;
	packetHeader.sourceAddr = ind->srcAddr;
	packetHeader.rssi = ind->rssi;
	uart_tx_data((uint8_t*)(&packetHeader), sizeof(rxHeader_t));
	uart_tx_data(ind->data, ind->size);
	return true;
}

int main(void) {
	SYS_Init(); // Init Atmel Lightweight Mesh stack
	SYS_TaskHandler(); // Call the system task handler once before we configure the radio
	NWK_SetAddr(APP_ADDR);
	NWK_SetPanId(APP_PANID);
	PHY_SetChannel(APP_CHANNEL);
	PHY_SetRxState(true);
	NWK_OpenEndpoint(APP_ENDPOINT, rfReceivePacket);
	PHY_SetTxPower(0);

	uart_init_port(uart_baud_38400, uart_tx_buf, 100, uart_rx_buf, 100); // Init uart

	// Configure onboard LED as output
	DDRB |= 1<<4;
	PORTB |= 1<<4;

	// Configure analog switch for antenna
	DDRG |= 1<<1;
	DDRF |= 1<<2;
	PORTG |= 1<<1;
	PORTF &= ~(1<<2);
	
	while (1) {
		SYS_TaskHandler();
		APP_TaskHandler();

		// Parse received uart packets, if we get one, then transmit it to the RF network
		if (uart_rx_peek(0) == 0)
			uart_rx_byte(); // Throw out the first byte if it's a null, we shouldn't need this, but it seemed to help on the PC side
		
		if ((uart_rx_peek(0) + sizeof(txHeader_t)) >= uart_received_bytes()) {
			txHeader_t packetHeader;
			uart_rx_data(&packetHeader, sizeof(txHeader_t));
			uart_rx_data(packet_buf, packetHeader.size);
			
			txPacket.dstAddr = packetHeader.destAddr;
			txPacket.dstEndpoint = APP_ENDPOINT;
			txPacket.srcEndpoint = APP_ENDPOINT;
			txPacket.options = 0;
			txPacket.data = packet_buf;
			txPacket.size = packetHeader.size;
			txPacket.confirm = packetTxConf;
			NWK_DataReq(&txPacket);
		}
	}
}