#include "ui.h"

enum ui_state {
	connecting = 0,
	connected = 1
};

static enum ui_state currentState = connecting;

int8_t curDisplayedBasestation = 0;

uint8_t lastButtonState;

void initUI(void) {
	DDRF &= ~(0b111);
	lastButtonState = PINF & 0b111;
	sendLCDCmd(LCD_CMD_CLEAR);
	serviceLCD(); // Clear the LCD just to make sure we know where we are.
	_delay_ms(2); // This takes a long time
	sendLCDCmd(LCD_CMD_DSP_ON);

	ui_baseStationDisconnected();
}

void updateUI(void) {
	if (currentState == connecting) {
		// Display list of networks to connect to
		uint8_t changedButtons = (lastButtonState ^ (PINF & 0b111)) & lastButtonState;
		if (changedButtons == 0b001) {
			if (curDisplayedBasestation > 0) {
				curDisplayedBasestation -= 1;
				LCD_MOVE_TO_CHAR(1,1);
				writeString("       ",7);
				LCD_MOVE_TO_CHAR(1,1);
				writeString(baseStationList[curDisplayedBasestation].name, 7);
			}
		}
		else if (changedButtons == 0b010) {
			if (curDisplayedBasestation < (baseStationListLength-1)) {
				curDisplayedBasestation += 1;
				LCD_MOVE_TO_CHAR(1,1);
				writeString("       ",7);
				LCD_MOVE_TO_CHAR(1,1);
				writeString(baseStationList[curDisplayedBasestation].name, 7);
			}
		}
		else if (changedButtons == 0b100) {
			sendConnectionRequest(curDisplayedBasestation, &deviceCalibration);
		}

		lastButtonState = PINF & 0b111;
	}
	else {
		
	}
}

void ui_baseStationConnected(void) {
	LCD_MOVE_TO_CHAR(0,0);
	writeString("1:  0.0W2:  0.0W", 16);

	currentState = connected;
}

void ui_updatePowerValues(int64_t ch1, int64_t ch2, uint32_t sampleCount){
	if (currentState == connected) {
		char pwrStr[6] = {0,0,0,0,0,0};
		ldiv_t res = ldiv(((((ch1*((int64_t)deviceCalibration.channel1VoltageScaling))/1000000)*((int64_t)deviceCalibration.channel1CurrentScaling))/sampleCount),10000000);
		if (res.rem > (10000000/2))
			res.quot++;
		ltoa(res.quot, pwrStr, 10);
		uint8_t len = strlen(pwrStr);
		LCD_MOVE_TO_CHAR(0, 2);
		while (len++ < 5)
			writeChar(' ');
		writeString(pwrStr, 5);
		
		memset(pwrStr, 0, 6);
		res = ldiv(((((ch2*((int64_t)deviceCalibration.channel2VoltageScaling))/1000000)*((int64_t)deviceCalibration.channel2CurrentScaling))/sampleCount),10000000);
		if (res.rem > (10000000/2))
			res.quot++;
		ltoa(res.quot, pwrStr, 10);	
		len = strlen(pwrStr);
		LCD_MOVE_TO_CHAR(1, 2);
		while (len++ < 5)
			writeChar(' ');
		writeString(pwrStr, 5);
	}
}

void ui_baseStationDisconnected(void) {
	LCD_MOVE_TO_CHAR(0,0);
	writeString("Network?",8);
	LCD_MOVE_TO_CHAR(1,0);
	
	curDisplayedBasestation = 0;
	writeChar(0b01111110);
	writeString(baseStationList[0].name, 7);

	currentState = connecting;
}

void ui_baseStationListChanged(int8_t modifiedEntry) {
	if (curDisplayedBasestation == modifiedEntry) {
		LCD_MOVE_TO_CHAR(1,1);
		writeString("       ",7);
		LCD_MOVE_TO_CHAR(1,1);
		writeString(baseStationList[curDisplayedBasestation].name, 7);
	}
	
	if (curDisplayedBasestation >= baseStationListLength) {
		curDisplayedBasestation = baseStationListLength-1;
		LCD_MOVE_TO_CHAR(1,1);
		writeString("       ",7);
		LCD_MOVE_TO_CHAR(1,1);
		writeString(baseStationList[curDisplayedBasestation].name, 7);
	}
}
