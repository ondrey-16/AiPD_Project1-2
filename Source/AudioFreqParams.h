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
	// Transformacja sygnału wybraną funkcją okienkową
	static std::vector<float> transformAudioDataByWindowFunction(juce::AudioBuffer<float> audioData, int* frameSize);
	// Parametry częstotliwościowe
	static std::vector<float> getVolume(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<float> getCentroid(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<float> getBandwidth(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<std::vector<float>> getBandEnergyRatio(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<std::vector<float>> getSFM(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<std::vector<float>> getSCF(juce::AudioBuffer<float> audioData, int sampleRate);
	// Wyznaczanie częstotliwości krtaniowych za pomocą cepstrum
	static std::vector<float> getCepstrumFrequences(juce::AudioBuffer<float> audioData, int sampleRate, std::vector<bool> sonorousFrames);
	// Wyznaczanie widma częstotliwościowego
	static std::vector<float> getFreqSpectrum(juce::AudioBuffer<float> audioData, int sampleRate, int* frameSize, float overlapLevel);

	std::vector<std::vector<float>> getMFCC(juce::AudioBuffer<float> audioData, int sampleRate);
	// Wybór funkcji okienkowej
	static void chooseWindowFunction(WINDOW_FUNCTION choice);
	// Domyślny rozmiar ramki
	static const int defaultFrameSize = 1024;
private:
	// Funkcje okienkowe
	static void (*chosenWindowFunction)(std::vector<float>&);
	static void rectangleWindowFunction(std::vector<float>& frame);
	static void triangleWindowFunction(std::vector<float>& frame);
	static void hammingWindowFunction(std::vector<float>& frame);
	static void vanHannWindowFunction(std::vector<float>& frame);
	static void blackmanWindowFunction(std::vector<float>& frame);

	// Weryfikacja głosu
	static juce::AudioBuffer<float> preemphaseStage(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<float> fft2Stage(juce::AudioBuffer<float> audioData, int sampleRate);
	static std::vector<std::vector<float>> getMelFilters(int sampleRate, int frameSize, int filterCount, float minFreq, float maxFreq);
	static std::vector<std::vector<float>> melFiltersStage(std::vector<float>& poweredSpectrum, int sampleRate, int filterCount);
	static std::vector<std::vector<float>> logStage(std::vector<std::vector<float>>& melEnergies);
	std::vector<std::vector<float>> dctStage(std::vector<std::vector<float>>& logMelEnergies);
	static float hzToMel(float f);
	static float melToHz(float mel);
}; 