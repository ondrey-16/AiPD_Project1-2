#include <JuceHeader.h>

#pragma once
enum WINDOW_FUNCTION
{
	RECTANGLE = 1,
	TRIANGLE,
	HAMMING,
	VAN_HANN,
	BLACKMAN
};

static class AudioFreqParams
{
public:
	static std::vector<float> transformAudioDataByWindowFunction(juce::AudioBuffer<float> audioData, int* frameSize);
	static std::vector<float> getVolume(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<float> getCentroid(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<float> getBandwidth(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<std::vector<float>> getBandEnergyRatio(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<std::vector<float>> getSFM(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<std::vector<float>> getSCF(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<float> getCepstrumFrequences(juce::AudioBuffer<float> audioData, int sampleRate, std::vector<bool> sonorousFrames);
	static std::vector<float> getFreqSpectrum(juce::AudioBuffer<float> audioData, int sampleRate, int* frameSize, float overlapLevel);
	static void chooseWindowFunction(WINDOW_FUNCTION choice);
	static const int defaultFrameSize = 1024;
private:
	static void (*chosenWindowFunction)(std::vector<float>&);
	static void rectangleWindowFunction(std::vector<float>& frame);
	static void triangleWindowFunction(std::vector<float>& frame);
	static void hammingWindowFunction(std::vector<float>& frame);
	static void vanHannWindowFunction(std::vector<float>& frame);
	static void blackmanWindowFunction(std::vector<float>& frame);
}; 