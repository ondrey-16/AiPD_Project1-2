#include "VoiceVerifierComponent.h"

VoiceVerifierComponent::VoiceVerifierComponent()
{
	addAndMakeVisible(referenceVoiceButton);
	addAndMakeVisible(verifyVoiceButton);

	addAndMakeVisible(referenceFileLabel);
	addAndMakeVisible(verifyFileLabel);
	addAndMakeVisible(resultLabel);
	addAndMakeVisible(distanceValueLabel);

	referenceVoiceButton.setButtonText("Choose reference voice");
	verifyVoiceButton.setButtonText("Choose voice to verify");

	referenceFileLabel.setText("Reference voice not selected", juce::dontSendNotification);
	verifyFileLabel.setText("Voice to verify not selected", juce::dontSendNotification);
	resultLabel.setText("Result: -", juce::dontSendNotification);
	distanceValueLabel.setText("Distance: -", juce::dontSendNotification);

	referenceVoiceButton.onClick = [this]
	{
		chooseReferenceVoice();
	};

	verifyVoiceButton.onClick = [this]
	{
		chooseVerifyVoice();
	};

	verifyVoiceButton.setEnabled(false);
}

void VoiceVerifierComponent::paint(juce::Graphics& g)
{
	g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

	g.setColour(juce::Colours::white);
	g.setFont(22.0f);
	g.drawText("Voice verification", 20, 20, getWidth() - 40, 30, juce::Justification::centredLeft);
}

void VoiceVerifierComponent::resized()
{
	int x = 50;
	int y = 80;
	int width = 300;
	int height = 30;
	int gap = 45;

	referenceVoiceButton.setBounds(x, y, width, height);
	referenceFileLabel.setBounds(x + 330, y, getWidth() - x - 360, height);

	y += gap;

	verifyVoiceButton.setBounds(x, y, width, height);
	verifyFileLabel.setBounds(x + 330, y, getWidth() - x - 360, height);

	y += gap * 2;

	resultLabel.setBounds(x, y, 500, height);
	y += gap;

	distanceValueLabel.setBounds(x, y, 500, height);
}

std::vector<float> VoiceVerifierComponent::getMFCCParams(const juce::AudioBuffer<float>& audioData, int sampleRate)
{
	auto mfccs = audioFreqParamsAnalyzer.getMFCC(audioData, sampleRate);

	if (mfccs.empty())
	{
		return {};
	}

	const int frameCount = mfccs.size();
	const int filterCount = mfccs[0].size();

	std::vector<float> means(filterCount, 0.0f);

	for (int frame = 0; frame < frameCount; frame++)
	{
		for (int i = 0; i < filterCount; i++)
		{
			means[i] += mfccs[frame][i];
		}
	}

	for (float& val : means)
	{
		val /= (float)(frameCount);
	}

	std::vector<float> mfccParams(filterCount);

	for (int i = 0; i < filterCount; i++)
	{
		mfccParams[i] = means[i];
	}

	return mfccParams;
}

float VoiceVerifierComponent::getDistance(const std::vector<float>& a, const std::vector<float>& b)
{
	if (a.empty() || b.empty())
		return -1.0f;

	float sum = 0.0f;

	for (size_t i = 0; i < a.size(); i++)
	{
		float diff = a[i] - b[i];
		sum += diff * diff;
	}

	return std::sqrt(sum / static_cast<float>(a.size()));
}

void VoiceVerifierComponent::chooseReferenceVoice()
{
	fileChooser = std::make_unique<juce::FileChooser>(
		"Choose to verify .wav file",
		juce::File::getCurrentWorkingDirectory(),
		"*.wav"
	);

	fileChooser->launchAsync(
		juce::FileBrowserComponent::openMode |
		juce::FileBrowserComponent::canSelectFiles,
		[this](const juce::FileChooser& chooser)
		{
			auto result = chooser.getResult();

			if (result.existsAsFile())
			{
				WAVDataAnalyzer wavDataAnalyzer;

				referenceAudioData = wavDataAnalyzer.parseWAVFile(result);
				referenceSampleRate = wavDataAnalyzer.getSampleRate();

				hasReferenceVoice = true;

				referenceFileLabel.setText(
					"Reference voice: " + result.getFileName(),
					juce::dontSendNotification
				);

				verifyVoiceButton.setEnabled(hasReferenceVoice);

				verifyFileLabel.setText("Voice to verify not selected", juce::dontSendNotification);
				resultLabel.setText("Result: reference voice loaded", juce::dontSendNotification);
				distanceValueLabel.setText("Distance: -", juce::dontSendNotification);
			}
		}
	);
}

void VoiceVerifierComponent::chooseVerifyVoice()
{
	fileChooser = std::make_unique<juce::FileChooser>(
		"Choose to verify .wav file",
		juce::File::getCurrentWorkingDirectory(),
		"*.wav"
	);

	fileChooser->launchAsync(
		juce::FileBrowserComponent::openMode |
		juce::FileBrowserComponent::canSelectFiles,
		[this](const juce::FileChooser& chooser)
		{
			auto result = chooser.getResult();

			if (result.existsAsFile())
			{
				WAVDataAnalyzer wavDataAnalyzer;

				verifyAudioData = wavDataAnalyzer.parseWAVFile(result);
				verifySampleRate = wavDataAnalyzer.getSampleRate();

				hasVerifyVoice = true;

				verifyFileLabel.setText(
					"Voice to verify: " + result.getFileName(),
					juce::dontSendNotification
				);

				if (hasReferenceVoice && hasVerifyVoice)
				{
					verifyVoice();
				}
			}
		}
	);
}

void VoiceVerifierComponent::verifyVoice()
{
	if (!hasReferenceVoice || !hasVerifyVoice)
	{
		return;
	}

	auto referenceVector = getMFCCParams(referenceAudioData, referenceSampleRate);
	auto verifyVector = getMFCCParams(verifyAudioData, verifySampleRate);

	const float distance = getDistance(referenceVector, verifyVector);

	const float threshold = 0.5f;

	const bool accepted = distance <= threshold;

	resultLabel.setText(
		accepted ? "Result: YES - it is the same voice!"
		: "Result: NO - the voices are different!",
		juce::dontSendNotification
	);

	distanceValueLabel.setText("Distance: " + juce::String(distance, 4), juce::dontSendNotification);
}


