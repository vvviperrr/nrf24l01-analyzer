#pragma once

#include <LogicPublicTypes.h>
#include <AnalyzerResults.h>

#include <string>
#include <vector>

enum nRFFrameFlags
{
	IS_COMMAND			= (1 << 0),
	IS_EXTENDED			= (1 << 1),
	IS_DATA_ON_MISO		= (1 << 2),
	HAS_DATA_FRAME		= (1 << 3),
};

struct SpiByte
{
	U8		mValMiso;
	U8		mValMosi;
	U64		mStartingSample;
	U64		mEndingSample;

	// the markers for this command
	U64								mMarkers[8];
	AnalyzerResults::MarkerType		mMarkerSCK[8];
	AnalyzerResults::MarkerType		mMarkerMOSI[8];
	AnalyzerResults::MarkerType		mMarkerMISO[8];

	void Clear()
	{
		//mValMiso = mValMosi = 0;
		//mStartingSample = mEndingSample = 0;
		memset(this, 0, sizeof(*this));
	}
};

enum nRFCommand_e
{
	R_REGISTER,
	W_REGISTER,
	R_RX_PAYLOAD,
	W_TX_PAYLOAD,
	FLUSH_TX,
	FLUSH_RX,
	ACTIVATE,
	REUSE_TX_PL,
	R_RX_PL_WID,
	W_ACK_PAYLOAD,
	W_TX_PAYLOAD_NOACK,
	NOP,

	undefined_cmd
};

enum nRFRegister_e
{
	CONFIG		= 0x00,
	EN_AA		= 0x01,
	EN_RXADDR	= 0x02,
	SETUP_AW	= 0x03,
	SETUP_RETR	= 0x04,
	RF_CH		= 0x05,
	RF_SETUP	= 0x06,
	STATUS		= 0x07,
	OBSERVE_TX	= 0x08,
	CD			= 0x09,
	RX_ADDR_P0	= 0x0A,
	RX_ADDR_P1	= 0x0B,
	RX_ADDR_P2	= 0x0C,
	RX_ADDR_P3	= 0x0D,
	RX_ADDR_P4	= 0x0E,
	RX_ADDR_P5	= 0x0F,
	TX_ADDR		= 0x10,
	RX_PW_P0	= 0x11,
	RX_PW_P1	= 0x12,
	RX_PW_P2	= 0x13,
	RX_PW_P3	= 0x14,
	RX_PW_P4	= 0x15,
	RX_PW_P5	= 0x16,
	FIFO_STATUS	= 0x17,

	DYNPD		= 0x1C,
	FEATURE		= 0x1D,

	reg_mask	= 0x1F,		// not really a register

	undefined_reg = 0xff,
};

struct nRFCommandDesc
{
	std::vector<std::string>		texts;
	bool							is_error;
	std::vector<U64>				dot_ndxs;
};

struct nRFCommand
{
	U8				mCommandByte;
	U8				mData[32];
	U8				mDataLength;
	U8				mStatus;

	nRFCommand_e	mCommand;
	nRFRegister_e	mRegister;

	nRFCommand()
	{
		Clear();
	}

	void Clear()
	{
		mCommandByte = mDataLength = mStatus = 0;
		::memset(mData, 0, sizeof(mData));

		mCommand = undefined_cmd;
		mRegister = undefined_reg;
	}

	void Decode(const Frame* frmCmd, const Frame* frmData, const std::vector<U8>& extendedData);

	bool IsRegister() const
	{
		return mCommand == W_REGISTER  ||  mCommand == R_REGISTER;
	}

	bool IsRead() const
	{
		return mCommand == R_REGISTER  ||  mCommand == R_RX_PAYLOAD  ||  mCommand == R_RX_PL_WID;
	}

	bool HasDataPayload() const
	{
		return mCommand == W_TX_PAYLOAD  
				||  mCommand == R_RX_PAYLOAD  
				||  mCommand == W_ACK_PAYLOAD
				||  mCommand == W_TX_PAYLOAD_NOACK;
	}

	bool HasAddr() const
	{
		return IsRegister()  &&  mRegister >= RX_ADDR_P0  &&  mRegister <= TX_ADDR;
	}

	bool HasData() const
	{
		return !(mCommand == FLUSH_TX  ||  mCommand == FLUSH_RX  ||  mCommand == REUSE_TX_PL  ||  mCommand == NOP);
	}

	void GetCommandText(const bool is_mosi, std::vector<std::string>& texts, DisplayBase display_base);
	void GetDataText(const bool is_mosi, std::vector<std::string>& texts, DisplayBase display_base);
	std::string GetRegisterString();

	// helpers
	static const char* GetCommandName(const nRFCommand_e cmd);
	static const char* GetRegisterName(U64 cmd_byte);
	static nRFCommand_e GetCommandFromByte(U64 cmd_byte);
	static std::string GetStatusBits(U8 stat);
	static std::string GetHighBits(U8 bits, nRFRegister_e reg);
};
