#pragma once

#include <JuceHeader.h>
#include "AudioFreqParams.h"
#include "WAVDataAnalyzer.h"

class VoiceVerifierComponent : public juce::Component
{
public:
	VoiceVerifierComponent();
	~VoiceVerifierComponent() override = default;

	void paint(juce::Graphics& g) override;
	void resized() override;

private:
	juce::TextButton referenceVoiceButton;
	juce::TextButton verifyVoiceButton;

	juce::Label referenceFileLabel;
	juce::Label verifyFileLabel;
	juce::Label resultLabel;
	juce::Label distanceValueLabel;

	std::unique_ptr<juce::FileChooser> fileChooser;

	juce::AudioBuffer<float> referenceAudioData;
	juce::AudioBuffer<float> verifyAudioData;

	int referenceSampleRate = 0;
	int verifySampleRate = 0;

	bool hasReferenceVoice = false;
	bool hasVerifyVoice = false;

	AudioFreqParams audioFreqParamsAnalyzer;

	std::vector<float> getMFCCParams(const juce::AudioBuffer<float>& audioData, int sampleRate);
	float getDistance(const std::vector<float>& a, const std::vector<float>& b);

	void chooseReferenceVoice();
	void chooseVerifyVoice();
	void verifyVoice();
};