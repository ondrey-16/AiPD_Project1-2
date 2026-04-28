#pragma once

#include <JuceHeader.h>
#include "AudioTimeParams.h"
#include "AudioFreqParams.h"
#include "imgui.h"
#include "implot/implot.h"
#include "backends/imgui_impl_opengl3.h"
#include <mutex>

enum FREQ_PARAMS_PLOT
{
	FREQ_PARAMS_1 = 1,
	FREQ_PARAMS_2,
	SPECTRUM,
	LAR_FREQ
};

enum PLOT_PAGE
{
	TIME_PARAMS,
	FREQ_PARAMS,
	SPECTROGRAM
};

class AudioParamsPlotComponent : public juce::Component, juce::OpenGLRenderer
{
public:
	AudioParamsPlotComponent();
	~AudioParamsPlotComponent() override;
	void setAudioData(juce::AudioBuffer<float> audioData, uint32_t sampleRate);
	void showSilentFrames();
	void showSonorousFrames();
	void showMusicSpeechClips();
	void changePlotPage(PLOT_PAGE newPage);
	void setFrameSize(int frameSize);
	void setOverlapLevel(float overlapLevel);
	void selectFreqBand(int band);
	void chooseFrequenceParamsPlot(FREQ_PARAMS_PLOT choice);
	void chooseWindowFunction(WINDOW_FUNCTION choice);

	void paint(juce::Graphics& g) override;
	void resized() override;

	void mouseMove(const juce::MouseEvent& e) override;
	void mouseDrag(const juce::MouseEvent& e) override;
	void mouseDown(const juce::MouseEvent& e) override;
	void mouseUp(const juce::MouseEvent& e) override;
	void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& msd) override;

	std::vector<float> volumesY;
	std::vector<float> STEsY;
	std::vector<float> ZCRsY;
	std::vector<float> VSTDsY;
	std::vector<float> VDRsY;
	std::vector<float> VUsY;
	std::vector<float> LSTERsY;
	std::vector<float> EntropiesY;
	std::vector<float> ZSTDsY;
	std::vector<float> HZCRRsY;
	std::vector<float> autoCorrFrequences;
	std::vector<float> AMDFFrequences;

	std::vector<float> freqVolumesY;
	std::vector<float> freqCentroidsY;
	std::vector<float> freqBandwidthsY;
	std::vector<std::vector<float>> freqBandEnergyRatiosY;
	std::vector<std::vector<float>> freqSFNs;
	std::vector<std::vector<float>> freqSCFs;

private:
	juce::OpenGLContext openGLContext;
	AudioTimeParams audioTimeParamsAnalyzer;
	AudioFreqParams audioFreqParamsAnalyzer;
	juce::AudioBuffer<float> audioData;
	uint32_t sampleRate = 0;

	bool silentFramesChecked = false;
	bool sonorousFramesChecked = false;
	bool musicSpeechClipsChecked = false;

	PLOT_PAGE showedPlotPage = PLOT_PAGE::TIME_PARAMS;
	FREQ_PARAMS_PLOT chosenFreqParamsPlot = FREQ_PARAMS_PLOT::FREQ_PARAMS_1;

	juce::Point<float> lastCursorPos{ 0.0f, 0.0f };
	bool leftMouseDown = false;
	float wheelDelta = 0.0f;

	int selectedFreqBand = 0;

	std::vector<float> timestampsX;
	std::vector<float> frameTimestampsX;
	std::vector<float> clipTimestampsX;
	std::vector<float> audioValsY;
	std::vector<bool> detectedFrames;
	std::vector<int> speechMusicDetectedClips;

	int spectrogramBins = 0;
	int spectrogramFrames = 0;
	std::vector<float> spectrogramData;

	std::vector<float> windowTransformedTimestampsX;
	std::vector<float> windowTransformedAudioValsY;
	std::vector<float> freqFrameTimestampsX;
	std::vector<float> frameFreqSpectrumX;
	std::vector<float> frameFreqSpectrumY;

	int maxFreqFrame = 0;
	int chosenFreqFrame = 0;
	std::vector<float> freqSpectrum;
	std::vector<float> spectrogramFreqSpectrum;
	int spectrogramFrameSize = 512;
	float spectrogramOverlapLevel = 0.0f;

	std::vector<float> cepstrumFrequences;

	std::mutex setDataMutex;
	bool shouldFitPlots = true;

	void renderTimeParamsPlot();
	void renderFreqParamsPlot();
	void renderSpectrogramPlot();
	void updateFreqSpectrum();
	void updateFrameFreqSpectrumOnDisplay();
	void updateSpectrogram();
	void updateImGuiInput();
	void fillDetectedFrames(ImU32 color);
	void fillMusicSpeechClips();
	void fillChosenFreqFrame();
	void newOpenGLContextCreated() override;
	void renderOpenGL() override;
	void openGLContextClosing() override;
};