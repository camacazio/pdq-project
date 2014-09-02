// usb_test.cpp : Defines the entry point for the console application.
#include "stdafx.h"
using namespace std;

///////////////////////////////////////////////////
//some USB interface code
//carbon copy of USB_Device.cpp in the hfGUI3.sln
// Skip to the bottom of this for the main() function that runs this code
///////////////////////////////////////////////////
// Definitions for communication with the PDQ device over USB
#include "USB_Device.h"

//Ignore some standard warnings
#pragma warning(disable:4146)

// The vector of class instances of devices
std::vector<USB_WaveDev> USB_Waveform_Manager::USBWaveDevList;
// Holds the channel map of data to be sent to devices
USBWVF USB_Waveform_Manager::USBWvf;

// Definitions for class functions for a USB-connected FPGA card
USB_WaveDev::USB_WaveDev() {}
FT_STATUS USB_WaveDev::Open()
{
	// The device is opened by it's serial number and referenced by it's handle
	return FT_OpenEx(Serial, FT_OPEN_BY_SERIAL_NUMBER, &ftHandle);
}
FT_STATUS USB_WaveDev::Write(BYTE* wavePoint, DWORD size)
{
	// Write can be used to write a waveform or to send a reset command, etc
	std::cerr << "`USB::WaveDev::Write() started`" << std::endl;
		return FT_Write(ftHandle, wavePoint, size, &written);
}
FT_STATUS USB_WaveDev::Close()
{
    // Finished with the device, so closing it
	return FT_Close(ftHandle);
}

// Definitions for the class functions that are longer than one or two lines for the USB waveform manager
// Sets up a USB waveform device list
FT_STATUS USB_Waveform_Manager::InitSinglePDQMaster(DWORD devIndex, const char * serialNum, unsigned dacNum) {
	// Sets the serial number and number of DACs to a class instance
	strncpy(USBWaveDevList[devIndex].Serial,serialNum,10);
	USBWaveDevList[devIndex].num_DACs = dacNum;

	// NULL terminate the last two entries of the serial number, just in case
	USBWaveDevList[devIndex].Serial[8] = USBWaveDevList[devIndex].Serial[9] = '\0';

	// Opens the device for accessing
	return USBWaveDevList.at(devIndex).Open();
}

// Maps a serial number to a device index found after scanning USB ports for all FT245RL chips
int USB_Waveform_Manager::GetDeviceIndexFromSerialNumber(string * mySerialNo) {
	// Holds device number and device information
	DWORD numDevs;
	FT_DEVICE_LIST_INFO_NODE* devInfo;

	if (CreateDeviceInfoList(&numDevs) != FT_OK) {return -123404;}
	
	if (numDevs > 0){
		// allocate storage for list based on numDevs 
		devInfo = (FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*numDevs);
		if (GetDeviceInfoList(devInfo, &numDevs) != FT_OK) {return -123405;}
		
		// search for the requested device
		for (unsigned i = 0; i < numDevs; i++) { 
			string foundSerialNo (devInfo[i].SerialNumber);
			if(mySerialNo->compare(foundSerialNo) == 0) {
				return i;
			}
		}
	}
	else { 
		return -123406;
	} //end of if(numDevs>0)

	return 0;
}

// Takes in the current waveform data, and for each waveform channel above USB_CHAN_OFFSET, send it to the
// USB waveform device.  For now, always sends all channels and steps

//bool USB_Waveform_Manager::WvfDownload(Cwvffrm * wvffrm) {
//	// Iterate through each channel that is at/above USB_CHAN_OFFSET and pass all of its steps to WvfFill
//	// If there was data for that channel, it is added to ChanUpdate
//	vector<unsigned> ChanUpdate;
//
//	// Initialize the branch address list
//	for (int i = 0; i < USB_CHAN_TOTAL; i++) {
//		for (int j = 0; j < BRANCH_TOTAL; j++) {
//			ilistBranchLength[i][j] = 0;
//		}
//	}
//
//	// Clear out the waveform data, as we will now scan for channels with data and write it into the waveform
//	WvfClear(-1, -1);
//
//	// For passing to WvfWrite; should be the same for all channels on a single FPGA card
//	int mode;
//
//	unsigned chan;
//	// Iterate over the channels starting at USB_CHAN_OFFSET
//	for(chan = USB_CHAN_OFFSET; chan < USB_CHAN_OFFSET + USB_CHAN_TOTAL; chan++) {
//		// Iterate through the Steps and check the map from channel to ESteps
//			// Also, put the waveforms in order by branch. For example, if the user defines waveforms in various branches in the branch-order 0,1,1,1,0,2,0 it's re-ordered to 0,0,0,1,1,1,2.
//			// The reason for 0,1,1,1,0 in the first place is to tell an interp where to go. The user can do this by adding a wvf step to branch 0 even though that step won't ever run.
//		unsigned	orderedStep = 0;	// This will be the "step" in the ordered waveform list written to the FPGA
//		for(int iBranch = 0; iBranch < BRANCH_TOTAL; iBranch++) {
//			for(Cwvffrm::iterator its = wvffrm->begin(); its != wvffrm->end(); ++its) {
//				// Find the channel specified and check that it exists
//				CwvffrmStep::iterator ites = its->find(chan);
//
//				unsigned debugtest = unsigned(its - wvffrm->begin());
//
//				if( (!(ites == its->end())) && (ites->second).m_BranchNum == iBranch) {
//					// Append the current EStep to the specified channel and step
//					unsigned	unorderedStep = unsigned(its - wvffrm->begin()); // this is the official "step" in the unsorted waveform list
//					if(!(WvfFill(chan - USB_CHAN_OFFSET,orderedStep,(ites->second).m_PDQdevMode,(ites->second).m_vCurVals,(ites->second).m_vTimeVals,
//																			(ites->second).m_vdVVals,(ites->second).m_vd2VVals,(ites->second).m_vd3VVals)))
//					{return false;}
//					// Set the mode stored in this step of the channel
//					mode = (ites->second).m_PDQdevMode;
//					// Update the branch data size
//					ilistBranchLength[(chan - USB_CHAN_OFFSET)][(ites->second).m_BranchNum] += int((ites->second).m_vCurVals.size());
//					// Finished downloading to this channel, check if the channel needs to be added to ChanUpdate
//					for(vector<unsigned>::size_type i = 0; i < ChanUpdate.size(); i++) { 
//						if(chan == ChanUpdate.at(i)) {
//							// channel already added to the list, so clear the boolean
//							(ites->second).m_bChanUpdate = false;
//						}
//					}
//					if((ites->second).m_bChanUpdate == true) {
//						// add channel to the update list
//						ChanUpdate.push_back(chan);
//					}
//					(ites->second).m_bChanUpdate = false;
//					orderedStep++;
//				}
//			}
//		}
//	} 
//
//	// Write the data into the channel to be ready for the experiment if it is in the channel update vector
//	for(vector<unsigned>::size_type i = 0; i < ChanUpdate.size(); i++) { 
//		chan = unsigned(ChanUpdate.at(i) - USB_CHAN_OFFSET);
//		// Write to the channel
//		if(!USB_Waveform_Manager::WvfWrite(chan, mode)) {return false;}
//	}
//
//	return true;
//}

// Fill out the data in a waveform as bytes derived from vectors sent from a DC file

/*	1) check to make sure the channel and step is there to store data
	2) convert the voltage and time values into pure numbers that are in the range for the DACs (32767 to -32767)
	3) convert to a bit stream
	4) write to a vector, little indian in words (the FPGA's VHDL code expects a lower word followed by a higher word)*/

bool USB_Waveform_Manager::WvfFill(unsigned channel, unsigned step, int dataMode, 
	std::vector<double> vCurVals, std::vector<double> vTimeVals,
	std::vector<double> vdVVals, std::vector<double> vd2VVals, std::vector<double> vd3VVals) {
	// Indeces and temporary variables for writing to USBWVF data
	unsigned i = 0;
	unsigned j = 0;
	unsigned __int64 ui;
	__int64 nSteps;
	BYTE uc;
	double currentTime;
	double timeInterval;

	// Times from a DC file are in absolute time for when a waveform section ENDS
	// This is for holding the previous time value; the PDQ wants time differences
	double lastTime = 0;

	// Check that the channel has been defined in the waveform list
	USBWVF::iterator itc = USBWvf.find(channel);
	if (itc == USBWvf.end()) {
		// channel isn't defined, so create it
		USBWvf[channel] = USBWVF_channel();
	}
	// Check that the step has been defined in the waveform list
	USBWVF_channel::iterator its = USBWvf[channel].find(step);
	if (its == USBWvf[channel].end()) {
		// step isn't defined, so create it
		USBWvf[channel][step] = USBWVF_data();
	}

	// Used for quick accessing of the channel and step at hand
	USBWVF_data & wvfchanstep = USBWvf[channel][step];

	// Voltage mode, no coefficients or time values
	if (dataMode == 0) {
		for (i = 0; i < vCurVals.size(); i++) {
			// Convert +/-10V to a number with no floating point by multiplying by 32767 = 7FFF and dividing by the max voltage
			
			// Check to make sure that vCurVals[i] is in range
			if(vCurVals[i] < -10) {
				vCurVals[i] = -10;
			}
			if(vCurVals[i] > 10) {
				vCurVals[i] = 10;
			}
			
			ui = unsigned __int64((vCurVals[i]*USB_BYTE_RANGE)/USB_MAX_VOLTAGE); //unsigned __int64 is chosen to be certain we have enough data size, fixed, for any machine
			for(j = 0; j < 2; j++) {
				// the data is broken into 2 words and put on the waveform step little indian
				uc = BYTE(ui);
				wvfchanstep.push_back(uc);
				ui = ui>>8;
			}
		}
	}

	// Time mode, no coefficients
	else if (dataMode == 1) {
		for (i = 0; i < vCurVals.size(); i++) {
			// Convert +/-10V to a number with no floating point by multiplying by 32767 = 7FFF and dividing by the max voltage
			
			// Check to make sure that vCurVals[i] is in range
			if(vCurVals[i] < -10) {
				vCurVals[i] = -10;
			}
			if(vCurVals[i] > 10) {
				vCurVals[i] = 10;
			}
			
			ui = unsigned __int64((vCurVals[i]*USB_BYTE_RANGE)/USB_MAX_VOLTAGE); //unsigned __int64 is chosen to be certain we have enough data size, fixed, for any machine
			for(j = 0; j < 2; j++) {
				// the data is broken into 2 words and put on the waveform step little indian
				uc = BYTE(ui);
				wvfchanstep.push_back(uc);
				ui = ui>>8;
			}

			// Time differences are divided by the DAC update time to get a number of cycles
			// At the end of a step, the time value is negative to tell the FPGA to wait for a trigger afterwards
			
			if(vTimeVals[i] < 0) {
				currentTime = -vTimeVals[i];
				//clamp time intervals
				timeInterval = currentTime - lastTime;
				if(timeInterval < MIN_PDQ_TIME) {timeInterval = MIN_PDQ_TIME;}
				ui = unsigned __int64(-(timeInterval)/USB_DAC_UPDATE); 
				nSteps = (-1)*ui;
				for(j = 0; j < 2; j++) {
					// the data is broken into 2 words and put on the waveform step little indian
					uc = BYTE(ui);
					wvfchanstep.push_back(uc);
					ui = ui>>8;
				}
			}
			else {
				currentTime = vTimeVals[i];
				//clamp time intervals
				timeInterval = currentTime - lastTime;
				if(timeInterval < MIN_PDQ_TIME) {timeInterval = MIN_PDQ_TIME;}
				nSteps = ui = unsigned __int64((timeInterval)/USB_DAC_UPDATE);
				for(j = 0; j < 2; j++) {
					// the data is broken into 2 words and put on the waveform step little indian
					uc = BYTE(ui);
					wvfchanstep.push_back(uc);
					ui = ui>>8;
				}
			}
			// store the current cycle's time value
			lastTime = currentTime;
		}
	}

	// Interpolation mode: assign voltages, times, and coefficients
	else if (dataMode == 2) {
		// Check to make sure that vCurVals[0] (start) is in range
		// If it tries to go outside the range, there is a clamp VHDL-side
		if(vCurVals[0] < MIN_VOLTAGE) {
			vCurVals[0] = MIN_VOLTAGE;
		}
		if(vCurVals[0] > MAX_VOLTAGE) {
			vCurVals[0] = MAX_VOLTAGE;
		}

		for (i = 0; i < vCurVals.size(); i++) {
			// Convert +/-10V to a number with no floating point by multiplying by 32767 = 7FFF and dividing by the max voltage
			
			ui = unsigned __int64((vCurVals[i]*USB_BYTE_RANGE)/USB_MAX_VOLTAGE); //unsigned __int64 is chosen to be certain we have enough data size, fixed, for any machine
			for(j = 0; j < 2; j++) {
				// the data is broken into 2 words and put on the waveform step little indian
				uc = BYTE(ui);
				wvfchanstep.push_back(uc);
				ui = ui>>8;
			}

			// Time differences are divided by the DAC update time to get a number of cycles
			// At the end of a step, the time value is negative to tell the FPGA to wait for a trigger afterwards
			
			if(vTimeVals[i] < 0) {
				currentTime = -vTimeVals[i];
				//clamp time intervals
				timeInterval = currentTime - lastTime;
				if(timeInterval < MIN_PDQ_TIME) {timeInterval = MIN_PDQ_TIME;}
				ui = unsigned __int64(-(timeInterval)/USB_DAC_UPDATE); 
				nSteps = (-1)*ui;
				for(j = 0; j < 2; j++) {
					// the data is broken into 2 words and put on the waveform step little indian
					uc = BYTE(ui);
					wvfchanstep.push_back(uc);
					ui = ui>>8;
				}
			}
			else {
				currentTime = vTimeVals[i];
				//clamp time intervals
				timeInterval = currentTime - lastTime;
				if(timeInterval < MIN_PDQ_TIME) {timeInterval = MIN_PDQ_TIME;}
				nSteps = ui = unsigned __int64((timeInterval)/USB_DAC_UPDATE);
				for(j = 0; j < 2; j++) {
					// the data is broken into 2 words and put on the waveform step little indian
					uc = BYTE(ui);
					wvfchanstep.push_back(uc);
					ui = ui>>8;
				}
			}
			// store the current cycle's time value
			lastTime = currentTime;

			// linear coefficient is divided by the total time in number of steps		
			ui = unsigned __int64(65536*(vdVVals[i]*USB_BYTE_RANGE)/(nSteps*USB_MAX_VOLTAGE));
			for(j = 0; j < 4; j++) {
				// the integer part is broken into 4 words and put on the waveform step little indian
				uc = BYTE(ui);
				wvfchanstep.push_back(uc);
				ui = ui>>8;
			}
			// quadratic coefficient is divided by the total time in discrete number of steps squared
			ui = unsigned __int64(4294967296*(vd2VVals[i]*USB_BYTE_RANGE*2)/((nSteps)*(nSteps + 1)*USB_MAX_VOLTAGE));
			for(j = 0; j < 6; j++) {
				// the integer part is broken into 6 words and put on the waveform step little indian
				uc = BYTE(ui);
				wvfchanstep.push_back(uc);
				ui = ui>>8;
			}
		}
	}

	// Cubic spline mode: assign voltages, times, and coefficients
	else if (dataMode == 3) {

		// Check to make sure that vCurVals[0] (start) is in range
		// If it tries to go outside the range, there is a clamp VHDL-side
		if(vCurVals[0] < MIN_VOLTAGE) {
			vCurVals[0] = MIN_VOLTAGE;
		}
		if(vCurVals[0] > MAX_VOLTAGE) {
			vCurVals[0] = MAX_VOLTAGE;
		}

		for (i = 0; i < vCurVals.size(); i++) {
			// Convert +/-10V to a number with no floating point by multiplying by 32767 = 7FFF and dividing by the max voltage
			
			ui = unsigned __int64((vCurVals[i]*USB_BYTE_RANGE)/USB_MAX_VOLTAGE); //unsigned __int64 is chosen to be certain we have enough data size, fixed, for any machine
			for(j = 0; j < 2; j++) {
				// the data is broken into 2 words and put on the waveform step little indian
				uc = BYTE(ui);
				wvfchanstep.push_back(uc);
				ui = ui>>8;
			}

			// Time differences are divided by the DAC update time to get a number of cycles
			// At the end of a step, the time value is negative to tell the FPGA to wait for a trigger afterwards
			
			if(vTimeVals[i] < 0) {
				currentTime = -vTimeVals[i];
				//clamp time intervals
				timeInterval = currentTime - lastTime;
				if(timeInterval < MIN_PDQ_TIME) {timeInterval = MIN_PDQ_TIME;}
				ui = unsigned __int64(-(timeInterval)/USB_DAC_UPDATE); 
				nSteps = (-1)*ui;
				for(j = 0; j < 2; j++) {
					// the data is broken into 2 words and put on the waveform step little indian
					uc = BYTE(ui);
					wvfchanstep.push_back(uc);
					ui = ui>>8;
				}
			}
			else {
				currentTime = vTimeVals[i];
				//clamp time intervals
				timeInterval = currentTime - lastTime;
				if(timeInterval < MIN_PDQ_TIME) {timeInterval = MIN_PDQ_TIME;}
				nSteps = ui = unsigned __int64((timeInterval)/USB_DAC_UPDATE);
				for(j = 0; j < 2; j++) {
					// the data is broken into 2 words and put on the waveform step little indian
					uc = BYTE(ui);
					wvfchanstep.push_back(uc);
					ui = ui>>8;
				}
			}
			// store the current cycle's time value
			lastTime = currentTime;

			// linear coefficient is divided by the total time in number of steps		
			ui = unsigned __int64(65536*(vdVVals[i]*USB_BYTE_RANGE)/(nSteps*USB_MAX_VOLTAGE));
			for(j = 0; j < 4; j++) {
				// the integer part is broken into 4 words and put on the waveform step little indian
				uc = BYTE(ui);
				wvfchanstep.push_back(uc);
				ui = ui>>8;
			}
			// quadratic coefficient is divided by the total time in discrete number of steps squared
			ui = unsigned __int64(4294967296*(vd2VVals[i]*USB_BYTE_RANGE*2)/((nSteps)*(nSteps + 1)*USB_MAX_VOLTAGE));
			for(j = 0; j < 6; j++) {
				// the integer part is broken into 6 words and put on the waveform step little indian
				uc = BYTE(ui);
				wvfchanstep.push_back(uc);
				ui = ui>>8;
			}
			// cubic coefficient is divided by the total time in discrete number of steps cubed
			ui = unsigned __int64(4294967296*(vd3VVals[i]*USB_BYTE_RANGE*6)/((nSteps)*(nSteps + 1)*(nSteps + 2)*USB_MAX_VOLTAGE));
			for(j = 0; j < 6; j++) {
				// the integer part is broken into 6 words and put on the waveform step little indian
				uc = BYTE(ui);
				wvfchanstep.push_back(uc);
				ui = ui>>8;
			}
		}
	}
	return true;
}

// Writes all the waveform steps in a channel to the FPGA

/*
Final version:
1) send FPGA address
2) send DAC number x
3) send operation mode (WAIT_TRIGGER mode)
4) send address 00 00
5) send reg_length x
6) send burst length
7) write waveform
*/

bool USB_Waveform_Manager::WvfWrite(unsigned channel, int dataMode) {
	std::cerr << "Started `USB_Waveform_Manager::WvfWrite`" << std::endl;
	if(!(USBWvf[channel].empty())) {

		// Indeces for writing to USBWVF data
		int i;
		unsigned local_chan;
		unsigned j;
		unsigned FPGA_addr = 0;
		unsigned DAC_num = 0;
		unsigned devIndex = 0;	// The first USB device
		unsigned dataLength = 0; // For setting burst/reg_length
		unsigned __int64 ui; BYTE uc; // Used to convert numbers into little indian hex to send into initWave

		BYTE initWave[36]; // Holds the initialization code for the waveform
			// currently, initWave is just the right length

		vector<BYTE>::pointer waveform_data; //used for the data in each waveform step when transmitting to the PDQ

		BYTE * pInit; // Used for walking through the initWave array
		pInit = initWave; // point to the first element of initWave
				
		// Run an algorithm for obtaining the FPGA board address and the DAC number on that board, both starting from 0
		// Channel = (FPGA address)*3 + (DAC number on that board)
		local_chan = channel; j = 0;
		// Check which device we must access, experiment returns an error if there are no devices here
		while(local_chan >= USB_Waveform_Manager::USBWaveDevList[devIndex].num_DACs) {
			local_chan = local_chan - USB_Waveform_Manager::USBWaveDevList[devIndex].num_DACs;
			devIndex++;
		}
		// Get the FPGA number on that device
		i = local_chan;
		while(i >= 3) {
			i = i - 3;
			j++;
		}
		FPGA_addr = j;
		DAC_num = local_chan - (FPGA_addr * 3);
		// Sending the FPGA address
		*pInit = 0x00;
		pInit++;
		// Put the FPGA_addr into a word to send
		ui = unsigned __int64(FPGA_addr);
		uc = BYTE(ui);
		*pInit = uc;
		pInit++;

		// Sending the DAC number
		if(DAC_num == 0) {
			*pInit = 0x10;
		}
		else if(DAC_num == 1) {
			*pInit = 0x11;
		}
		else if(DAC_num == 2) {
			*pInit = 0x12;
		}
		else {
			*pInit = 0x10;
		}
		pInit++;

		// Set the DAC operation mode
		*pInit = 0x08;
		pInit++;
		if(dataMode == 0) {
			*pInit = 0x00;
		}
		else if(dataMode == 1) {
			*pInit = 0x01;
		}
		else if(dataMode == 2) {
			*pInit = 0x02;
		}
		else if(dataMode == 3) {
			*pInit = 0x03;
		}
		// Default to cubic splines
		else {
			*pInit = 0x03;
		}
		pInit++;
		//*pInit = 0x01; // TTL trigger mode	
		//pInit++;

		// Memory address to write is to be set to 00 00, at the start of memory
		*pInit = 0x02;
		pInit++;
		*pInit = 0x00;
		pInit++;
		*pInit = 0x00;
		pInit++;

		// The total length of the data is obtained for the channel specified
		for(USBWVF_channel::iterator its = USBWvf[channel].begin(); its != USBWvf[channel].end(); ++its) {
			// If the step isn't empty then procede
			if(!(its->second).empty()) {
				for(j = 0; j < (its->second).size(); ++j) {
					// There is data, put it into dataLength; this counts number of words, 2 words is 1 reg_length
					dataLength++;
				}
			}
		}

		*pInit = 0x06; // Set reg_length
		// Put the dataLength/2 into reg_length
		ui = unsigned __int64(dataLength/2);
		for(i = 0; i < 2; i++) {
			// the data is broken into 2 words and put on the waveform step little indian
			uc = BYTE(ui);
			pInit++;
			*pInit = uc;
			ui = ui>>8;
		}
		pInit++;

		//// Setting branch point segments, assumes cubic spline (mode 3) which has 10 bytes per waveform polynomial

			*pInit = 0x20; // Set the 0th branch ending address; for each voltage value, there are 10 spots in memory for a cubic spline
			unsigned __int64 branch0 = unsigned __int64(ilistBranchLength[channel][0]*10);
			ui = branch0;
			for(i = 0; i < 2; i++) {
				// the data is broken into 2 words and put on the waveform step little indian
				uc = BYTE(ui);
				pInit++;
				*pInit = uc;
				ui = ui>>8;
			}
			pInit++;

			*pInit = 0x21; // Set the 1st branch ending address; for each voltage value, there are 10 spots in memory for a cubic spline
			unsigned __int64 branch1 = unsigned __int64(ilistBranchLength[channel][1]*10);
			ui = branch1 + branch0;
			for(i = 0; i < 2; i++) {
				// the data is broken into 2 words and put on the waveform step little indian
				uc = BYTE(ui);
				pInit++;
				*pInit = uc;
				ui = ui>>8;
			}
			pInit++;

			*pInit = 0x22; // Set the 2nd branch ending address; for each voltage value, there are 10 spots in memory for a cubic spline
			unsigned __int64 branch2 = unsigned __int64(ilistBranchLength[channel][2]*10);
			ui = branch2 + branch1 + branch0;
			for(i = 0; i < 2; i++) {
				// the data is broken into 2 words and put on the waveform step little indian
				uc = BYTE(ui);
				pInit++;
				*pInit = uc;
				ui = ui>>8;
			}
			pInit++;

			*pInit = 0x23; // Set the 3rd branch ending address; for each voltage value, there are 10 spots in memory for a cubic spline
			unsigned __int64 branch3 = unsigned __int64(ilistBranchLength[channel][3]*10);
			ui = branch3 + branch2 + branch1 + branch0;
			for(i = 0; i < 2; i++) {
				// the data is broken into 2 words and put on the waveform step little indian
				uc = BYTE(ui);
				pInit++;
				*pInit = uc;
				ui = ui>>8;
			}
			pInit++;

			*pInit = 0x24; // Set the 4th branch ending address; for each voltage value, there are 10 spots in memory for a cubic spline
			unsigned __int64 branch4 = unsigned __int64(ilistBranchLength[channel][4]*10);
			ui = branch4 + branch3 + branch2 + branch1 + branch0;
			for(i = 0; i < 2; i++) {
				// the data is broken into 2 words and put on the waveform step little indian
				uc = BYTE(ui);
				pInit++;
				*pInit = uc;
				ui = ui>>8;
			}
			pInit++;

			*pInit = 0x25; // Set the 5th branch ending address; for each voltage value, there are 10 spots in memory for a cubic spline
			unsigned __int64 branch5 = unsigned __int64(ilistBranchLength[channel][5]*10);
			ui = branch5 + branch4 + branch3 + branch2 + branch1 + branch0;
			for(i = 0; i < 2; i++) {
				// the data is broken into 2 words and put on the waveform step little indian
				uc = BYTE(ui);
				pInit++;
				*pInit = uc;
				ui = ui>>8;
			}
			pInit++;

			*pInit = 0x26; // Set the 6th branch ending address; for each voltage value, there are 10 spots in memory for a cubic spline
			unsigned __int64 branch6 = unsigned __int64(ilistBranchLength[channel][6]*10);
			ui = branch6 + branch5 + branch4 + branch3 + branch2 + branch1 + branch0;
			for(i = 0; i < 2; i++) {
				// the data is broken into 2 words and put on the waveform step little indian
				uc = BYTE(ui);
				pInit++;
				*pInit = uc;
				ui = ui>>8;
			}
			pInit++;

		////

		*pInit = 0x01; // Set burst length
		// Put the data length into burst_length
		ui = unsigned __int64(dataLength/2);
		for(i = 0; i < 2; i++) {
			// the data is broken into 2 words and put on the waveform step little indian
			uc = BYTE(ui);
			pInit++;
			*pInit = uc;
			ui = ui>>8;
		}

		// Finally, the write command
		pInit++;
		*pInit = 0x04;

		// Write the initialization waveform part to the device
		std::cerr << "Write the initialization waveform part to the device (USbWaveDevList[].Write)" << std::endl;
		if(USBWaveDevList.size()){
			if (USB_Waveform_Manager::USBWaveDevList[devIndex].Write(&initWave[0], (DWORD) sizeof(initWave)) == FT_OK) {
				// No errors detected
			}
			else {
				// failure
				return false;
			}
		}
		
		// For the channel specified, the data in the channel is sent to the FPGA
		// Send one BYTE at a time, and loop through the BYTE vector in a step for each step in a channel
		// Iterate across the channel, writing in all the steps of the waveform
		std::cerr << ".. the data in the channel is sent to the FPGA" << std::endl;
		if(USBWaveDevList.size()){
			for(USBWVF_channel::iterator its = USBWvf[channel].begin(); its != USBWvf[channel].end(); ++its) {
				if(!(its->second).empty()) {
					// Write all the data held in the current step to a specified device
					waveform_data = &(its->second)[0];
					if (USB_Waveform_Manager::USBWaveDevList[devIndex].Write(&waveform_data[0], (DWORD) (its->second).size()) == FT_OK) {
						// No errors detected
					}
					else {
						// failure
						return false;
					}
				}
				// At the end of the step, the final time value should be negative: VHDL code sees this as a pause
			}
		}

	}
	std::cerr << "Returning from `USB_Waveform_Manager::WvfWrite` with 'true'" << std::endl;
	return true;
}

// Clear out the data in a channel or step, or clear it all
void USB_Waveform_Manager::WvfClear(int channel, int step) {
	// Check that the channel has been defined in the waveform list
	USBWVF::iterator itc = USBWvf.find(channel);
	if (channel == -1) {
			// clear all channels
			USBWvf.clear();
	}
	else if (channel < -1 || step < -1) {
		// Bad channel or step choice
	}
	else if (itc == USBWvf.end()) {
		// Channel isn't defined
	}
	else {
		if (step == -1) {
			// remove the channel data
			USBWvf.erase(itc);
		}
		else {
			// Check that the step has been defined in the waveform list
			USBWVF_channel::iterator its = USBWvf[channel].find(step);
			if (its == USBWvf[channel].end()) {
				// step isn't defined
			}
			else{
				// remove the specific step from the waveform
				USBWvf[channel].erase(its);
			}
		}
	}
}
//////////////////////////////////////////
//end of stuff for USB class type functions
//////////////////////////////////////////

// main!
int main()
{

	// Welcome
	std::cout << "This is PDQ test" << std::endl;

	// -------------------------------

	unsigned tempDevIndex; // local temporary device index when searching through all connected USB devices

	// Parses out the USB_WAVEFORM_LIST in 6733wfm.h into a set of things that can be sent into a device instance
	string str(USB_WAVEFORM_LIST);
    string buf; // Have a buffer string
    stringstream ss(str); // Insert the string into a stream
    vector<string> serialList; // Create vector to hold expected serial numbers for PDQDACs
	vector<int> devIndexList; // Create vector to hold expected USB FT245RL device index for PDQDACs
	vector<string> dacList; // Create a vector, each master has some number of DAC channels

	// properly parse the USB_WAVEFORM_LIST in 6733wfm.h
	// USB waveform card definition: "serial# #ofDACs serial# #ofDACs ..."
	bool i = 0;
	while (ss >> buf) {
		switch (i){
			case 0: {
				// Seek out the desired serial number
				tempDevIndex = USB_Waveform_Manager::GetDeviceIndexFromSerialNumber( &buf );
				if (tempDevIndex < 0){
					return -123402;
				}
				devIndexList.push_back(tempDevIndex);
				serialList.push_back(buf); //stores a serial number
				i = 1;}
				break;
			case 1: {
				dacList.push_back(buf); //stores a number of DACs
				i = 0;}
		}
	}

	// This sets the serial numbers and number of DACs of the devices and opens them for accessing
	const char * serialNum;
	unsigned dacNum;
	unsigned PDQtotal = unsigned(devIndexList.size());
	// Size the vector of devices to match the number of attached devices
	USB_Waveform_Manager::ListSize(PDQtotal);
	if (PDQtotal > 0) { //numDevs >= len(serialList)

		for(unsigned i = 0; i < PDQtotal; i++) {
			serialNum = serialList[i].c_str();
			stringstream ss(dacList[i]); 
			ss >> dacNum;
			if (USB_Waveform_Manager::InitSinglePDQMaster(i/*devIndexList.at(i)*/, serialNum, dacNum) == FT_OK) {

				// No errors detected
			}
			else {
				// failure
				return -123403;

			}
		}
	}

	// Here, one can set some options for the desired channel and step for the waveform
	// Included here is a set of voltages and coefficients that generate a sin^2 ramp from 0 to 1
	// The final time, being negative, indicates a point where the PDQ will await for a TTL trigger before continuing

	unsigned channel = 0;
	unsigned step = 0;
	unsigned branch = 0;
	int mode = 3;

	
	// Initialization for USB Waveform cards
	// This accesses functions in USB_Device.h

	// Input Channel and branch
	std::cout << "Available Channels: " << dacNum << std::endl;

	std::cout << "Enter channel number: ";
	std::cin >> channel;

	std::cout << "Enter branch number: ";
	std::cin >> branch;

	// -----------------------------

	// std::cout << "Read wavefrom from file (press <f>) or enter constant voltage (press <v> (or any other))" << std::endl;
	int mychar = 'f';
	std::string waveformfile = std::string("");

		vector<double> vTimeSSQ;
	vector<double> vSSQVals;
	vector<double> vdVSSQ;
	vector<double> vd2VSSQ;
	vector<double> vd3VSSQ;

	// mychar = getchar();
	mychar = 'f';
	switch (mychar)
	{
	case 'f':
		std::cout << "Enter filename:" << std::endl;
		std::cin >> waveformfile;

		if (waveformfile != "") //change for a proper check, whether file name is valid
		{
			std::cout << "Reading waveform from: " << waveformfile << std::endl;
			std::string line;
			ifstream wfstream(waveformfile);
			std::stringstream sss;


			double darray[5] = { 0, 0, 0, 0, 0 };

			if (wfstream.is_open()) // put into loop, until file is really open, or user chooses break
			{
				std::cout << "Reading waveform from: " << waveformfile << std::endl;
				int i = 0;
				while (getline(wfstream, line))
				{
					std::cout << line << std::endl;
					sss << line;
					
					for (int j = 0; j < 5; j++)
					{
						sss >> darray[j];

					}
					for (int j = 0; j < 5; j++)
					{
						std::cout << darray[j] << ", ";
						switch (j)
						{
						case 0:
							vTimeSSQ.push_back(darray[j]);
							break;
						case 1:
							vSSQVals.push_back(darray[j]);
							break;
						case 2:
							vdVSSQ.push_back(darray[j]);
							break;
						case 3:
							vd2VSSQ.push_back(darray[j]);
							break;
						case 4:
							vd3VSSQ.push_back(darray[j]);
							break;
						default:
							std::cout << "This should not happen. Invalid position in loop" << std::endl;
							break;
						}
					}
					sss.clear();
					sss = std::stringstream(std::string(""));
					std::cout << std::endl;

					i++;
				}
				wfstream.close();
			}

		}
		break;
	default:
		// set a contant voltage
		double voltage = 0;
		std::cout << "Set Voltage:"; // Add a check, whether voltage is in Range
		std::cin >> voltage;

		for (int i = 0; i < 10; i++)
		{
			vTimeSSQ.push_back(i);
			vSSQVals.push_back(voltage);
			vdVSSQ.push_back(0);
			vd2VSSQ.push_back(0);
			vd3VSSQ.push_back(-.01+i*.001);
		}
		break;
	}

	// -----------------------------



	// Give the branch size for the chosen channel. If multiple branches are used, each branch will have a different length.
	for(unsigned i = 0; i < BRANCH_TOTAL; i++) {
		ilistBranchLength[channel][i] = vTimeSSQ.size();
	}

	std::cerr << "Fill Waveform ( calling USB_Waveform_Manager::WftFill(...) )" << std::endl;
	USB_Waveform_Manager::WvfFill(channel, step, mode, vSSQVals, vTimeSSQ, vdVSSQ, vd2VSSQ, vd3VSSQ);

	// clear command, empties local waveform data
	//USB_Waveform_Manager::WvfClear(channel,step);



	// Transmit waveform data
	std::cerr << "transmit waveform data ( Calling USB_Waveform_Manager::WvfWrite(...) )" << std::endl;
	USB_Waveform_Manager::WvfWrite(channel, mode);

	std::cerr << "Close devices" << std::endl;
    // Close each device found
	if (PDQtotal > 0) {
		for(unsigned i = 0; i < PDQtotal; i++) {
			if (USB_Waveform_Manager::CloseDevice(i) == FT_OK) {
				std::cerr << "No errors detected. Exit" << std::endl;
				// No errors detected
			}
			else {
				std::cerr << "Error writing PDQ" << std::endl;
				return -432101;

			}
		}
	}
	return 0;
}