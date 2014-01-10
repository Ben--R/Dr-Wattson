#include "basestation.h"

struct baseStation baseStationList[BASESTATION_LIST_SIZE];
int8_t baseStationListLength = 0;
int8_t connectedBaseStation = -1;
	
static NWK_DataReq_t nwkPacket;
static connectionRequestPacket_t connReqPacket;

void packetTxConf(NWK_DataReq_t *req);

// Adds a new base station to the end of the base station list. If there is no
// space left in the list then the first item in the list is removed.
int8_t addBaseStation(uint16_t PAN_ID, uint16_t addr, int8_t rssi, char name[]) {
	if (baseStationListLength >= BASESTATION_LIST_SIZE) {
		// There isn't enough space to fit a new item, first make some space
		removeBaseStation(0);
	}

	baseStationList[baseStationListLength].PAN_ID = PAN_ID;
	baseStationList[baseStationListLength].addr = addr;
	baseStationList[baseStationListLength].rssi = rssi;
	memcpy(&(baseStationList[baseStationListLength].name), name, 16);
	baseStationListLength += 1;
	
	return baseStationListLength - 1;
}

void removeBaseStation(int8_t num) {
	memcpy(&(baseStationList[num]), &(baseStationList[num+1]), sizeof(struct baseStation) * (BASESTATION_LIST_SIZE - num - 1));
	baseStationListLength -= 1;
}

// Processes a new bacon packet. If the source base station is not already in 
// the list, it is added to the list. If there is no space in the list, and the
// RSSI of the packet is greater than another base station in the list, the 
// station with the lowest RSSI is replaced. If the same network with a different
// name is detected, the old network name is replaced with the new one from the packet.
void processBaconPacket(NWK_DataInd_t *packet) {
	baconPacket_t *bacon = (baconPacket_t*)(packet->data);
	int8_t rssi_min = 0;
	int8_t rssi_min_pos = -1;
	for (int8_t count = 0; count < baseStationListLength; count++) {
		if ((bacon->PAN_ID == baseStationList[count].PAN_ID) &&
		    (packet->srcAddr == baseStationList[count].addr)) {
			// This base station PAN and address matches one which exists in the list.
			// Update the entry and return.
			baseStationList[count].rssi = packet->rssi;
			memcpy(&(baseStationList[count].name), &(bacon->name), 16);
			return;
		}

		if (baseStationList[count].rssi <= rssi_min) { // New minimum
			rssi_min = baseStationList[count].rssi;
			rssi_min_pos = count;
		}
	}

	// If there isn't room in the list, remove the basestation with the lowest RSSI if it's less then the new rssi.
	if (baseStationListLength >= BASESTATION_LIST_SIZE) {
		if (rssi_min > packet->rssi)
			return; // The new packet's rssi is less than any of the existing networks. Just ignore this one.
		
		removeBaseStation(rssi_min_pos);
	}

	// Add the new base station;
	addBaseStation(bacon->PAN_ID, packet->srcAddr, packet->rssi, (char*)(&(bacon->name)));
}

bool processConnectionAck(NWK_DataInd_t *packet) {
	for (int8_t count = 0; count < baseStationListLength; count++) {
		if (packet->srcAddr == baseStationList[count].addr) {
			// Found the source base station in our list
			connectedBaseStation = count;
			return true;
		}
	}
	// If we got here then the source of the ack packet was not found in our basestation list,
	// so don't say we connected.
	return false;
}

void sendConnectionRequest(int8_t num, struct calibData *cal) {
	connectedBaseStation = -1; // -1 Indicates that we are not connected to a network

	connReqPacket.type = connectionRequest;
	connReqPacket.channel1VoltageScaling = cal->channel1VoltageScaling;
	connReqPacket.channel1CurrentScaling = cal->channel1CurrentScaling;
	connReqPacket.channel2VoltageScaling = cal->channel2VoltageScaling;
	connReqPacket.channel2VoltageScaling = cal->channel2CurrentScaling;
	connReqPacket.linePeriodScalingFactor = cal->linePeriodScalingFactor;

	nwkPacket.dstAddr = baseStationList[num].addr;
	nwkPacket.dstEndpoint = APP_ENDPOINT;
	nwkPacket.srcEndpoint = APP_ENDPOINT;
	nwkPacket.data = (uint8_t *)(&connReqPacket);
	nwkPacket.size = sizeof(connectionRequestPacket_t);
	nwkPacket.confirm = packetTxConf;
	NWK_SetPanId(baseStationList[num].PAN_ID);
	NWK_DataReq(&nwkPacket);
}

