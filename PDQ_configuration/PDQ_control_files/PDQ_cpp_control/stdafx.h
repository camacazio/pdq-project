// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently

#pragma once

#include <iostream> //for cin and cout
#include <fstream> // for reading files
#include <sstream> // needed for interpreting files

// Parse by white space, listing serial numbers and number of dacs on that serial number
// "Serial# #DACs Serial# #DACs ..."
#define USB_WAVEFORM_LIST "DREIECK0 6"
// Gives the total number of all DACs
#define USB_CHAN_TOTAL 6