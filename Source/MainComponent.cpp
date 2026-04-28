#include "MainComponent.h"
#include "AudioTimeParams.h"

MainComponent::MainComponent()
{
	setApplicationCommandManagerToWatch(&this->commandManager);
	this->menuBar.reset(new juce::MenuBarComponent(this));
	addAndMakeVisible(menuBar.get());
    addAndMakeVisible(this->silenceButton);
    addAndMakeVisible(this->sonorousButton);
    addAndMakeVisible(this->speechMusicButton);

    addAndMakeVisible(this->selectFreqParamPlotButton);
    addAndMakeVisible(this->selectWindowFunctionButton);

    addAndMakeVisible(this->frame256SpectrogramChoiceButton);
    addAndMakeVisible(this->frame512SpectrogramChoiceButton);
    addAndMakeVisible(this->frame1024SpectrogramChoiceButton);

    addAndMakeVisible(this->overlap0SpectrogramChoiceButton);
    addAndMakeVisible(this->overlap25SpectrogramChoiceButton);
    addAndMakeVisible(this->overlap50SpectrogramChoiceButton);

    addAndMakeVisible(this->audioParamsPlot);

    addAndMakeVisible(this->timeParamsPlotsChoiceButton);
    addAndMakeVisible(this->freqParamsPlotsChoiceButton);
    addAndMakeVisible(this->spectrogramChoiceButton);

    this->silenceButton.setButtonText("Silence detection");
    this->sonorousButton.setButtonText("Sonorous frames detection");
    this->speechMusicButton.setButtonText("Music/Speech clips detection");

    this->timeParamsPlotsChoiceButton.setButtonText("Show time audio parameters");
    this->freqParamsPlotsChoiceButton.setButtonText("Show frequence audio parameters");
    this->spectrogramChoiceButton.setButtonText("Show spectrogram");

    this->frame256SpectrogramChoiceButton.setButtonText("Frame size 256");
    this->frame512SpectrogramChoiceButton.setButtonText("Frame size 512");
    this->frame1024SpectrogramChoiceButton.setButtonText("Frame size 1024");

    this->overlap0SpectrogramChoiceButton.setButtonText("Overlap 0%");
    this->overlap25SpectrogramChoiceButton.setButtonText("Overlap 25%");
    this->overlap50SpectrogramChoiceButton.setButtonText("Overlap 50%");

    this->silenceButton.setClickingTogglesState(true);
    this->sonorousButton.setClickingTogglesState(true);
    this->speechMusicButton.setClickingTogglesState(true);

    this->timeParamsPlotsChoiceButton.setClickingTogglesState(true);
    this->freqParamsPlotsChoiceButton.setClickingTogglesState(true);
    this->spectrogramChoiceButton.setClickingTogglesState(true);

    this->frame256SpectrogramChoiceButton.setClickingTogglesState(true);
    this->frame512SpectrogramChoiceButton.setClickingTogglesState(true);
    this->frame1024SpectrogramChoiceButton.setClickingTogglesState(true);

    this->overlap0SpectrogramChoiceButton.setClickingTogglesState(true);
    this->overlap25SpectrogramChoiceButton.setClickingTogglesState(true);
    this->overlap50SpectrogramChoiceButton.setClickingTogglesState(true);

    this->selectFreqParamPlotButton.addItem("Frequence parameters plot", 1);
    this->selectFreqParamPlotButton.addItem("Frequence band-parameters plot", 2);
    this->selectFreqParamPlotButton.addItem("Frame frequence spectrum", 3);
    this->selectFreqParamPlotButton.addItem("Laryngeal frequency plot", 4);

    this->selectWindowFunctionButton.addItem("Rectangle window", 1);
    this->selectWindowFunctionButton.addItem("Triangle window", 2);
    this->selectWindowFunctionButton.addItem("Hamming window", 3);
    this->selectWindowFunctionButton.addItem("van Hann window", 4);
    this->selectWindowFunctionButton.addItem("Blackman window", 5);

    this->timeParamsPlotsChoiceButton.setToggleState(true, juce::dontSendNotification);
    this->frame512SpectrogramChoiceButton.setToggleState(true, juce::dontSendNotification);
    this->overlap0SpectrogramChoiceButton.setToggleState(true, juce::dontSendNotification);
    this->selectFreqParamPlotButton.setSelectedId(1);
    this->selectWindowFunctionButton.setSelectedId(1);

    this->silenceButton.setRadioGroupId(1);
    this->sonorousButton.setRadioGroupId(1);
    this->speechMusicButton.setRadioGroupId(1);

    this->timeParamsPlotsChoiceButton.setRadioGroupId(2);
    this->freqParamsPlotsChoiceButton.setRadioGroupId(2);
    this->spectrogramChoiceButton.setRadioGroupId(2);

    this->frame256SpectrogramChoiceButton.setRadioGroupId(3);
    this->frame512SpectrogramChoiceButton.setRadioGroupId(3);
    this->frame1024SpectrogramChoiceButton.setRadioGroupId(3);

    this->overlap0SpectrogramChoiceButton.setRadioGroupId(4);
    this->overlap25SpectrogramChoiceButton.setRadioGroupId(4);
    this->overlap50SpectrogramChoiceButton.setRadioGroupId(4);

    setSize (1200, 800);
	this->audioData.setSize(0, 0);
    this->silenceButton.onClick = [this] {
        audioParamsPlot.showSilentFrames();
        repaint();
    };
    this->sonorousButton.onClick = [this] {
        audioParamsPlot.showSonorousFrames();
        repaint();
    };
    this->speechMusicButton.onClick = [this] {
        audioParamsPlot.showMusicSpeechClips();
        repaint();
    };
    this->timeParamsPlotsChoiceButton.onClick = [this] {
        this->plotPage = PLOT_PAGE::TIME_PARAMS;

        this->silenceButton.setVisible(true);
        this->sonorousButton.setVisible(true);
        this->speechMusicButton.setVisible(true);

        this->changeButtonsVisibility(true, false, false, false);

        audioParamsPlot.changePlotPage(this->plotPage);
        resized();
        repaint();
    };
    this->freqParamsPlotsChoiceButton.onClick = [this] {
        this->plotPage = PLOT_PAGE::FREQ_PARAMS;

        this->changeButtonsVisibility(false, true, false, true);

        audioParamsPlot.changePlotPage(this->plotPage);
        resized();
        repaint();
    };
    this->spectrogramChoiceButton.onClick = [this] {
        this->plotPage = PLOT_PAGE::SPECTROGRAM;

        this->silenceButton.setVisible(false);
        this->sonorousButton.setVisible(false);
        this->speechMusicButton.setVisible(false);

        this->selectFreqParamPlotButton.setVisible(false);

        this->changeButtonsVisibility(false, false, true, true);

        audioParamsPlot.changePlotPage(this->plotPage);
        resized();
        repaint();
    };

    this->frame256SpectrogramChoiceButton.onClick = [this] {
        this->audioParamsPlot.setFrameSize(256);
    };
    this->frame512SpectrogramChoiceButton.onClick = [this] {
        this->audioParamsPlot.setFrameSize(512);
    };
    this->frame1024SpectrogramChoiceButton.onClick = [this] {
        this->audioParamsPlot.setFrameSize(1024);
    };

    this->overlap0SpectrogramChoiceButton.onClick = [this] {
        this->audioParamsPlot.setOverlapLevel(0.0f);
    };
    this->overlap25SpectrogramChoiceButton.onClick = [this] {
        this->audioParamsPlot.setOverlapLevel(0.25f);
    };
    this->overlap50SpectrogramChoiceButton.onClick = [this] {
        this->audioParamsPlot.setOverlapLevel(0.5f);
    };

    this->selectFreqParamPlotButton.onChange = [this] {
        this->audioParamsPlot.chooseFrequenceParamsPlot(static_cast<FREQ_PARAMS_PLOT>(this->selectFreqParamPlotButton.getSelectedId()));
    };

    this->selectWindowFunctionButton.onChange = [this] {
        this->audioParamsPlot.chooseWindowFunction(static_cast<WINDOW_FUNCTION>(this->selectWindowFunctionButton.getSelectedId()));
    };
}

MainComponent::~MainComponent()
{
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void MainComponent::resized()
{
	if (this->menuBar != nullptr)
    {
        this->menuBar->setBounds(0, 0, this->getWidth(), 24);
    }

    this->timeParamsPlotsChoiceButton.setBounds(50, 40, 150, 30);
    this->freqParamsPlotsChoiceButton.setBounds(300, 40, 150, 30);
    this->spectrogramChoiceButton.setBounds(550, 40, 150, 30);
    
    switch (this->plotPage)
    {
        case PLOT_PAGE::TIME_PARAMS:
        {
            this->silenceButton.setBounds(50, 80, 150, 30);
            this->sonorousButton.setBounds(50, 110, 200, 30);
            this->speechMusicButton.setBounds(50, 140, 220, 30);
            break;
        }
        case PLOT_PAGE::FREQ_PARAMS:
        {
            this->selectWindowFunctionButton.setBounds(50, 80, 300, 30);
            this->selectFreqParamPlotButton.setBounds(50, 130, 300, 30);
            break;
        }
        case PLOT_PAGE::SPECTROGRAM:
        {
            this->selectWindowFunctionButton.setBounds(50, 80, 300, 30);
            this->frame256SpectrogramChoiceButton.setBounds(50, 140, 120, 30);
            this->frame512SpectrogramChoiceButton.setBounds(180, 140, 120, 30);
            this->frame1024SpectrogramChoiceButton.setBounds(300, 140, 120, 30);
            this->overlap0SpectrogramChoiceButton.setBounds(500, 140, 120, 30);
            this->overlap25SpectrogramChoiceButton.setBounds(630, 140, 120, 30);
            this->overlap50SpectrogramChoiceButton.setBounds(760, 140, 120, 30);
            break;
        }
    }

    auto bounds = this->getLocalBounds();

    auto plotArea = bounds.reduced(40);
    plotArea.removeFromTop(140);

    this->audioParamsPlot.setBounds(plotArea);
}

juce::StringArray MainComponent::getMenuBarNames()
{
    return { "New file:", "Save:"};
}

juce::PopupMenu MainComponent::getMenuForIndex(int topLevelMenuIndex, const juce::String& menuName)
{
	juce::PopupMenu menu;

	if (topLevelMenuIndex == 0)
    {
        menu.addItem(1, "Add .wav file");
    }
    else if (topLevelMenuIndex == 1)
    {
        menu.addItem(2, "Save parameters into file");
    }

	return menu;
}

void MainComponent::menuItemSelected(int menuItemID, int topLevelMenuIndex)
{
	if (menuItemID == 1)
    {
		fileChooser = std::make_unique<juce::FileChooser>("Choose a .wav file", juce::File::getCurrentWorkingDirectory(), "*.wav");
        fileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [this](const juce::FileChooser& chooser)
            {
                auto result = chooser.getResult();
                if (result.existsAsFile())
                {
                    juce::Logger::writeToLog("Selected file: " + result.getFullPathName());
                    juce::Logger::writeToLog("Added .wav file");
				    WAVDataAnalyzer wavDataAnalyzer;
				    audioData = wavDataAnalyzer.parseWAVFile(result);
				    sampleRate = wavDataAnalyzer.getSampleRate();
                    audioParamsPlot.setAudioData(audioData, sampleRate);
                    repaint();
                }
                else
                {
                    juce::Logger::writeToLog("No file selected");
                }
		    });
    }
    else if (menuItemID == 2)
    {
        fileChooser = std::make_unique<juce::FileChooser>(
            "Save parameters into txt file",
            juce::File::getCurrentWorkingDirectory().getChildFile("parameters.txt"),
            "*.txt");

        fileChooser->launchAsync(
            juce::FileBrowserComponent::saveMode
            | juce::FileBrowserComponent::canSelectFiles
            | juce::FileBrowserComponent::warnAboutOverwriting,
            [this](const juce::FileChooser& chooser)
            {
                auto result = chooser.getResult();

                if (result != juce::File{})
                {
                    juce::Logger::writeToLog("Saving to file: " + result.getFullPathName());

                    if (result.getFileExtension() != ".txt")
                        result = result.withFileExtension(".txt");

                    juce::String content;

                    content << "|Frame parameters|\n\n";

                    addParamsToFile("Volume", audioParamsPlot.volumesY, content);
                    addParamsToFile("STE", audioParamsPlot.STEsY, content);
                    addParamsToFile("ZCR", audioParamsPlot.ZCRsY, content);

                    content << "|Frequences|\n\n";

                    addParamsToFile("Autocorrelation frequences", audioParamsPlot.autoCorrFrequences, content);
                    addParamsToFile("AMDF frequences", audioParamsPlot.AMDFFrequences, content);

                    content << "|Clip parameters|\n\n";

                    addParamsToFile("VSTD", audioParamsPlot.VSTDsY, content);
                    addParamsToFile("VDR", audioParamsPlot.VDRsY, content);
                    addParamsToFile("VU", audioParamsPlot.VUsY, content);
                    addParamsToFile("LSTER", audioParamsPlot.LSTERsY, content);
                    addParamsToFile("Energy Entropy", audioParamsPlot.EntropiesY, content);
                    addParamsToFile("ZSTD", audioParamsPlot.ZSTDsY, content);
                    addParamsToFile("HZCRR", audioParamsPlot.HZCRRsY, content);

                    if (result.replaceWithText(content))
                        juce::Logger::writeToLog("File saved successfully");
                    else
                        juce::Logger::writeToLog("Error while saving file");
                }
                else
                {
                    juce::Logger::writeToLog("Save cancelled");
                }
            });
    }
}

void MainComponent::changeButtonsVisibility(bool forTimePageVisible, bool forFreqPageVisible, bool forSpectrogramVisible, bool forFreqAndSpectrogramPagesVisible)
{
    this->silenceButton.setVisible(forTimePageVisible);
    this->sonorousButton.setVisible(forTimePageVisible);
    this->speechMusicButton.setVisible(forTimePageVisible);

    this->selectFreqParamPlotButton.setVisible(forFreqPageVisible);

    this->frame256SpectrogramChoiceButton.setVisible(forSpectrogramVisible);
    this->frame512SpectrogramChoiceButton.setVisible(forSpectrogramVisible);
    this->frame1024SpectrogramChoiceButton.setVisible(forSpectrogramVisible);
    this->overlap0SpectrogramChoiceButton.setVisible(forSpectrogramVisible);
    this->overlap25SpectrogramChoiceButton.setVisible(forSpectrogramVisible);
    this->overlap50SpectrogramChoiceButton.setVisible(forSpectrogramVisible);

    this->selectWindowFunctionButton.setVisible(forFreqAndSpectrogramPagesVisible);
}

void MainComponent::drawFrameParamPlot(juce::Graphics& g, juce::Rectangle<int> plotArea, const std::vector<float>& params, juce::Colour color)
{
    g.setColour(color);

    int width = plotArea.getWidth();
    juce::Path path;

    const size_t N = params.size();
    for (size_t i = 0; i < N; i++)
    {
        int x1 = juce::jmap((float)i, 0.0f, (float)N, (float)plotArea.getX(), (float)plotArea.getRight());
        int x2 = juce::jmap((float)(i + 1), 0.0f, (float)N, (float)plotArea.getX(), (float)plotArea.getRight());
        int x = (x1 + x2) / 2;
        int y = juce::jmap((float)params[i], -1.0f, 1.0f, (float)plotArea.getBottom(), (float)plotArea.getY());

        if (i == 0)
        {
            path.startNewSubPath(x, y);
        }
        else
        {
            path.lineTo(x, y);
        }
    }

    g.strokePath(path, juce::PathStrokeType(1.0f));
}

void MainComponent::drawAudioPlot(juce::Graphics& g, juce::Rectangle<int> plotArea, const std::vector<float>& params, juce::Colour color)
{
    g.setColour(color);

    int width = plotArea.getWidth();

    for (int x = 0; x < width; ++x)
    {
        int pos1 = x / (float)(width - 1) * (params.size() - 1);
        int pos2 = std::min((int)((x + 1) / (float)(width - 1) * (params.size() - 1)), (int)params.size() - 1);

        float minVal = params[pos1];
        float maxVal = params[pos1];
        for (int i = pos1 + 1; i <= pos2; i++)
        {
            minVal = std::min(minVal, params[i]);
            maxVal = std::max(maxVal, params[i]);
        }

        int yPos1 = juce::jmap(minVal, -1.0f, 1.0f, (float)plotArea.getBottom(), (float)plotArea.getY());
        int yPos2 = juce::jmap(maxVal, -1.0f, 1.0f, (float)plotArea.getBottom(), (float)plotArea.getY());
        int xPos = x + plotArea.getX();

        g.drawLine(xPos, yPos1, xPos, yPos2);
    }
}

void MainComponent::drawDetection(juce::Graphics& g, juce::Rectangle<int> plotArea, const std::vector<bool>& silentFrames, juce::Colour color)
{
    g.setColour(color);

    int width = plotArea.getWidth();

    const size_t N = silentFrames.size();
    for (size_t i = 0; i < N; i++)
    {
        if (!silentFrames[i])
        {
            continue;
        }

        int x1 = juce::jmap((float)i, 0.0f, (float)N, (float)plotArea.getX(), (float)plotArea.getRight());
        while (i + 1 < N && silentFrames[i + 1])
        {
            i++;
        }
        int x2 = juce::jmap((float)(i + 1), 0.0f, (float)N, (float)plotArea.getX(), (float)plotArea.getRight());

        g.fillRect(x1, plotArea.getY(), x2 - x1, plotArea.getHeight());
    }
}

void MainComponent::addParamsToFile(const juce::String& name, const std::vector<float>& data, juce::String& content)
{
    content << name << ":\n";
    for (size_t i = 0; i < data.size(); i++)
    {
        content << i + 1 << ". " << data[i] << "\n";
    }
    content << "\n";
}
