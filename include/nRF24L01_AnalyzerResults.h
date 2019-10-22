#pragma once

#include <AnalyzerResults.h>

#include "nRFTypes.h"

class nRF24L01_Analyzer;
class nRF24L01_AnalyzerSettings;

class nRF24L01_AnalyzerResults: public AnalyzerResults
{
public:
	nRF24L01_AnalyzerResults(nRF24L01_Analyzer* analyzer, nRF24L01_AnalyzerSettings* settings);
	virtual ~nRF24L01_AnalyzerResults();

	virtual void GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base);
	virtual void GenerateExportFile(const char* file, DisplayBase display_base, U32 export_type_user_id);

	virtual void GenerateFrameTabularText(U64 frame_index, DisplayBase display_base);
	virtual void GeneratePacketTabularText(U64 packet_id, DisplayBase display_base);
	virtual void GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base);

	bool CreateFramesFromSpiBytes(const std::vector<SpiByte>& spi_bytes, U64 csnLow, U64 csnHi);

protected: //functions

	nRFCommand		mCommand;
	U64				mCommandWordFrameIndex;

protected:  //vars

	// used for storing data that doesn't fit into Frame's mData1 and mData2
	std::vector<U8>		mExtendedData;

	nRF24L01_AnalyzerSettings*	mSettings;
	nRF24L01_Analyzer*			mAnalyzer;
};
