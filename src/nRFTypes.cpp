#include <AnalyzerHelpers.h>

#include <algorithm>

#include "nRFTypes.h"
#include "utils.h"


inline void AddPart(std::string& s, const char* p)
{
	if (!s.empty())
		s += ", ";

	s += p;
}

inline void AddPart(std::string& s, const std::string& p)
{
	AddPart(s, p.c_str());
}

void nRFCommand::Decode(const Frame* frmCmd, const Frame* frmData, const std::vector<U8>& extendedData)
{
	Clear();

	mCommandByte = U8(frmCmd->mData1);
	mCommand = GetCommandFromByte(mCommandByte);
	mStatus = U8(frmCmd->mData2);

	if (IsRegister())
		mRegister = (nRFRegister_e) (frmCmd->mData1 & reg_mask);

	// copy the data if we have any
	if (HasData())
	{
		// copy the data
		mDataLength = frmData->mType;

		// extended data frame?
		if (frmData->mFlags & IS_EXTENDED)
		{
			// copy the data from the extendedData into this->mData
			std::copy(	extendedData.begin() + size_t(frmData->mData1),
						extendedData.begin() + size_t(frmData->mData1) + mDataLength,
						mData);

		} else {

			// copy the data from frmData->mData1 and frmData->mData2 into this->mData
			int cnt = 0, data_len = mDataLength;

			U8* pData = (U8*) &frmData->mData1;
			U8* pWrite = mData;
			while (data_len > 0)
			{
				*pWrite = pData[cnt];

				++cnt;
				--data_len;
				++pWrite;

				if (cnt == sizeof(frmData->mData1))
				{
					pData = (U8*) &frmData->mData2;
					cnt = 0;
				}
			}
		}
	}
}

void nRFCommand::GetCommandText(const bool is_mosi, std::vector<std::string>& texts, DisplayBase display_base)
{
	texts.clear();

	if (is_mosi)
	{
		std::string cmdByteStr(int2str_sal(mCommandByte, display_base));
		std::string cmdName(GetCommandName(mCommand));
		std::string shortCmdName;

		if (IsRegister())
		{
			std::string regName(GetRegisterName(mRegister));
			shortCmdName = cmdName.substr(0, 5);

			texts.push_back("(" + cmdByteStr + ") " + cmdName + " " + regName);
			texts.push_back(shortCmdName);
			texts.push_back(shortCmdName + " " + regName);
		}

		if (mCommand == W_ACK_PAYLOAD)
			cmdName += " " + int2str_sal(mCommandByte & 7, display_base);
		
		texts.push_back("(" + cmdByteStr + ") " + cmdName);
		texts.push_back(cmdName);
		texts.push_back(cmdByteStr);

	} else {

		std::string statusByteStr(int2str_sal(mStatus, display_base));

		// the STATUS register
		std::string status_bits(GetStatusBits(mStatus));
		texts.push_back("(" + statusByteStr + ") " + status_bits);
		texts.push_back(status_bits);
		texts.push_back("STATUS = (" + statusByteStr + ") " + status_bits);
		texts.push_back(statusByteStr);
	}
}

void nRFCommand::GetDataText(const bool is_mosi, std::vector<std::string>& texts, DisplayBase display_base)
{
	texts.clear();

	if (!HasData())
	{
		texts.push_back("");
		texts.push_back("err");
		texts.push_back("MCU error");
		texts.push_back("MCU error. Command has no data");

	} else if (is_mosi  ==  !IsRead()) {

		if (HasDataPayload()  ||  HasAddr())
		{
			// make the data string
			std::string strData;

			char buff[32];
			for (int cnt = 0; cnt < mDataLength; ++cnt)
			{
				AnalyzerHelpers::GetNumberString(mData[cnt], display_base, 8, buff, sizeof(buff));
				if (cnt == 0)
				{
					strData += buff;
				} else if (display_base == Hexadecimal  ||  display_base == Binary) {
					strData += buff + 2;
				} else {		// Decimal, AsciiHex, ASCII
					strData += " ";
					strData += buff;
				}
			}

			// get the length
			std::string strDataLen("(" + int2str(mDataLength) + ")");

			texts.push_back(strData + " " + strDataLen);
			texts.push_back(strData);
			texts.push_back(strDataLen);

		} else if (mCommand == ACTIVATE  ||  mCommand == R_RX_PL_WID) {

			texts.push_back(int2str_sal(mData[0], display_base));

		} else if (IsRegister()) {

			std::string regByteStr(int2str_sal(*mData, display_base));
			std::string regStr(GetRegisterString());

			texts.push_back("(" + regByteStr + ") " + regStr);
			texts.push_back(regStr);
			texts.push_back(regByteStr);
		}
	}
}

std::string nRFCommand::GetStatusBits(U8 stat)
{
	std::string ret_val(GetHighBits(stat, STATUS));

	U8 pipe = (stat >> 1) & 7;
	if (pipe < 7)
		AddPart(ret_val, "RX_P_NO=" + int2str(pipe));
	else
		AddPart(ret_val, "RX_FIFO empty");

	return ret_val;
}

struct BitName
{
	const char*		mBitName;
	int				mBitNum;
};

BitName CONFIG_BN[] = 
{
	{"MASK_RX_DR",	6},
	{"MASK_TX_DS",	5},
	{"MASK_MAX_RT",	4},
	{"EN_CRC",		3},
	{"CRCO",		2},
	{"PWR_UP",		1},
	{"PRIM_RX",		0},
	{NULL, -1},
};

BitName EN_AA_BN[] = {
	{"ENAA_P5",	5},
	{"ENAA_P4",	4},
	{"ENAA_P3",	3},
	{"ENAA_P2",	2},
	{"ENAA_P1",	1},
	{"ENAA_P0",	0},
	{NULL, -1},
};

BitName EN_RXADDR_BN[] = {
	{"ERX_P5",	5},
	{"ERX_P4",	4},
	{"ERX_P3",	3},
	{"ERX_P2",	2},
	{"ERX_P1",	1},
	{"ERX_P0",	0},
	{NULL, -1},
};

BitName RF_SETUP_BN[] = {
	{"CONT_WAVE",	7},
	{"RF_DR_LOW",	5},
	{"PLL_LOCK",	4},
	{"RF_DR_HIGH",	3},
	{"LNA_HCURR",	0},
	{NULL, -1},
};

BitName STATUS_BN[] = {
	{"RX_DR",		6},
	{"TX_DS",		5},
	{"MAX_RT",		4},
	{"TX_FULL",		0},
	{NULL, -1},
};

BitName CD_BN[] = {
	{"CD",		0},
	{NULL, -1},
};

BitName FIFO_STATUS_BN[] = {
	{"TX_REUSE",	6},
	{"TX_FULL",		5},
	{"TX_EMPTY",	4},
	{"RX_FULL",		1},
	{"RX_EMPTY",	0},
	{NULL, -1},
};

BitName DYNPD_BN[] = {
	{"DPL_P5",	6},
	{"DPL_P5",	5},
	{"DPL_P4",	4},
	{"DPL_P3",	3},
	{"DPL_P2",	2},
	{"DPL_P1",	1},
	{"DPL_P0",	0},
	{NULL, -1},
};

BitName FEATURE_BN[] = {
	{"EN_DPL",		2},
	{"EN_ACK_PAY",	1},
	{"EN_DYN_ACK",	0},
	{NULL, -1},
};

BitName* AllRegisters[] = {
	CONFIG_BN,			// CONFIG		
	EN_AA_BN,			// EN_AA		
	EN_RXADDR_BN,		// EN_RXADDR	
	NULL,				// SETUP_AW	
	NULL,				// SETUP_RETR	
	NULL,				// RF_CH		
	RF_SETUP_BN,		// RF_SETUP	
	STATUS_BN,			// STATUS		
	NULL,				// OBSERVE_TX	
	CD_BN,				// CD			
	NULL,				// RX_ADDR_P0	
	NULL,				// RX_ADDR_P1	
	NULL,				// RX_ADDR_P2	
	NULL,				// RX_ADDR_P3	
	NULL,				// RX_ADDR_P4	
	NULL,				// RX_ADDR_P5	
	NULL,				// TX_ADDR		
	NULL,				// RX_PW_P0	
	NULL,				// RX_PW_P1	
	NULL,				// RX_PW_P2	
	NULL,				// RX_PW_P3	
	NULL,				// RX_PW_P4	
	NULL,				// RX_PW_P5
	FIFO_STATUS_BN,		// FIFO_STATUS
	NULL,				// 0x18
	NULL,				// 0x19
	NULL,				// 0x1A
	NULL,				// 0x1B
	DYNPD_BN,			// 0x1C
	FEATURE_BN,			// 0x1D
};

std::string nRFCommand::GetHighBits(U8 bits, nRFRegister_e reg)
{
	std::string ret_val;

	BitName* iBN = AllRegisters[reg];
	while (iBN->mBitName != NULL)
	{
		// is the bit set?
		if ((bits & (1 << iBN->mBitNum)) != 0)
			AddPart(ret_val, iBN->mBitName);

		++iBN;
	}

	return ret_val;
}

std::string nRFCommand::GetRegisterString()
{
	std::string ret_val("***");

	if (mRegister == SETUP_AW)
	{
		switch (*mData)
		{
		case 1:		ret_val = "AW=3 bytes";			break;
		case 2:		ret_val = "AW=4 bytes";			break;
		case 3:		ret_val = "AW=5 bytes";			break;
		default:	ret_val = "<SETUP_AW ERROR>";	break;
 		}

	} else if (mRegister == SETUP_RETR) {
		
		ret_val = "ARD=" + int2str((((*mData) >> 4) + 1) * 250) + "us";
		ret_val += ", ARC=";

		U8 arc = (*mData) & 0xf;
		if (arc == 0)
			ret_val += "Disabled";
		else
			ret_val += int2str(arc) + " retransmit";

	} else if (mRegister == RF_CH) {

		ret_val = "channel=" + int2str(*mData);

	} else if (mRegister == RF_SETUP) {

		ret_val = GetHighBits(*mData, mRegister);

		AddPart(ret_val, "RF_PWR=");

		switch ((*mData >> 1) & 3)
		{
		case 0:		ret_val += "-18dBm";	break;
		case 1:		ret_val += "-12dBm";	break;
		case 2:		ret_val += "-6dBm";		break;
		case 3:		ret_val += "0dBm";		break;
		}

	} else if (mRegister == STATUS) {

		ret_val = GetStatusBits(*mData);

	} else if (mRegister == OBSERVE_TX) {

		U8 plos_cnt = (*mData) >> 4;
		U8 arc_cnt = (*mData) & 7;

		ret_val = "PLOS_CNT=" + int2str(plos_cnt);
		ret_val += " ARC_CNT=" + int2str(arc_cnt);

	} else if (mRegister >= RX_PW_P0  &&  mRegister <= RX_PW_P5) {

		ret_val = GetRegisterName(mRegister);
		ret_val += "=" + int2str((*mData) & 0x1f);
		
	} else if (mRegister <= FEATURE  &&  AllRegisters[mRegister] != NULL) {

		ret_val = GetHighBits(*mData, mRegister);

	} else {
		ret_val = "<register error>";
	}

	return ret_val;
}

nRFCommand_e nRFCommand::GetCommandFromByte(U64 cmd_byte)
{
	U8 highest_3_bits = U8(cmd_byte) >> 5;
	nRFCommand_e cmd = undefined_cmd;

	if (highest_3_bits < 2)
		cmd = highest_3_bits == 0 ? R_REGISTER : W_REGISTER;
	else if (cmd_byte == 0x61)
		cmd = R_RX_PAYLOAD;
	else if (cmd_byte == 0xA0)
		cmd = W_TX_PAYLOAD;
	else if (cmd_byte == 0xE1)
		cmd = FLUSH_TX;
	else if (cmd_byte == 0xE2)
		cmd = FLUSH_RX;
	else if (cmd_byte == 0xE3)
		cmd = REUSE_TX_PL;
	else if (cmd_byte == 0x50)
		cmd = ACTIVATE;
	else if (cmd_byte == 0x60)
		cmd = R_RX_PL_WID;
	else if ((cmd_byte >> 3) == 0x15)
		cmd = W_ACK_PAYLOAD;
	else if (cmd_byte == 0xB0)
		cmd = W_TX_PAYLOAD_NOACK;
	else if (cmd_byte == 0xFF)
		cmd = NOP;

	return cmd;
}

#define ID2NAME(id)		case id: return #id

const char* nRFCommand::GetRegisterName(U64 cmd_byte)
{
	switch (cmd_byte & 0x1F)
	{
	ID2NAME(CONFIG);
	ID2NAME(EN_AA);
	ID2NAME(EN_RXADDR);
	ID2NAME(SETUP_AW);
	ID2NAME(SETUP_RETR);
	ID2NAME(RF_CH);
	ID2NAME(RF_SETUP);
	ID2NAME(STATUS);
	ID2NAME(OBSERVE_TX);
	ID2NAME(CD);
	ID2NAME(RX_ADDR_P0);
	ID2NAME(RX_ADDR_P1);
	ID2NAME(RX_ADDR_P2);
	ID2NAME(RX_ADDR_P3);
	ID2NAME(RX_ADDR_P4);
	ID2NAME(RX_ADDR_P5);
	ID2NAME(TX_ADDR);
	ID2NAME(RX_PW_P0);
	ID2NAME(RX_PW_P1);
	ID2NAME(RX_PW_P2);
	ID2NAME(RX_PW_P3);
	ID2NAME(RX_PW_P4);
	ID2NAME(RX_PW_P5);
	ID2NAME(FIFO_STATUS);
	ID2NAME(DYNPD);
	ID2NAME(FEATURE);
	}

	return "<undef>";
}

const char* nRFCommand::GetCommandName(const nRFCommand_e cmd)
{
	switch (cmd)
	{
	ID2NAME(R_REGISTER);
	ID2NAME(W_REGISTER);
	ID2NAME(R_RX_PAYLOAD);
	ID2NAME(W_TX_PAYLOAD);
	ID2NAME(FLUSH_TX);
	ID2NAME(FLUSH_RX);
	ID2NAME(REUSE_TX_PL);
	ID2NAME(ACTIVATE);
	ID2NAME(R_RX_PL_WID);
	ID2NAME(W_ACK_PAYLOAD);
	ID2NAME(W_TX_PAYLOAD_NOACK);
	ID2NAME(NOP);
	}

	return "<undef>";
}

