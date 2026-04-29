#include "AudioParamsPlotComponent.h"
#include "AudioTimeParams.h"
#include "AudioFreqParams.h"
#include <algorithm>

AudioParamsPlotComponent::AudioParamsPlotComponent()
{
	this->openGLContext.setRenderer(this);
	this->openGLContext.setContinuousRepainting(true);
	this->openGLContext.setComponentPaintingEnabled(false);
	this->openGLContext.attachTo(*this);
}

AudioParamsPlotComponent::~AudioParamsPlotComponent()
{
	this->openGLContext.detach();
}

void AudioParamsPlotComponent::setAudioData(juce::AudioBuffer<float> audioData, uint32_t sampleRate)
{
	std::lock_guard<std::mutex> lock(this->setDataMutex);

	this->audioData.makeCopyOf(audioData);
	this->sampleRate = sampleRate;

	this->timestampsX.clear();
	this->frameTimestampsX.clear();
	this->clipTimestampsX.clear();
	this->audioValsY.clear();
	this->volumesY.clear();
	this->STEsY.clear();
	this->ZCRsY.clear();
	this->autoCorrFrequences.clear();
	this->AMDFFrequences.clear();
	this->VSTDsY.clear();
	this->VDRsY.clear();
	this->VUsY.clear();
	this->LSTERsY.clear();
	this->EntropiesY.clear();
	this->ZSTDsY.clear();
	this->HZCRRsY.clear();
	this->speechMusicDetectedClips.clear();
	this->detectedFrames.clear();

	this->windowTransformedTimestampsX.clear();
	this->windowTransformedAudioValsY.clear();
	this->frameFreqSpectrumX.clear();
	this->frameFreqSpectrumY.clear();

	this->spectrogramFreqSpectrum.clear();
	this->spectrogramData.clear();

	this->cepstrumFrequences.clear();

	if (!audioData.getNumSamples() || !sampleRate)
	{
		return;
	}

	const auto* samples = audioData.getReadPointer(0);
	const int numSamples = audioData.getNumSamples();

	this->timestampsX.resize(numSamples);
	this->audioValsY.resize(numSamples);

	for (size_t i = 0; i < numSamples; i++)
	{
		this->timestampsX[i] = (float)i / (float)sampleRate;
		this->audioValsY[i] = samples[i];
	}

	this->volumesY = audioTimeParamsAnalyzer.getVolume(audioData, sampleRate);
	this->STEsY = audioTimeParamsAnalyzer.getSTE(audioData, sampleRate);
	this->ZCRsY = audioTimeParamsAnalyzer.getZCR(audioData, sampleRate);

	float frameTimestamp = (float)audioTimeParamsAnalyzer.frameSize / (float)(sampleRate);
	float val = frameTimestamp / 2.0f;

	this->frameTimestampsX.resize(this->volumesY.size());
	for (size_t i = 0; i < this->volumesY.size(); i++)
	{
		this->frameTimestampsX[i] = val;
		val += frameTimestamp;
	}

	this->VSTDsY = audioTimeParamsAnalyzer.getVSTD(audioData, sampleRate);
	this->VDRsY = audioTimeParamsAnalyzer.getVDR(audioData, sampleRate);
	this->VUsY = audioTimeParamsAnalyzer.getVU(audioData, sampleRate);
	this->LSTERsY = audioTimeParamsAnalyzer.getLSTER(audioData, sampleRate);
	this->EntropiesY = audioTimeParamsAnalyzer.getEntropy(audioData, sampleRate);
	this->ZSTDsY = audioTimeParamsAnalyzer.getZSTD(audioData, sampleRate);
	this->HZCRRsY = audioTimeParamsAnalyzer.getHZCRR(audioData, sampleRate);

	int framesPerClip = sampleRate / audioTimeParamsAnalyzer.frameSize;
	float clipTimestamp = (float)(framesPerClip * audioTimeParamsAnalyzer.frameSize) / (float)sampleRate;
	val = clipTimestamp / 2.0f;
	this->clipTimestampsX.resize(this->VSTDsY.size());
	for (size_t i = 0; i < this->VSTDsY.size(); i++)
	{
		this->clipTimestampsX[i] = val;
		val += clipTimestamp;
	}

	if (silentFramesChecked && this->audioData.getNumSamples() > 0 && this->sampleRate)
	{
		this->detectedFrames = this->audioTimeParamsAnalyzer.silenceDetection(this->audioData, this->sampleRate);
	}
	else if (sonorousFramesChecked && this->audioData.getNumSamples() > 0 && this->sampleRate)
	{
		this->detectedFrames = this->audioTimeParamsAnalyzer.sonorousDetection(this->audioData, this->sampleRate);
		this->autoCorrFrequences = this->audioTimeParamsAnalyzer.getFundamentalFreqAutoCorr(this->audioData, this->sampleRate);
		this->AMDFFrequences = this->audioTimeParamsAnalyzer.getFundamentalFreqAMDF(this->audioData, this->sampleRate);
	}
	else if (musicSpeechClipsChecked && this->audioData.getNumSamples() > 0 && this->sampleRate)
	{
		this->speechMusicDetectedClips = this->audioTimeParamsAnalyzer.musicSpeechDetection(this->audioData, this->sampleRate);
	}


	const int freqNumSamples = numSamples - numSamples % this->audioFreqParamsAnalyzer.defaultFrameSize;
	this->windowTransformedTimestampsX.resize(freqNumSamples);
	copy(this->timestampsX.begin(), this->timestampsX.begin() + freqNumSamples, this->windowTransformedTimestampsX.begin());
	this->windowTransformedAudioValsY = audioFreqParamsAnalyzer.transformAudioDataByWindowFunction(this->audioData, nullptr);

	this->updateFreqSpectrum();
	this->updateFrameFreqSpectrumOnDisplay();
	const int freqSpectrumSize = this->audioFreqParamsAnalyzer.defaultFrameSize / 2 + 1;
	const int frameCount = (int)this->freqSpectrum.size() / freqSpectrumSize;

	this->chosenFreqFrame = (frameCount > 0) ? frameCount / 2 : 0;

	this->freqFrameTimestampsX.resize(this->freqVolumesY.size());
	const float frameTime = (float)this->audioFreqParamsAnalyzer.defaultFrameSize 
		/ (float)this->sampleRate;

	for (size_t i = 0; i < this->freqFrameTimestampsX.size(); i++)
	{
		this->freqFrameTimestampsX[i] = (i + 0.5f) * frameTime;
	}

	this->updateSpectrogram();

	this->cepstrumFrequences = this->audioFreqParamsAnalyzer.getCepstrumFrequences(this->audioData, this->sampleRate, 
		this->audioTimeParamsAnalyzer.sonorousDetection(this->audioData, this->sampleRate));

	this->shouldFitPlots = true;
}

void AudioParamsPlotComponent::showSilentFrames()
{
	std::lock_guard<std::mutex> lock(this->setDataMutex);

	this->silentFramesChecked = true;
	this->sonorousFramesChecked = false;
	this->musicSpeechClipsChecked = false;
	if (this->audioData.getNumSamples() > 0 && this->sampleRate)
	{
		this->detectedFrames = this->audioTimeParamsAnalyzer.silenceDetection(this->audioData, this->sampleRate);
	}
}

void AudioParamsPlotComponent::showSonorousFrames()
{
	std::lock_guard<std::mutex> lock(this->setDataMutex);

	this->sonorousFramesChecked = true;
	this->silentFramesChecked = false;
	this->musicSpeechClipsChecked = false;
	if (this->audioData.getNumSamples() > 0 && this->sampleRate)
	{
		this->detectedFrames = this->audioTimeParamsAnalyzer.sonorousDetection(this->audioData, this->sampleRate);
		this->autoCorrFrequences = this->audioTimeParamsAnalyzer.getFundamentalFreqAutoCorr(this->audioData, this->sampleRate);
		this->AMDFFrequences = this->audioTimeParamsAnalyzer.getFundamentalFreqAMDF(this->audioData, this->sampleRate);
	}
}

void AudioParamsPlotComponent::showMusicSpeechClips()
{
	std::lock_guard<std::mutex> lock(this->setDataMutex);

	this->musicSpeechClipsChecked = true;
	this->silentFramesChecked = false;
	this->sonorousFramesChecked = false;
	if (this->audioData.getNumSamples() > 0 && this->sampleRate)
	{
		this->speechMusicDetectedClips = this->audioTimeParamsAnalyzer.musicSpeechDetection(this->audioData, this->sampleRate);
	}
}

void AudioParamsPlotComponent::changePlotPage(PLOT_PAGE newPage)
{
	std::lock_guard<std::mutex> lock(this->setDataMutex);
	this->showedPlotPage = newPage;
}

void AudioParamsPlotComponent::setFrameSize(int frameSize)
{
	std::lock_guard<std::mutex> lock(this->setDataMutex);

	this->spectrogramFrameSize = frameSize;
	this->updateSpectrogram();
}

void AudioParamsPlotComponent::setOverlapLevel(float overlapLevel)
{
	std::lock_guard<std::mutex> lock(this->setDataMutex);

	this->spectrogramOverlapLevel = overlapLevel;
	this->updateSpectrogram();
}

void AudioParamsPlotComponent::selectFreqBand(int band)
{
	std::lock_guard<std::mutex> lock(this->setDataMutex);
	this->selectedFreqBand = std::clamp(band, 0, 2);
}

void AudioParamsPlotComponent::chooseFrequenceParamsPlot(FREQ_PARAMS_PLOT choice)
{
	this->chosenFreqParamsPlot = choice;
}

void AudioParamsPlotComponent::chooseWindowFunction(WINDOW_FUNCTION choice)
{
	std::lock_guard<std::mutex> lock(this->setDataMutex);

	this->audioFreqParamsAnalyzer.chooseWindowFunction(choice);

	const auto* samples = this->audioData.getReadPointer(0);
	const int numSamples = this->audioData.getNumSamples();

	const int freqNumSamples = numSamples - numSamples % this->audioFreqParamsAnalyzer.defaultFrameSize;
	this->windowTransformedTimestampsX.resize(freqNumSamples);
	copy(this->timestampsX.begin(), this->timestampsX.begin() + freqNumSamples, this->windowTransformedTimestampsX.begin());
	this->windowTransformedAudioValsY = audioFreqParamsAnalyzer.transformAudioDataByWindowFunction(this->audioData, nullptr);
	
	this->updateFreqSpectrum();
	this->updateFrameFreqSpectrumOnDisplay();

	this->cepstrumFrequences = this->audioFreqParamsAnalyzer.getCepstrumFrequences(this->audioData, this->sampleRate,
		this->audioTimeParamsAnalyzer.sonorousDetection(this->audioData, this->sampleRate));

	this->updateSpectrogram();
}

void AudioParamsPlotComponent::paint(juce::Graphics& g)
{}

void AudioParamsPlotComponent::resized()
{}

void AudioParamsPlotComponent::mouseMove(const juce::MouseEvent & e)
{
	this->lastCursorPos = e.position;
}

void AudioParamsPlotComponent::mouseDrag(const juce::MouseEvent& e)
{
	this->lastCursorPos = e.position;
	this->leftMouseDown = e.mods.isLeftButtonDown();
}

void AudioParamsPlotComponent::mouseDown(const juce::MouseEvent& e)
{
	this->lastCursorPos = e.position;
	this->leftMouseDown = e.mods.isLeftButtonDown();
}

void AudioParamsPlotComponent::mouseUp(const juce::MouseEvent & e)
{
	this->lastCursorPos = e.position;
	this->leftMouseDown = e.mods.isLeftButtonDown();
}

void AudioParamsPlotComponent::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& msd)
{
	this->lastCursorPos = e.position;
	this->wheelDelta = msd.deltaY;
}

void AudioParamsPlotComponent::renderTimeParamsPlot()
{
	std::lock_guard<std::mutex> lock(this->setDataMutex);

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2((float)getWidth(), (float)getHeight()), ImGuiCond_Always);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

	ImGui::Begin("PlotHost", nullptr, flags);

	if (!this->timestampsX.empty() && !this->audioValsY.empty())
	{
		float maxWidth = ImGui::GetContentRegionAvail().x;
		float maxHeight = ImGui::GetContentRegionAvail().y;

		if (ImPlot::BeginPlot("Audio graph + frame parameters", ImVec2(maxWidth, (this->sonorousFramesChecked) ? maxHeight * 0.33f : maxHeight * 0.5f)))
		{
			ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
			ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);

			ImPlot::SetupAxes("Time [s]", "Amplitude/Value");

			if (this->silentFramesChecked)
			{
				this->fillDetectedFrames(IM_COL32(0, 200, 0, 60));
			}
			else if (this->sonorousFramesChecked)
			{
				this->fillDetectedFrames(IM_COL32(0, 0, 200, 60));
			}
			else if (this->musicSpeechClipsChecked)
			{
				this->fillMusicSpeechClips();
			}

			ImPlot::PlotLine("Audio", this->timestampsX.data(), this->audioValsY.data(), (int)this->audioValsY.size());
			ImPlot::PlotLine("Volume", this->frameTimestampsX.data(), this->volumesY.data(), (int)this->volumesY.size());
			ImPlot::PlotLine("STE", this->frameTimestampsX.data(), this->STEsY.data(), (int)this->STEsY.size());
			ImPlot::PlotLine("ZCR", this->frameTimestampsX.data(), this->ZCRsY.data(), (int)this->ZCRsY.size());

			ImPlot::PopStyleVar(2);
			ImPlot::EndPlot();
		}
		if (this->sonorousFramesChecked && ImPlot::BeginPlot("Fundamental frequences", ImVec2(maxWidth, maxHeight * 0.33f)))
		{
			ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
			ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);

			ImPlot::SetupAxes("Time [s]", "Frequency [Hz]");

			ImPlot::PlotLine("Frequences by Autocorrelation", this->frameTimestampsX.data(), this->autoCorrFrequences.data(), (int)this->autoCorrFrequences.size());
			ImPlot::PlotLine("Frequences by AMDF", this->frameTimestampsX.data(), this->AMDFFrequences.data(), (int)this->AMDFFrequences.size());

			ImPlot::PopStyleVar(2);
			ImPlot::EndPlot();
		}
		if (ImPlot::BeginPlot("Audio clip parameters", ImVec2(maxWidth, (this->sonorousFramesChecked) ? maxHeight * 0.33f : maxHeight * 0.5f)))
		{
			ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
			ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);

			ImPlot::SetupAxes("Time [s]", "Amplitude/Value");

			if (clipTimestampsX.size() == 1)
			{
				ImPlot::PlotScatter("VSTD", this->clipTimestampsX.data(), this->VSTDsY.data(), (int)this->VSTDsY.size());
				ImPlot::PlotScatter("VDR", this->clipTimestampsX.data(), this->VDRsY.data(), (int)this->VDRsY.size());
				ImPlot::PlotScatter("VU", this->clipTimestampsX.data(), this->VUsY.data(), (int)this->VUsY.size());
				ImPlot::PlotScatter("LSTER", this->clipTimestampsX.data(), this->LSTERsY.data(), (int)this->LSTERsY.size());
				ImPlot::PlotScatter("Entropy", this->clipTimestampsX.data(), this->EntropiesY.data(), (int)this->EntropiesY.size());
				ImPlot::PlotScatter("ZSTD", this->clipTimestampsX.data(), this->ZSTDsY.data(), (int)this->ZSTDsY.size());
				ImPlot::PlotScatter("HZCRR", this->clipTimestampsX.data(), this->HZCRRsY.data(), (int)this->HZCRRsY.size());
			}
			else
			{
				ImPlot::PlotLine("VSTD", this->clipTimestampsX.data(), this->VSTDsY.data(), (int)this->VSTDsY.size());
				ImPlot::PlotLine("VDR", this->clipTimestampsX.data(), this->VDRsY.data(), (int)this->VDRsY.size());
				ImPlot::PlotLine("VU", this->clipTimestampsX.data(), this->VUsY.data(), (int)this->VUsY.size());
				ImPlot::PlotLine("LSTER", this->clipTimestampsX.data(), this->LSTERsY.data(), (int)this->LSTERsY.size());
				ImPlot::PlotLine("Entropy", this->clipTimestampsX.data(), this->EntropiesY.data(), (int)this->EntropiesY.size());
				ImPlot::PlotLine("ZSTD", this->clipTimestampsX.data(), this->ZSTDsY.data(), (int)this->ZSTDsY.size());
				ImPlot::PlotLine("HZCRR", this->clipTimestampsX.data(), this->HZCRRsY.data(), (int)this->HZCRRsY.size());
			}

			ImPlot::PopStyleVar(2);
			ImPlot::EndPlot();
		}
	}

	ImGui::End();
	ImGui::PopStyleVar(2);
}

void AudioParamsPlotComponent::renderFreqParamsPlot()
{
	std::lock_guard<std::mutex> lock(this->setDataMutex);

	bool ifFrameChanged = false;

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2((float)getWidth(), (float)getHeight()), ImGuiCond_Always);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;

	ImGui::Begin("PlotHost", nullptr, flags);

	if (!this->timestampsX.empty() && !this->audioValsY.empty())
	{
		float maxWidth = ImGui::GetContentRegionAvail().x;
		float maxHeight = ImGui::GetContentRegionAvail().y;

		const float controlsHeight = 34.0f;
		const float sliderHeight = 28.0f;
		const float gap = 4.0f;

		const float availablePlotHeight =
			maxHeight - controlsHeight - sliderHeight - gap;

		const float audioHeight = availablePlotHeight * 0.5f;

		if (this->shouldFitPlots)
		{
			ImPlot::SetNextAxesToFit();
		}
		if (ImPlot::BeginPlot("Audio graph with window function transformation", ImVec2(maxWidth, audioHeight)))
		{
			ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
			ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);

			ImPlot::SetupAxes("Time [s]", "Amplitude");

			this->fillChosenFreqFrame();

			ImPlot::PlotLine("Audio graph", this->windowTransformedTimestampsX.data(), this->windowTransformedAudioValsY.data(), (int)this->windowTransformedAudioValsY.size());

			ImPlot::PopStyleVar(2);
			ImPlot::EndPlot();
		}

		switch (this->chosenFreqParamsPlot)
		{
			case FREQ_PARAMS_PLOT::FREQ_PARAMS_1:
			{
				const float paramsHeight = availablePlotHeight * 0.5f;

				if (this->shouldFitPlots)
				{
					ImPlot::SetNextAxesToFit();
				}
				if (ImPlot::BeginPlot("Audio frequence parameters", ImVec2(maxWidth, paramsHeight)))
				{
					ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
					ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);

					ImPlot::SetupAxes("Time [s]", "Amplitude");

					this->fillChosenFreqFrame();

					ImPlot::PlotLine("Volume", this->freqFrameTimestampsX.data(), this->freqVolumesY.data(), (int)this->freqVolumesY.size());
					ImPlot::PlotLine("Centroid", this->freqFrameTimestampsX.data(), this->freqCentroidsY.data(), (int)this->freqCentroidsY.size());
					ImPlot::PlotLine("Bandwidth", this->freqFrameTimestampsX.data(), this->freqBandwidthsY.data(), (int)this->freqBandwidthsY.size());

					ImPlot::PopStyleVar(2);
					ImPlot::EndPlot();
				}

				break;
			}
			case FREQ_PARAMS_PLOT::FREQ_PARAMS_2:
			{
				const float bandHeight = availablePlotHeight * 0.5f;

				ImGui::Text("Selected band:");
				ImGui::SameLine();

				if (ImGui::RadioButton("Band 1: 0-630 Hz", this->selectedFreqBand == 0))
					this->selectedFreqBand = 0;
				ImGui::SameLine();
				if (ImGui::RadioButton("Band 2: 630-1720 Hz", this->selectedFreqBand == 1))
					this->selectedFreqBand = 1;
				ImGui::SameLine();
				if (ImGui::RadioButton("Band 3: 1720-4400 Hz", this->selectedFreqBand == 2))
					this->selectedFreqBand = 2;

				ImGui::Dummy(ImVec2(0.0f, gap));

				const int band = std::clamp(this->selectedFreqBand, 0, 2);

				if (this->shouldFitPlots)
				{
					ImPlot::SetNextAxesToFit();
				}
				if (ImPlot::BeginPlot("Audio band-frequence parameters", ImVec2(maxWidth, bandHeight)))
				{
					ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
					ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);

					ImPlot::SetupAxes("Time [s]", "Amplitude");

					this->fillChosenFreqFrame();

					ImPlot::PlotLine("ERSB", this->freqFrameTimestampsX.data(), this->freqBandEnergyRatiosY[band].data(), (int)this->freqBandEnergyRatiosY[band].size());
					ImPlot::PlotLine("SFM", this->freqFrameTimestampsX.data(), this->freqSFNs[band].data(), (int)this->freqSFNs[band].size());
					ImPlot::PlotLine("SCF", this->freqFrameTimestampsX.data(), this->freqSCFs[band].data(), (int)this->freqSCFs[band].size());

					ImPlot::PopStyleVar(2);
					ImPlot::EndPlot();
				}
				break;
			}
			case FREQ_PARAMS_PLOT::SPECTRUM:
			{
				const float spectrumHeight = availablePlotHeight * 0.5f;

				if (this->shouldFitPlots)
				{
					ImPlot::SetNextAxesToFit();
				}
				if (ImPlot::BeginPlot("Frame frequence spectrum", ImVec2(maxWidth, spectrumHeight)))
				{
					ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
					ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);

					ImPlot::SetupAxes("Frequency [Hz]", "Amplitude");

					ImPlot::PlotLine("Frequency spectrum", this->frameFreqSpectrumX.data(), this->frameFreqSpectrumY.data(), (int)this->frameFreqSpectrumY.size());

					ImPlot::PopStyleVar(2);
					ImPlot::EndPlot();
				}
				break;
			}
			case FREQ_PARAMS_PLOT::LAR_FREQ:
			{
				const float larFreqHeight = availablePlotHeight * 0.5f;

				if (this->shouldFitPlots)
				{
					ImPlot::SetNextAxesToFit();
				}
				if (ImPlot::BeginPlot("Laryngeal frequency plot", ImVec2(maxWidth, larFreqHeight)))
				{
					ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
					ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);

					ImPlot::SetupAxes("Time [s]", "Frequency [Hz]");

					ImPlot::PlotLine("Laryngeal frequency", this->freqFrameTimestampsX.data(), this->cepstrumFrequences.data(), (int)this->cepstrumFrequences.size());

					ImPlot::PopStyleVar(2);
					ImPlot::EndPlot();
				}
				break;
			}
		}

		if (this->maxFreqFrame > 0)
		{
			int prevFrame = this->chosenFreqFrame;

			if (ImGui::SliderInt("Choose frame", &this->chosenFreqFrame, 0, this->maxFreqFrame))
			{
				if (prevFrame != this->chosenFreqFrame)
				{
					updateFrameFreqSpectrumOnDisplay();
					ifFrameChanged = true;
				}
			}
		}
	}
	ImGui::End();
	ImGui::PopStyleVar(2);
	this->shouldFitPlots = ifFrameChanged;
}

void AudioParamsPlotComponent::renderSpectrogramPlot()
{
	std::lock_guard<std::mutex> lock(this->setDataMutex);

	if (this->sampleRate == 0 || this->spectrogramFreqSpectrum.empty())
	{
		return;
	}

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2((float)getWidth(), (float)getHeight()), ImGuiCond_Always);

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

	const ImGuiWindowFlags plotFlags =
		ImPlotFlags_NoMouseText |
		ImPlotFlags_NoMenus |
		ImPlotFlags_NoBoxSelect;

	const ImPlotAxisFlags axisFlags =
		ImPlotAxisFlags_NoMenus |
		ImPlotAxisFlags_NoHighlight;

	ImGui::Begin("PlotHost", nullptr);

	float maxWidth = ImGui::GetContentRegionAvail().x;
	float maxHeight = ImGui::GetContentRegionAvail().y;

	const int hopSize = std::roundf(this->spectrogramFrameSize * (1.0f - this->spectrogramOverlapLevel));

	const double duration = ((double)(this->spectrogramFrames - 1) * hopSize + this->spectrogramFrameSize) / (double)this->sampleRate;
	const double nyquist = (double)this->sampleRate / 2.0;

	const float colorbarWidth = 70.0f;
	const float gap = 60.0f;
	const double scaleMin = -80.0;
	const double scaleMax = 0.0;

	if (ImPlot::BeginPlot("Spectrogram", ImVec2(maxWidth - gap, maxHeight), plotFlags))
	{
		ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
		ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);

		ImPlot::SetupAxes("Time [s]", "Frequency [Hz]", axisFlags, axisFlags);
		ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, duration, ImGuiCond_Always);
		ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, nyquist, ImGuiCond_Always);

		ImPlot::PushColormap(ImPlotColormap_Viridis);

		ImPlot::PlotHeatmap("Spectrogram", spectrogramData.data(), this->spectrogramBins, this->spectrogramFrames, scaleMin, scaleMax,
			nullptr, ImPlotPoint(0.0, 0.0), ImPlotPoint(duration, nyquist));

		ImPlot::PopStyleVar(2);
		ImPlot::EndPlot();
	}
	ImGui::SameLine();
	ImPlot::ColormapScale("dB", -80.0, 0.0, ImVec2(60, maxHeight));

	ImPlot::PopColormap();

	ImGui::End();
	ImGui::PopStyleVar(2);
}

void AudioParamsPlotComponent::updateFreqSpectrum()
{
	this->freqSpectrum = this->audioFreqParamsAnalyzer.getFreqSpectrum(this->audioData, this->sampleRate, nullptr, 0.0f);
	const int freqSpectrumSize = this->audioFreqParamsAnalyzer.defaultFrameSize / 2 + 1;
	const int frameCount = (int)this->freqSpectrum.size() / freqSpectrumSize;

	this->freqVolumesY = audioFreqParamsAnalyzer.getVolume(this->audioData, this->sampleRate);
	this->freqCentroidsY = audioFreqParamsAnalyzer.getCentroid(this->audioData, this->sampleRate);
	this->freqBandwidthsY = audioFreqParamsAnalyzer.getBandwidth(this->audioData, this->sampleRate);
	this->freqBandEnergyRatiosY = audioFreqParamsAnalyzer.getBandEnergyRatio(this->audioData, this->sampleRate);
	this->freqSFNs = audioFreqParamsAnalyzer.getSFM(this->audioData, this->sampleRate);
	this->freqSCFs = audioFreqParamsAnalyzer.getSCF(this->audioData, this->sampleRate);

	this->freqFrameTimestampsX.resize(this->freqVolumesY.size());
	const float frameTime = (float)this->audioFreqParamsAnalyzer.defaultFrameSize
		/ (float)this->sampleRate;
}

void AudioParamsPlotComponent::updateFrameFreqSpectrumOnDisplay()
{
	const int freqSpectrumSize = this->audioFreqParamsAnalyzer.defaultFrameSize / 2 + 1;

	if (this->freqSpectrum.empty())
	{
		this->frameFreqSpectrumY.clear();
		return;
	}

	const int frameCount = (int)this->freqSpectrum.size() / freqSpectrumSize;
	if (frameCount <= 0)
	{
		this->frameFreqSpectrumY.clear();
		return;
	}

	this->chosenFreqFrame = std::clamp(this->chosenFreqFrame, 0, frameCount - 1);

	const int begin = this->chosenFreqFrame * freqSpectrumSize;

	this->frameFreqSpectrumY.resize(freqSpectrumSize);
	for (int i = 0; i < freqSpectrumSize; i++)
	{
		this->frameFreqSpectrumY[i] = this->freqSpectrum[begin + i];
	}

	this->frameFreqSpectrumX.resize(freqSpectrumSize);
	for (int i = 0; i < freqSpectrumSize; i++)
	{
		this->frameFreqSpectrumX[i] = (float)i * (float)this->sampleRate 
			/ (float)this->audioFreqParamsAnalyzer.defaultFrameSize;
	}

	this->maxFreqFrame = frameCount - 1;
}

void AudioParamsPlotComponent::updateSpectrogram()
{
	this->spectrogramFreqSpectrum = this->audioFreqParamsAnalyzer.getFreqSpectrum(this->audioData, this->sampleRate, &this->spectrogramFrameSize, this->spectrogramOverlapLevel);

	this->spectrogramBins = this->spectrogramFrameSize / 2 + 1;
	this->spectrogramFrames = this->spectrogramFreqSpectrum.size() / this->spectrogramBins;

	if (this->spectrogramFrames <= 0)
	{
		return;
	}
	this->spectrogramData.resize(this->spectrogramFrames * this->spectrogramBins);

	float maxA = 0.0f;

	for (float A : this->spectrogramFreqSpectrum)
	{
		maxA = std::max(maxA, A);
	}

	for (int t = 0; t < this->spectrogramFrames; t++)
	{
		for (int f = 0; f < this->spectrogramBins; f++)
		{
			float amplitude = this->spectrogramFreqSpectrum[t * this->spectrogramBins + f];
			float dBValue = 20.0f * std::log10((amplitude / maxA) + 1e-6f);

			dBValue = std::clamp(dBValue, -80.0f, 0.0f);

			this->spectrogramData[(this->spectrogramBins - 1 - f) * this->spectrogramFrames + t] = dBValue;
		}
	}
}

void AudioParamsPlotComponent::updateImGuiInput()
{
	ImGuiIO& io = ImGui::GetIO();

	float scale = (float)openGLContext.getRenderingScale();
	io.DisplayFramebufferScale = ImVec2(scale, scale);

	io.DisplaySize = ImVec2((float)getWidth(), (float)getHeight());
	io.MousePos = ImVec2((float)this->lastCursorPos.x, (float)this->lastCursorPos.y);
	io.MouseDown[0] = this->leftMouseDown;
	io.MouseWheel = this->wheelDelta;

	this->wheelDelta = 0.0f;
}

void AudioParamsPlotComponent::fillDetectedFrames(ImU32 color)
{
	if (!this->detectedFrames.size())
	{
		return;
	}

	auto limits = ImPlot::GetPlotLimits();
	auto* drawList = ImPlot::GetPlotDrawList();

	const float frameTime = (float)this->audioTimeParamsAnalyzer.frameSize / (float)this->sampleRate;

	ImPlot::PushPlotClipRect();

	for (size_t i = 0; i < this->detectedFrames.size(); i++)
	{
		if (!this->detectedFrames[i])
		{
			continue;
		}

		int j = i;
		while (j < this->detectedFrames.size() && this->detectedFrames[j])
		{
			j++;
		}

		float x1 = i * frameTime;
		float x2 = j * frameTime;

		ImVec2 p1 = ImPlot::PlotToPixels(ImPlotPoint(x1, limits.Y.Max));
		ImVec2 p2 = ImPlot::PlotToPixels(ImPlotPoint(x2, limits.Y.Min));

		drawList->AddRectFilled(p1, p2, color);

		i = j - 1;
	}

	ImPlot::PopPlotClipRect();
}

void AudioParamsPlotComponent::fillMusicSpeechClips()
{
	if (!this->speechMusicDetectedClips.size())
	{
		return;
	}

	auto limits = ImPlot::GetPlotLimits();
	auto* drawList = ImPlot::GetPlotDrawList();

	const float clipTime = 1.0f;

	ImPlot::PushPlotClipRect();

	for (size_t i = 0; i < this->speechMusicDetectedClips.size(); i++)
	{
		if (!this->speechMusicDetectedClips[i])
		{
			continue;
		}

		float x1 = i * clipTime;
		float x2 = (i + 1) * clipTime;

		ImVec2 p1 = ImPlot::PlotToPixels(ImPlotPoint(x1, limits.Y.Max));
		ImVec2 p2 = ImPlot::PlotToPixels(ImPlotPoint(x2, limits.Y.Min));

		if (this->speechMusicDetectedClips[i] == 1)
		{
			drawList->AddRectFilled(p1, p2, IM_COL32(255, 0, 0, 60));
		}
		else if (this->speechMusicDetectedClips[i] == 2)
		{
			drawList->AddRectFilled(p1, p2, IM_COL32(255, 0, 255, 60));
		}
	}

	ImPlot::PopPlotClipRect();
}

void AudioParamsPlotComponent::fillChosenFreqFrame()
{
	if (this->sampleRate == 0)
	{
		return;
	}

	auto limits = ImPlot::GetPlotLimits();
	auto* drawList = ImPlot::GetPlotDrawList();

	const float frameTime = (float)this->audioFreqParamsAnalyzer.defaultFrameSize / (float)this->sampleRate;

	const float x1 = (float)this->chosenFreqFrame * frameTime;
	const float x2 = (float)(this->chosenFreqFrame + 1) * frameTime;

	ImPlot::PushPlotClipRect();

	ImVec2 p1 = ImPlot::PlotToPixels(ImPlotPoint(x1, limits.Y.Max));
	ImVec2 p2 = ImPlot::PlotToPixels(ImPlotPoint(x2, limits.Y.Min));

	drawList->AddRectFilled(p1, p2, IM_COL32(255, 0, 0, 60));

	ImPlot::PopPlotClipRect();
}

void AudioParamsPlotComponent::newOpenGLContextCreated()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();
	ImGui::StyleColorsDark();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui_ImplOpenGL3_Init("#version 130");
}

void AudioParamsPlotComponent::renderOpenGL()
{
	juce::OpenGLHelpers::clear(juce::Colours::black);
	updateImGuiInput();

	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	switch (this->showedPlotPage)
	{
		case PLOT_PAGE::TIME_PARAMS:
			this->renderTimeParamsPlot();
			break;
		case PLOT_PAGE::FREQ_PARAMS:
			this->renderFreqParamsPlot();
			break;
		case PLOT_PAGE::SPECTROGRAM:
			this->renderSpectrogramPlot();
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void AudioParamsPlotComponent::openGLContextClosing()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();
}
