#include <iostream>
#include <fstream>
#include <assert.h>

#include <AnalyzerHelpers.h>

#include "utils.h"
#include "nRF24L01_AnalyzerResults.h"
#include "nRF24L01_Analyzer.h"
#include "nRF24L01_AnalyzerSettings.h"

nRF24L01_AnalyzerResults::nRF24L01_AnalyzerResults(nRF24L01_Analyzer* analyzer, nRF24L01_AnalyzerSettings* settings) :
	mSettings(settings),
	mAnalyzer(analyzer),
	mCommandWordFrameIndex(0xFFFFFFFFFFFFFFFFLL)
{}

nRF24L01_AnalyzerResults::~nRF24L01_AnalyzerResults()
{}

void nRF24L01_AnalyzerResults::GenerateBubbleText(U64 frame_index, Channel& channel, DisplayBase display_base)
{
	ClearResultStrings();
	Frame f = GetFrame(frame_index);

	// create the command object if we're on a new command
	if (frame_index != mCommandWordFrameIndex)
	{
		U64 cmd_frame_index = 0;
		Frame fOther;
		Frame* pFCmd = 0;
		Frame* pFData = 0;

		if (f.mFlags & IS_COMMAND)
		{
			fOther = GetFrame(frame_index + 1);

			pFCmd = &f;
			pFData = &fOther;

			cmd_frame_index = frame_index;
		} else {
			fOther = GetFrame(frame_index - 1);

			pFCmd = &fOther;
			pFData = &f;

			cmd_frame_index = frame_index - 1;
		}

		if (cmd_frame_index != mCommandWordFrameIndex)
		{
			mCommand.Decode(pFCmd, pFData, mExtendedData);
			mCommandWordFrameIndex = cmd_frame_index;
		}
	}

	std::vector<std::string> texts;

	if (f.mFlags & IS_COMMAND)
		mCommand.GetCommandText(channel == mSettings->mMosiChannel, texts, display_base);
	else
		mCommand.GetDataText(channel == mSettings->mMosiChannel, texts, display_base);

	for (std::vector<std::string>::iterator it(texts.begin()); it != texts.end(); ++it)
		AddResultString(it->c_str());
}

void nRF24L01_AnalyzerResults::GenerateExportFile(const char* file, DisplayBase display_base, U32 export_type_user_id)
{
	std::ofstream file_stream( file, std::ios::out );

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	file_stream << "Time [s];Status;Command;Data" << std::endl;

	U64 num_frames = GetNumFrames();
	Frame cmd_frame, data_frame;
	nRFCommand cmd;
	std::vector<std::string> texts;
	std::string commandText, statusText, dataText;
	bool has_data;
	for (U64 fcnt = 0; fcnt < num_frames; fcnt++)
	{
		// get the command frame
		cmd_frame = GetFrame(fcnt);

		// do we have a data frame as well?
		has_data = (cmd_frame.mFlags & HAS_DATA_FRAME) != 0;
		if (has_data)
			data_frame = GetFrame(fcnt + 1);

		// decode the frames
		cmd.Decode(&cmd_frame, &data_frame, mExtendedData);

		char time_str[128];
		AnalyzerHelpers::GetTimeString(cmd_frame.mStartingSampleInclusive, trigger_sample, sample_rate, time_str, sizeof(time_str));

		cmd.GetCommandText(false, texts, display_base);
		statusText = texts.front(); 

		cmd.GetCommandText(true, texts, display_base);
		commandText = texts.front();

		dataText.clear();
		if (has_data)
		{
			cmd.GetDataText(true, texts, display_base);
			if (texts.empty())
				cmd.GetDataText(false, texts, display_base);

			if (!texts.empty())
				dataText = texts.front();
		}

		file_stream << time_str << ";" << statusText << ";" << commandText << ";" << dataText << std::endl;

		if (UpdateExportProgressAndCheckForCancel(fcnt, num_frames))
			return;

		// skip the data frame
		if (has_data)
			fcnt++;
	}

	// end
	UpdateExportProgressAndCheckForCancel(num_frames, num_frames);
}

void nRF24L01_AnalyzerResults::GenerateFrameTabularText(U64 frame_index, DisplayBase display_base)
{
	Frame frame = GetFrame(frame_index);
	ClearResultStrings();

	char number_str[128];
	AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
	AddResultString( number_str );
}

void nRF24L01_AnalyzerResults::GeneratePacketTabularText(U64 packet_id, DisplayBase display_base)
{
	ClearResultStrings();
	AddResultString("not supported");
}

void nRF24L01_AnalyzerResults::GenerateTransactionTabularText(U64 transaction_id, DisplayBase display_base)
{
	ClearResultStrings();
	AddResultString("not supported");
}

bool nRF24L01_AnalyzerResults::CreateFramesFromSpiBytes(const std::vector<SpiByte>& spi_bytes, U64 csnLow, U64 csnHi)
{
	if (spi_bytes.empty()  ||  spi_bytes.size() > 33)
		return false;

	// make the command frame
	Frame frmCmd;
	const SpiByte& command_byte(spi_bytes.front());
	frmCmd.mData1 = command_byte.mValMosi;		// command byte
	frmCmd.mData2 = command_byte.mValMiso;		// STATUS register
	frmCmd.mStartingSampleInclusive = csnLow;
	frmCmd.mEndingSampleInclusive = command_byte.mEndingSample;
	frmCmd.mType = 0;
	frmCmd.mFlags = IS_COMMAND;

	std::vector<SpiByte>::const_iterator spi_i;

	// make the data frame
	Frame frmData;
	if (spi_bytes.size() > 1)
	{
		frmData.mStartingSampleInclusive = spi_bytes[1].mStartingSample;
		frmData.mEndingSampleInclusive = spi_bytes.back().mEndingSample;
		frmData.mFlags = 0;

		nRFCommand_e cmd = nRFCommand::GetCommandFromByte(command_byte.mValMosi);
		bool use_miso = (cmd == R_REGISTER  ||  cmd == R_RX_PAYLOAD  ||  cmd == R_RX_PL_WID);

		// do we have a data for a command which should have none?
		if (cmd != FLUSH_TX  &&  cmd != FLUSH_RX  &&  cmd != REUSE_TX_PL  &&  cmd != NOP)
			frmData.mFlags |= DISPLAY_AS_ERROR_FLAG;

		// do we have 16 or less data bytes?
		if (spi_bytes.size() < 17)
		{
			// store the data into mData1 and mData2 members of the Frame object
			frmData.mData1 = 0;
			frmData.mData2 = 0;

			U8* pData = (U8*) &frmData.mData1;

			int cnt = 0;
			spi_i = spi_bytes.begin() + 1;
			while (spi_i != spi_bytes.end())
			{
				pData[cnt] = (use_miso ? spi_i->mValMiso : spi_i->mValMosi);

				++cnt;
				++spi_i;

				// out of space on mData1?
				if (cnt == sizeof(frmData.mData1))
				{
					// move on to mData2
					cnt = 0;
					pData = (U8*) &frmData.mData2;
				}
			}

		} else {
			// The data payload is longer that 16 bytes.
			// In this case the data will be kept in mExtendedData,
			// and mData1 will hold the index of where the data begins in the vector.
			// We could also use mData2 to store the first 8 bytes of the payload
			// and save a little memory, but that would be a little too much.
			// Premature optimisation and all that....
			
			frmData.mData1 = mExtendedData.size();
			frmData.mData2 = 0;

			for (spi_i = spi_bytes.begin() + 1; spi_i != spi_bytes.end(); ++spi_i)
			{
				if (use_miso)
					mExtendedData.push_back(spi_i->mValMiso);
				else
					mExtendedData.push_back(spi_i->mValMosi);
			}

			frmData.mFlags |= IS_EXTENDED;
		}

		// write the length of the data part into the mType frame member
		frmData.mType = U8(spi_bytes.size() - 1);
		frmData.mFlags |= (use_miso ? IS_DATA_ON_MISO : 0);

		frmCmd.mFlags |= HAS_DATA_FRAME;
	}

	if (spi_bytes.size() == 1)
	{
		frmCmd.mEndingSampleInclusive = csnHi;
		AddFrame(frmCmd);
	} else {
		// extend the frame start/ends. makes the output a little nicer
		U64 cmd_end		= frmCmd.mEndingSampleInclusive;
		U64 data_start	= frmData.mStartingSampleInclusive;
		U64 middle		= (cmd_end + data_start) / 2;

		frmCmd.mEndingSampleInclusive = middle;
		frmData.mStartingSampleInclusive = middle;
		frmData.mEndingSampleInclusive = csnHi;

		AddFrame(frmCmd);
		AddFrame(frmData);
	}

	CommitResults();

	return true;
}
