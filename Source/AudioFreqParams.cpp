#include "AudioFreqParams.h"
#include <numbers>
#include <algorithm>
#include <fftw3.h>

void (*AudioFreqParams::chosenWindowFunction)(std::vector<float>&) = AudioFreqParams::rectangleWindowFunction;

std::vector<float> AudioFreqParams::transformAudioDataByWindowFunction(juce::AudioBuffer<float> audioData, int* frameSize)
{
	auto* samples = audioData.getReadPointer(0);
	int numSamples = audioData.getNumSamples();

	int fSize = (frameSize != nullptr) ? *frameSize : defaultFrameSize;

	numSamples -= numSamples % fSize;

	std::vector<float> transformedAudioData(numSamples);

	for (int i = 0; i + fSize < numSamples; i += fSize)
	{
		std::vector<float> frame(fSize);
		
		for (size_t j = i; j < fSize + i; j++)
		{
			frame[j - i] = samples[j];
		}

		chosenWindowFunction(frame);
		copy(frame.begin(), frame.end(), transformedAudioData.begin() + i);
	}

	return transformedAudioData;
}

std::vector<float> AudioFreqParams::getVolume(juce::AudioBuffer<float> audioData, int sampleRate)
{
	auto freqSpectrum = getFreqSpectrum(audioData, sampleRate, nullptr, 0.0f);
	const int freqSpectrumFrameSize = defaultFrameSize / 2 + 1;
	size_t frameCount = freqSpectrum.size() / freqSpectrumFrameSize;

	std::vector<float> volumes(frameCount);

	for (size_t i = 0; i < frameCount; i++)
	{
		float volume = 0.0f;
		for (int j = 0; j < freqSpectrumFrameSize; j++)
		{
			volume += std::powf(freqSpectrum[i * freqSpectrumFrameSize + j], 2);
		}
		volumes[i] = volume / (float)freqSpectrumFrameSize;
	}

	return volumes;
}

std::vector<float> AudioFreqParams::getCentroid(juce::AudioBuffer<float> audioData, int sampleRate)
{
	auto freqSpectrum = getFreqSpectrum(audioData, sampleRate, nullptr, 0.0f);
	const int freqSpectrumFrameSize = defaultFrameSize / 2 + 1;
	size_t frameCount = freqSpectrum.size() / freqSpectrumFrameSize;

	std::vector<float> centroids(frameCount);

	for (size_t i = 0; i < frameCount; i++)
	{
		float nominator = 0.0f;
		float denominator = 0.0f;

		for (int j = 0; j < freqSpectrumFrameSize; j++)
		{
			float freq = (float)j * (float)sampleRate / (float)defaultFrameSize;
			nominator += freq * freqSpectrum[i * freqSpectrumFrameSize + j];
			denominator += freqSpectrum[i * freqSpectrumFrameSize + j];
		}

		centroids[i] = (denominator > 0.0f) ? nominator / denominator : 0.0;
	}

	return centroids;
}

std::vector<float> AudioFreqParams::getBandwidth(juce::AudioBuffer<float> audioData, int sampleRate)
{
	auto freqSpectrum = getFreqSpectrum(audioData, sampleRate, nullptr, 0.0f);
	auto centroids = getCentroid(audioData, sampleRate);
	const int freqSpectrumFrameSize = defaultFrameSize / 2 + 1;
	size_t frameCount = freqSpectrum.size() / freqSpectrumFrameSize;

	std::vector<float> BWs(frameCount);
	for (size_t i = 0; i < frameCount; i++)
	{
		float nominator = 0.0f;
		float denominator = 0.0f;

		for (int j = 0; j < freqSpectrumFrameSize; j++)
		{
			float freq = (float)j * (float)sampleRate / (float)defaultFrameSize;
			nominator += std::powf((freq - centroids[i]), 2)
				* std::powf(freqSpectrum[i * freqSpectrumFrameSize + j], 2);
			denominator += std::powf(freqSpectrum[i * freqSpectrumFrameSize + j], 2);
		}

		BWs[i] = (denominator > 0.0f) ? std::sqrtf(nominator / denominator) : 0.0;
	}

	return BWs;
}

std::vector<std::vector<float>> AudioFreqParams::getBandEnergyRatio(juce::AudioBuffer<float> audioData, int sampleRate)
{
	auto freqSpectrum = getFreqSpectrum(audioData, sampleRate, nullptr, 0.0f);
	auto volumes = getVolume(audioData, sampleRate);
	const int freqSpectrumFrameSize = defaultFrameSize / 2 + 1;
	size_t frameCount = freqSpectrum.size() / freqSpectrumFrameSize;

	std::vector<std::vector<float>> bandEnergyRatios(3);
	for (size_t i = 0; i < 3; i++)
	{
		bandEnergyRatios[i].resize(frameCount);
	}

	for (size_t i = 0; i < frameCount; i++)
	{
		float energies[3] = { 0.0f, 0.0f, 0.0f };
		for (int j = 0; j < freqSpectrumFrameSize; j++)
		{
			float freq = (float)j * (float)sampleRate / (float)defaultFrameSize;
			if (freq <= 630)
			{
				energies[0] += std::powf(freqSpectrum[i * freqSpectrumFrameSize + j], 2);
			}
			else if (freq <= 1720)
			{
				energies[1] += std::powf(freqSpectrum[i * freqSpectrumFrameSize + j], 2);
			}
			else if (freq <= 4400)
			{
				energies[2] += std::powf(freqSpectrum[i * freqSpectrumFrameSize + j], 2);
			}
		}

		bandEnergyRatios[0][i] = energies[0] / (volumes[i] * freqSpectrumFrameSize);
		bandEnergyRatios[1][i] = energies[1] / (volumes[i] * freqSpectrumFrameSize);
		bandEnergyRatios[2][i] = energies[2] / (volumes[i] * freqSpectrumFrameSize);
	}

	return bandEnergyRatios;
}

std::vector<std::vector<float>> AudioFreqParams::getSFM(juce::AudioBuffer<float> audioData, int sampleRate)
{
	auto freqSpectrum = getFreqSpectrum(audioData, sampleRate, nullptr, 0.0f);
	const int freqSpectrumFrameSize = defaultFrameSize / 2 + 1;
	size_t frameCount = freqSpectrum.size() / freqSpectrumFrameSize;

	std::vector<std::vector<float>> SFMs(3);
	for (size_t i = 0; i < 3; i++)
	{
		SFMs[i].resize(frameCount);
	}
	const float eps = 1e-12f;
	for (size_t i = 0; i < frameCount; i++)
	{
		float geoms[3] = { 0.0f, 0.0f, 0.0f };
		float arithms[3] = { 0.0f, 0.0f, 0.0f };
		int N[3] = { 0 };
		for (int j = 0; j < freqSpectrumFrameSize; j++)
		{
			float freq = (float)j * (float)sampleRate / (float)defaultFrameSize;
			float power = std::powf(freqSpectrum[i * freqSpectrumFrameSize + j], 2);
			if (freq <= 630)
			{
				geoms[0] += logf(power + eps);
				arithms[0] += power;
				N[0]++;
			}
			else if (freq <= 1720)
			{
				geoms[1] += logf(power + eps);
				arithms[1] += power;
				N[1]++;
			}
			else if (freq <= 4400)
			{
				geoms[2] += logf(power + eps);
				arithms[2] += power;
				N[2]++;
			}
		}

		for (size_t k = 0; k < 3; k++)
		{
			geoms[k] = std::powf(expf(geoms[k]), 1.0f / (float)N[k]);
			arithms[k] /= N[k];

			SFMs[k][i] = geoms[k] / arithms[k];
		}
	}

	return SFMs;
}

std::vector<std::vector<float>> AudioFreqParams::getSCF(juce::AudioBuffer<float> audioData, int sampleRate)
{
	auto freqSpectrum = getFreqSpectrum(audioData, sampleRate, nullptr, 0.0f);
	const int freqSpectrumFrameSize = defaultFrameSize / 2 + 1;
	size_t frameCount = freqSpectrum.size() / freqSpectrumFrameSize;

	std::vector<std::vector<float>> SCFs(3);
	for (size_t i = 0; i < 3; i++)
	{
		SCFs[i].resize(frameCount);
	}
	const float eps = 1e-12f;
	for (size_t i = 0; i < frameCount; i++)
	{
		float maxPow[3] = { 0.0f, 0.0f, 0.0f };
		float arithms[3] = { 0.0f, 0.0f, 0.0f };
		int N[3] = { 0 };
		for (int j = 0; j < freqSpectrumFrameSize; j++)
		{
			float freq = (float)j * (float)sampleRate / (float)defaultFrameSize;
			float power = std::powf(freqSpectrum[i * freqSpectrumFrameSize + j], 2);
			if (freq <= 630)
			{
				if (maxPow[0] < power)
				{
					maxPow[0] = power;
				}
				arithms[0] += power;
				N[0]++;
			}
			else if (freq <= 1720)
			{
				if (maxPow[1] < power)
				{
					maxPow[1] = power;
				}
				arithms[1] += power;
				N[1]++;
			}
			else if (freq <= 4400)
			{
				if (maxPow[2] < power)
				{
					maxPow[2] = power;
				}
				arithms[2] += power;
				N[2]++;
			}
		}

		for (size_t k = 0; k < 3; k++)
		{
			arithms[k] /= N[k];
			SCFs[k][i] = maxPow[k] / arithms[k];
		}
	}

	return SCFs;
}

std::vector<float> AudioFreqParams::getCepstrumFrequences(juce::AudioBuffer<float> audioData, int sampleRate)
{
	auto freqSpectrum = getFreqSpectrum(audioData, sampleRate, nullptr, 0.0f);

	const int freqSpectrumFrameSize = defaultFrameSize / 2 + 1;
	size_t frameCount = freqSpectrum.size() / freqSpectrumFrameSize;

	std::vector<float> cepstrumFrequences(frameCount);

	fftwf_complex* FSpectrum = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * freqSpectrumFrameSize);
	float* frameCepstrum = (float*)fftwf_malloc(sizeof(float) * defaultFrameSize);

	fftwf_plan transformPlan = fftwf_plan_dft_c2r_1d(defaultFrameSize, FSpectrum, frameCepstrum, FFTW_ESTIMATE);
	
	const float eps = 1e-12f;
	const float minF = 50.0f;
	const float maxF = 400.0f;

	const int qMin = std::max(1, (int)std::floor(sampleRate / maxF));
	const int qMax = std::min(defaultFrameSize / 2, (int)std::ceil(sampleRate / minF));

	for (int i = 0; i < frameCount; i++)
	{
		for (int j = 0; j < freqSpectrumFrameSize; j++)
		{
			float mag = freqSpectrum[i * freqSpectrumFrameSize + j];

			FSpectrum[j][0] = std::log(mag + eps);
			FSpectrum[j][1] = 0.0f;
		}

		fftwf_execute(transformPlan);

		for (int j = 0; j < defaultFrameSize; j++)
		{
			frameCepstrum[j] /= defaultFrameSize;
		}

		int maxIdx = -1;
		float maxVal = -1.0f;

		for (int q = qMin; q <= qMax; q++)
		{
			if (frameCepstrum[q] > maxVal)
			{
				maxVal = frameCepstrum[q];
				maxIdx = q;
			}
		}

		cepstrumFrequences[i] = (maxIdx > 0) ? (float)sampleRate / (float)maxIdx : 0.0f;
	}

	fftwf_destroy_plan(transformPlan);
	fftwf_free(FSpectrum);
	fftwf_free(frameCepstrum);

	return cepstrumFrequences;
}

std::vector<float> AudioFreqParams::getFreqSpectrum(juce::AudioBuffer<float> audioData, int sampleRate, int* frameSize, float overlapLevel)
{
	int fSize = (frameSize != nullptr) ? *frameSize : defaultFrameSize;
	overlapLevel = std::clamp(overlapLevel, 0.0f, 0.99f);

	const auto* samples = audioData.getReadPointer(0);
	const int numSamples = audioData.getNumSamples();

	if (numSamples < fSize)
	{
		return {};
	}

	const int hopSize = std::roundf(fSize * (1.0f - overlapLevel));

	const int frameCount = (numSamples - fSize) / hopSize + 1;

	const int freqSpectrumFrameSize = fSize / 2 + 1;
	std::vector<float> freqSpectrum(freqSpectrumFrameSize * frameCount);

	for (int i = 0; i < frameCount; i++)
	{
		std::vector<float> frame(fSize);

		for (int j = 0; j < fSize; j++)
		{
			frame[j] = samples[i * hopSize + j];
		}

		chosenWindowFunction(frame);

		float* in = (float*)fftwf_malloc(sizeof(float) * fSize);
		fftwf_complex* out = (fftwf_complex*)fftwf_malloc(sizeof(fftwf_complex) * freqSpectrumFrameSize);

		for (int j = 0; j < fSize; j++)
		{
			in[j] = frame[j];
		}

		fftwf_plan plan = fftwf_plan_dft_r2c_1d(fSize, in, out, FFTW_ESTIMATE);

		fftwf_execute(plan);

		for (int k = 0; k < freqSpectrumFrameSize; k++)
		{
			float re = out[k][0];
			float im = out[k][1];

			float mag = std::sqrtf(re * re + im * im);

			freqSpectrum[i * freqSpectrumFrameSize + k] = mag;
		}

		fftwf_destroy_plan(plan);
		fftwf_free(in);
		fftwf_free(out);
	}

	return freqSpectrum;
}

void AudioFreqParams::chooseWindowFunction(WINDOW_FUNCTION choice)
{
	switch (choice)
	{
	case WINDOW_FUNCTION::RECTANGLE:
		chosenWindowFunction = rectangleWindowFunction;
		break;
	case WINDOW_FUNCTION::TRIANGLE:
		chosenWindowFunction = triangleWindowFunction;
		break;
	case WINDOW_FUNCTION::HAMMING:
		chosenWindowFunction = hammingWindowFunction;
		break;
	case WINDOW_FUNCTION::VAN_HANN:
		chosenWindowFunction = vanHannWindowFunction;
		break;
	case WINDOW_FUNCTION::BLACKMAN:
		chosenWindowFunction = blackmanWindowFunction;
		break;
	}
}

void AudioFreqParams::rectangleWindowFunction(std::vector<float>& frame)
{
	for (size_t i = 0; i < frame.size(); i++)
	{
		frame[i] *= 1.0f;
	}
}

void AudioFreqParams::triangleWindowFunction(std::vector<float>& frame)
{
	const float tmpDiffVal = (float)(defaultFrameSize - 1) / 2.0f;

	for (size_t i = 0; i < frame.size(); i++)
	{
		frame[i] *= 1.0f - std::abs((float)(i - tmpDiffVal) / tmpDiffVal);
	}
}

void AudioFreqParams::hammingWindowFunction(std::vector<float>& frame)
{
	for (size_t i = 0; i < frame.size(); i++)
	{
		frame[i] *= 0.53836f - 0.46164f *
			std::cosf(2.0f * std::numbers::pi * i / (float)(frame.size() - 1));
	}
}

void AudioFreqParams::vanHannWindowFunction(std::vector<float>& frame)
{
	for (size_t i = 0; i < frame.size(); i++)
	{
		frame[i] *= 0.5f * (1.0f - 
			std::cosf(2.0f * std::numbers::pi * i / (float)(frame.size() - 1)));
	}
}

void AudioFreqParams::blackmanWindowFunction(std::vector<float>& frame)
{
	const float a = 0.16f;
	const float a0 = (1.0f - a) / 2.0f;
	const float a1 = 0.5f;
	const float a2 = a / 2.0f;

	for (size_t i = 0; i < frame.size(); i++)
	{
		frame[i] *= a0 - a1 * std::cosf(2.0f * std::numbers::pi * i / (float)(frame.size() - 1))
			+ a2 * std::cosf(4.0f * std::numbers::pi * i / (float)(frame.size() - 1));
	}
}


