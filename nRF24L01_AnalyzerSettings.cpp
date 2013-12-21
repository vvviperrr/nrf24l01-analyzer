#include <AnalyzerHelpers.h>

#include "utils.h"
#include "nRF24L01_AnalyzerSettings.h"

nRF24L01_AnalyzerSettings::nRF24L01_AnalyzerSettings()
:	mMosiChannel( UNDEFINED_CHANNEL ),
	mMisoChannel( UNDEFINED_CHANNEL ),
	mSckChannel( UNDEFINED_CHANNEL ),
	mCsnChannel( UNDEFINED_CHANNEL )/*,
	mMarkBits(true),
	mMarkStartEnd(true)	*/
{
	// init the interfaces
	mMosiChannelInterface.SetTitleAndTooltip( "MOSI", "uC Out, nRF24L01 In" );
	mMosiChannelInterface.SetChannel( mMosiChannel );
	//mMosiChannelInterface.SetSelectionOfNoneIsAllowed( true );

	mMisoChannelInterface.SetTitleAndTooltip( "MISO", "uC In, nRF24L01 Out" );
	mMisoChannelInterface.SetChannel( mMisoChannel );
	//mMisoChannelInterface.SetSelectionOfNoneIsAllowed( true );

	mSckChannelInterface.SetTitleAndTooltip( "SCK", "Slave clock (SCK)" );
	mSckChannelInterface.SetChannel( mSckChannel );

	mCsnChannelInterface.SetTitleAndTooltip( "CSN", "Chip select NOT" );
	mCsnChannelInterface.SetChannel( mCsnChannel );
	//mCsnChannelInterface.SetSelectionOfNoneIsAllowed( true );

	//mMarkBitsInterface.SetCheckBoxText("Mark 0/1 on MOSI and MISO");
	//mMarkStartEndInterface.SetCheckBoxText("Mark command start/end on CSN");

	// add the interfaces
	AddInterface( &mMosiChannelInterface );
	AddInterface( &mMisoChannelInterface );
	AddInterface( &mSckChannelInterface );
	AddInterface( &mCsnChannelInterface );
	//AddInterface( &mMarkBitsInterface );
	//AddInterface( &mMarkStartEndInterface );

	AddExportOption( 0, "Export as text/csv file" );
	AddExportExtension( 0, "text", "txt" );
	AddExportExtension( 0, "csv", "csv" );

	ClearChannels();

	AddChannel( mMosiChannel,	"MOSI",	false );
	AddChannel( mMisoChannel,	"MISO",	false );
	AddChannel( mSckChannel,	"SCK",	false );
	AddChannel( mCsnChannel,	"CSN",	false );
}

nRF24L01_AnalyzerSettings::~nRF24L01_AnalyzerSettings()
{
}

bool nRF24L01_AnalyzerSettings::SetSettingsFromInterfaces()
{
	const int NUM_CHANNELS = 4;

	Channel	all_channels[NUM_CHANNELS] = {mMosiChannelInterface.GetChannel(),
											mMisoChannelInterface.GetChannel(),
											mSckChannelInterface.GetChannel(),
											mCsnChannelInterface.GetChannel()};

	for (int ch = 0; ch < NUM_CHANNELS; ++ch)
	{
		if (all_channels[ch] == UNDEFINED_CHANNEL)
		{
			SetErrorText( "Please select inputs for all channels." );
			return false;
		}
	}

	if ( AnalyzerHelpers::DoChannelsOverlap(all_channels, 4) )
	{
		SetErrorText( "Please select different channels for each input." );
		return false;
	}

	mMosiChannel = all_channels[0];
	mMisoChannel = all_channels[1];
	mSckChannel = all_channels[2];
	mCsnChannel = all_channels[3];

	ClearChannels();

	AddChannel( mMosiChannel,	"MOSI",	true );
	AddChannel( mMisoChannel,	"MISO",	true );
	AddChannel( mSckChannel,	"SCK",	true );
	AddChannel( mCsnChannel,	"CSN",	true );

	//mMarkBits = mMarkBitsInterface.GetValue();
	//mMarkStartEnd = mMarkStartEndInterface.GetValue();

	return true;
}

void nRF24L01_AnalyzerSettings::UpdateInterfacesFromSettings()
{
	mMosiChannelInterface.SetChannel(mMosiChannel);
	mMisoChannelInterface.SetChannel(mMisoChannel);
	mSckChannelInterface.SetChannel(mSckChannel);
	mCsnChannelInterface.SetChannel(mCsnChannel);
	//mMarkBitsInterface.SetValue(mMarkBits);
	//mMarkStartEndInterface.SetValue(mMarkStartEnd);
}

void nRF24L01_AnalyzerSettings::LoadSettings( const char* settings )
{
	SimpleArchive text_archive;
	text_archive.SetString( settings );

	text_archive >> mMosiChannel;
	text_archive >> mMisoChannel;
	text_archive >> mSckChannel;
	text_archive >> mCsnChannel;
	//text_archive >> mMarkBits;
	//text_archive >> mMarkStartEnd;

	ClearChannels();

	AddChannel( mMosiChannel,	"MOSI",	true );
	AddChannel( mMisoChannel,	"MISO",	true );
	AddChannel( mSckChannel,	"SCK",	true );
	AddChannel( mCsnChannel,	"CSN",	true );

	UpdateInterfacesFromSettings();
}

const char* nRF24L01_AnalyzerSettings::SaveSettings()
{
	SimpleArchive text_archive;

	text_archive << mMosiChannel;
	text_archive << mMisoChannel;
	text_archive << mSckChannel;
	text_archive << mCsnChannel;
	//text_archive << mMarkBits;
	//text_archive << mMarkStartEnd;

	return SetReturnString( text_archive.GetString() );
}
