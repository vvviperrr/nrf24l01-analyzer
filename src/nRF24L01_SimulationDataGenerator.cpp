#include "nRF24L01_SimulationDataGenerator.h"
#include "nRF24L01_AnalyzerSettings.h"

#include "nRFTypes.h"

#define SPACE_COMMAND		12
#define SPACE_CYCLE			48

nRF24L01_SimulationDataGenerator::nRF24L01_SimulationDataGenerator()
{
}

nRF24L01_SimulationDataGenerator::~nRF24L01_SimulationDataGenerator()
{
}

void nRF24L01_SimulationDataGenerator::Initialize(U32 simulation_sample_rate, nRF24L01_AnalyzerSettings* settings)
{
	mSimulationSampleRateHz = simulation_sample_rate;
	mSettings = settings;

	mClockGenerator.Init(simulation_sample_rate / 10, simulation_sample_rate);

	if (settings->mMisoChannel != UNDEFINED_CHANNEL)
		mMiso = mSpiSimulationChannels.Add( settings->mMisoChannel, mSimulationSampleRateHz, BIT_LOW );
	else
		mMiso = NULL;
	
	if (settings->mMosiChannel != UNDEFINED_CHANNEL)
		mMosi = mSpiSimulationChannels.Add( settings->mMosiChannel, mSimulationSampleRateHz, BIT_LOW );
	else
		mMosi = NULL;

	mSck = mSpiSimulationChannels.Add(settings->mSckChannel, mSimulationSampleRateHz, BIT_LOW);

	if (settings->mCsnChannel != UNDEFINED_CHANNEL)
		mCsn = mSpiSimulationChannels.Add(settings->mCsnChannel, mSimulationSampleRateHz, BIT_HIGH);
	else
		mCsn = NULL;

	mSpiSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(10));			// insert 10 bit-periods of idle
}

U32 nRF24L01_SimulationDataGenerator::GenerateSimulationData(U64 largest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels)
{
	U64 adjusted_largest_sample_requested = AnalyzerHelpers::AdjustSimulationTargetSample(largest_sample_requested, sample_rate, mSimulationSampleRateHz);

	while (mSck->GetCurrentSampleNumber() < adjusted_largest_sample_requested)
		CreateNRFTransaction();

	*simulation_channels = mSpiSimulationChannels.GetArray();

	return mSpiSimulationChannels.GetCount();
}

void nRF24L01_SimulationDataGenerator::CreateNRFTransaction()
{
	int c;

	if (mCsn != NULL)
		mCsn->Transition();

	mSpiSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(SPACE_COMMAND));
	
	OutputWord(0xFF, 0x0E);		// NOP
	OutputWord(0xFF, 0x0E);		// this one causes an MCU error message

	NewCommand();

	OutputWord(0x50, 0x0E);		// ACTIVATE
	OutputWord(0x73, 0x00);		// the activate command

	NewCommand();

	OutputWord(0xE2, 0x0E);		// FLUSH_RX

	NewCommand();

	OutputWord(0x2A, 0x0E);		// W_REGISTER RX_ADDR_P0
	OutputWord(0xE7, 0x00);
	OutputWord(0xE8, 0x00);
	OutputWord(0xE9, 0x00);
	OutputWord(0xE0, 0x00);
	OutputWord(0xE1, 0x00);

	NewCommand();

	OutputWord(0x21, 0x0E);		// W_REGISTER EN_AA
	OutputWord(0x01, 0x00);		// EN_AA_P0

	NewCommand();

	OutputWord(0x20, 0x0E);		// W_REGISTER CONFIG
	OutputWord(0x0F, 0x00);		// EN_CRC | CRC0 | PWR_UP | PRIM_RX

	NewCommand();

	OutputWord(0xA8, 0x0E);		// W_ACK_PAYLOAD
	for (c = 0; c < 12; c++)
		OutputWord(14 - c, 0x00);	// ACK payload data

	NewCommand();
	
	OutputWord(0x60, 0x0E);		// R_RX_PL_WID
	OutputWord(0x00, 0x08);

	NewCommand();

	// read RX payload
	OutputWord(0x61, 0x40);		// R_RX_PAYLOAD

	for (c = 0; c < 8; c++)
		OutputWord(0x00, c);	// RX payload data

	NewCommand();

	// switch to TX mode
	OutputWord(0x20, 0x0E);		// W_REGISTER CONFIG
	OutputWord(0x0E, 0x00);		// EN_CRC | CRC0 | PWR_UP

	NewCommand();

	OutputWord(0xE1, 0x0E);		// FLUSH_TX

	NewCommand();

	OutputWord(0xA0, 0x0E);		// W_TX_PAYLOAD
	for (c = 0; c < 10; c++)
		OutputWord(10 - c, 0x00);	// TX payload data

	NewCommand();

	OutputWord(0xE3, 0x0E);		// REUSE_TX_PL

	NewCommand();

	OutputWord(0xB0, 0x0E);		// W_TX_PAYLOAD_NOACK
	for (c = 0; c < 6; c++)
		OutputWord(10 + c, 0x00);


	if (mCsn != NULL)
		mCsn->Transition();

	mSpiSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(SPACE_CYCLE));
}

void nRF24L01_SimulationDataGenerator::NewCommand()
{
	// CSN goes high
	if (mCsn != NULL)
		mCsn->Transition();

	// a short pause
	mSpiSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(SPACE_COMMAND));

	// CSN goes low
	if (mCsn != NULL)
		mCsn->Transition();
}

void nRF24L01_SimulationDataGenerator::OutputWord(U64 mosi_data, U64 miso_data)
{
	BitExtractor mosi_bits(mosi_data, AnalyzerEnums::MsbFirst, 8);
	BitExtractor miso_bits(miso_data, AnalyzerEnums::MsbFirst, 8);

	U32 count = 8;
	for (U32 i = 0; i < count; i++)
	{
		if (mMosi != NULL)
			mMosi->TransitionIfNeeded(mosi_bits.GetNextBit());

		if (mMiso != NULL)
			mMiso->TransitionIfNeeded(miso_bits.GetNextBit());

		mSpiSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod(.5));
		mSck->Transition();  //data valid

		mSpiSimulationChannels.AdvanceAll( mClockGenerator.AdvanceByHalfPeriod(.5));
		mSck->Transition();  //data invalid
	}

	if (mMosi != NULL)
		mMosi->TransitionIfNeeded(BIT_LOW);

	if (mMiso != NULL)
		mMiso->TransitionIfNeeded(BIT_LOW);

	mSpiSimulationChannels.AdvanceAll(mClockGenerator.AdvanceByHalfPeriod(2));
}
