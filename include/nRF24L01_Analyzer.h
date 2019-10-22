#pragma once

#include <Analyzer.h>

#include "nRF24L01_AnalyzerResults.h"
#include "nRF24L01_AnalyzerSettings.h"
#include "nRF24L01_SimulationDataGenerator.h"

class nRF24L01_Analyzer : public Analyzer
{
public:
	nRF24L01_Analyzer();
	virtual ~nRF24L01_Analyzer();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData(U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels);
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();

protected:	// vars

	nRF24L01_AnalyzerSettings					mSettings;
	std::auto_ptr<nRF24L01_AnalyzerResults>		mResults;

	AnalyzerChannelData*	mMiso;
	AnalyzerChannelData*	mMosi;
	AnalyzerChannelData*	mSck;
	AnalyzerChannelData*	mCsn;

	nRF24L01_SimulationDataGenerator mSimulationDataGenerator;

	bool mSimulationInitilized;

	bool GetByte(SpiByte& b, const bool is_first_byte_of_command);
	void SyncToSample(U64 sample);
	void SyncToChannel(AnalyzerChannelData* channel);

	bool AdvanceSck(U64 not_past_sample);
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer();
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer(Analyzer* analyzer);
