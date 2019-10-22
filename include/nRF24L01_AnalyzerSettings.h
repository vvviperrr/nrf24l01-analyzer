#pragma once

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class nRF24L01_AnalyzerSettings : public AnalyzerSettings
{
public:
	nRF24L01_AnalyzerSettings();
	virtual ~nRF24L01_AnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	virtual void LoadSettings(const char* settings);
	virtual const char* SaveSettings();

	void UpdateInterfacesFromSettings();

	Channel		mMosiChannel;
	Channel		mMisoChannel;
	Channel		mSckChannel;
	Channel		mCsnChannel;

	//bool		mMarkBits;
	//bool		mMarkStartEnd;

protected:
	AnalyzerSettingInterfaceChannel		mMosiChannelInterface;
	AnalyzerSettingInterfaceChannel		mMisoChannelInterface;
	AnalyzerSettingInterfaceChannel		mSckChannelInterface;
	AnalyzerSettingInterfaceChannel		mCsnChannelInterface;

	//AnalyzerSettingInterfaceBool		mMarkBitsInterface;
	//AnalyzerSettingInterfaceBool		mMarkStartEndInterface;
};
