#include <AnalyzerChannelData.h>

#include <vector>

#include "utils.h"
#include "nRF24L01_Analyzer.h"
#include "nRF24L01_AnalyzerSettings.h"


nRF24L01_Analyzer::nRF24L01_Analyzer()
:	mSimulationInitilized(false)
{
	SetAnalyzerSettings(&mSettings);
}

nRF24L01_Analyzer::~nRF24L01_Analyzer()
{
	KillThread();
}

void nRF24L01_Analyzer::SyncToChannel(AnalyzerChannelData* channel)
{
	SyncToSample(channel->GetSampleNumber());
}

void nRF24L01_Analyzer::SyncToSample(U64 to_sample)
{
	mCsn->AdvanceToAbsPosition(to_sample);
	mMiso->AdvanceToAbsPosition(to_sample);
	mMosi->AdvanceToAbsPosition(to_sample);
	mSck->AdvanceToAbsPosition(to_sample);
}

bool nRF24L01_Analyzer::AdvanceSck(U64 csn_edge)
{
	// CSN went high before the byte was over?
	if (!mSck->DoMoreTransitionsExistInCurrentData()  ||  mSck->GetSampleOfNextEdge() > csn_edge)
	{
		SyncToSample(csn_edge);
		return false;
	}

	mSck->AdvanceToNextEdge();

	return true;
}

bool nRF24L01_Analyzer::GetByte(SpiByte& b, const bool is_first_byte_of_command)
{
	b.Clear();

	U8 num_bits = 0;
	U64 csn_edge = mCsn->GetSampleOfNextEdge();

	// check if SCK is high
	bool sample_first_bit_on_falling_edge = false;
	if (mSck->GetBitState() == BIT_LOW)
	{
		if (!AdvanceSck(csn_edge))
			return false;
	} else if (is_first_byte_of_command) {
		sample_first_bit_on_falling_edge = true;
	}

	for (;;)
	{
		// advance to the falling edge for misbehaved SPI (maybe this behavor should be an option in the settings?)
		if (sample_first_bit_on_falling_edge  &&  !AdvanceSck(csn_edge))
			return false;

		// resync the other channels
		SyncToChannel(mSck);

		if (num_bits == 0)
			b.mStartingSample = mSck->GetSampleNumber();

		// remember the up or down arrow depending on the rising/falling signal edge
		b.mMarkers[num_bits] = mSck->GetSampleNumber();
		b.mMarkerSCK[num_bits] = sample_first_bit_on_falling_edge ? AnalyzerResults::DownArrow : AnalyzerResults::UpArrow;
		b.mMarkerMISO[num_bits] = mMiso->GetBitState() == BIT_HIGH ? AnalyzerResults::One : AnalyzerResults::Zero;
		b.mMarkerMOSI[num_bits] = mMosi->GetBitState() == BIT_HIGH ? AnalyzerResults::One : AnalyzerResults::Zero;

		b.mValMiso = (b.mValMiso << 1) | (mMiso->GetBitState() == BIT_HIGH ? 1 : 0);
		b.mValMosi = (b.mValMosi << 1) | (mMosi->GetBitState() == BIT_HIGH ? 1 : 0);

		num_bits++;

		// advance to SCK falling edge
		if (!sample_first_bit_on_falling_edge  &&  !AdvanceSck(csn_edge))
			return false;

		if (num_bits == 8)
			break;

		// advance to SCK rising edge
		if (!AdvanceSck(csn_edge))
			return false;

		sample_first_bit_on_falling_edge = false;
	}

	b.mEndingSample = mSck->GetSampleNumber();

	return true;
}

void nRF24L01_Analyzer::WorkerThread()
{
	// create the results object
	mResults.reset(new nRF24L01_AnalyzerResults(this, &mSettings));
	SetAnalyzerResults(mResults.get());

	// init the channels
	mResults->AddChannelBubblesWillAppearOn(mSettings.mMosiChannel);
	mResults->AddChannelBubblesWillAppearOn(mSettings.mMisoChannel);

	mMiso = GetAnalyzerChannelData(mSettings.mMisoChannel);
	mMosi = GetAnalyzerChannelData(mSettings.mMosiChannel);
	mSck = GetAnalyzerChannelData(mSettings.mSckChannel);
	mCsn = GetAnalyzerChannelData(mSettings.mCsnChannel);

	std::vector<SpiByte> spi_bytes;
	U64 cmdStart, cmdEnd;
	for (;;)
	{
		// find the falling edge of CSN
		mCsn->AdvanceToNextEdge();
		if (mCsn->GetBitState() == BIT_HIGH)
			mCsn->AdvanceToNextEdge();

		// advance all the others here too
		SyncToChannel(mCsn);

		// remember in case we have to put a marker
		cmdStart = mCsn->GetSampleNumber();

		// get a command
		SpiByte b;
		bool is_first = true;
		spi_bytes.clear();
		while (GetByte(b, is_first))
		{
			// 33 bytes is the longest valid command according to the specs
			if (spi_bytes.size() < 34)
				spi_bytes.push_back(b);

			is_first = false;
		}

		cmdEnd = mCsn->GetSampleNumber();

		// decode the command
		// this creates separate command and data frames
		bool created = mResults->CreateFramesFromSpiBytes(spi_bytes, cmdStart, cmdEnd);

		// add the markers if frames were created
		if (created)
		{
			int num_bits, markerType = 0;
			std::vector<SpiByte>::const_iterator spi_i;
			for (spi_i = spi_bytes.begin(); spi_i != spi_bytes.end(); ++spi_i)
			{
				for (num_bits = 0; num_bits < 8; ++num_bits)
				{
					mResults->AddMarker(spi_i->mMarkers[num_bits], spi_i->mMarkerSCK[num_bits], mSettings.mSckChannel);

					mResults->AddMarker(spi_i->mMarkers[num_bits], spi_i->mMarkerMOSI[num_bits], mSettings.mMosiChannel);
					mResults->AddMarker(spi_i->mMarkers[num_bits], spi_i->mMarkerMISO[num_bits], mSettings.mMisoChannel);
				}
			}

			mResults->AddMarker(cmdStart, AnalyzerResults::Start, mSettings.mCsnChannel);
			mResults->AddMarker(cmdEnd, AnalyzerResults::Stop, mSettings.mCsnChannel);
		}

		// update progress bar
		ReportProgress(mSck->GetSampleNumber());
	}
}

bool nRF24L01_Analyzer::NeedsRerun()
{
	return false;
}

U32 nRF24L01_Analyzer::GenerateSimulationData(U64 minimum_sample_index, U32 device_sample_rate, SimulationChannelDescriptor** simulation_channels)
{
	if (!mSimulationInitilized)
	{
		mSimulationDataGenerator.Initialize(GetSimulationSampleRate(), &mSettings);
		mSimulationInitilized = true;
	}

	return mSimulationDataGenerator.GenerateSimulationData(minimum_sample_index, device_sample_rate, simulation_channels);
}

U32 nRF24L01_Analyzer::GetMinimumSampleRateHz()
{
	// whatever
	return 9600;
}

const char* nRF24L01_Analyzer::GetAnalyzerName() const
{
	return ::GetAnalyzerName();
}

const char* GetAnalyzerName()
{
	return "nRF24L01";
}

Analyzer* CreateAnalyzer()
{
	return new nRF24L01_Analyzer();
}

void DestroyAnalyzer(Analyzer* analyzer)
{
	delete analyzer;
}
