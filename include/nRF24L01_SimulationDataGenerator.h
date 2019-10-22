#pragma once

#include <AnalyzerHelpers.h>

class nRF24L01_AnalyzerSettings;

class nRF24L01_SimulationDataGenerator
{
public:
	nRF24L01_SimulationDataGenerator();
	~nRF24L01_SimulationDataGenerator();

	void Initialize(U32 simulation_sample_rate, nRF24L01_AnalyzerSettings* settings);
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );

protected:
	nRF24L01_AnalyzerSettings*	mSettings;
	U32			mSimulationSampleRateHz;

protected:	// SPI specific

	ClockGenerator mClockGenerator;

	void CreateNRFTransaction();
	void NewCommand();

	void OutputWord(U64 mosi_data, U64 miso_data);

	SimulationChannelDescriptorGroup	mSpiSimulationChannels;

	SimulationChannelDescriptor* mMiso;
	SimulationChannelDescriptor* mMosi;
	SimulationChannelDescriptor* mSck;
	SimulationChannelDescriptor* mCsn;
};
