/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SpeechSynthAudioProcessor::SpeechSynthAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
apvts(*this, nullptr, "ParamTreeIdentifier", setupParameters())
{
    vocalSynth.addSound(new VoicedSynthSound());
    for (int voice = 0; voice < voiceCount; voice++) {
        vocalSynth.addVoice(new VoicedSynthVoice());
        VoicedSynthVoice* v = dynamic_cast<VoicedSynthVoice*>(vocalSynth.getVoice(voice));
        v -> connectParameters(apvts);
        v -> connectFormantDrift(&f1d, &f2d, &f3d);
    }
    
    voicedStopSynth.addSound(new VoicedStopSynthSound());
    voicedStopSynth.addVoice(new VoicedStopSynthVoice());
    VoicedStopSynthVoice* v_vStop = dynamic_cast<VoicedStopSynthVoice*>(voicedStopSynth.getVoice(0));
    v_vStop -> connectFormantDrift(&f1d, &f2d, &f3d);
    
    voicelessStopSynth.addSound(new VoicelessStopSynthSound());
    voicelessStopSynth.addVoice(new VoicelessStopSynthVoice());
    VoicelessStopSynthVoice* v_Stop = dynamic_cast<VoicelessStopSynthVoice*>(voicelessStopSynth.getVoice(0));
    v_Stop -> connectParameters(apvts);
    v_Stop -> connectFormantDrift(&f1d, &f2d, &f3d);
    
    fricativeSynth.addSound(new FricativeSynthSound());
    fricativeSynth.addVoice(new FricativeSynthVoice());
    FricativeSynthVoice* v_fric = dynamic_cast<FricativeSynthVoice*>(fricativeSynth.getVoice(0));
    v_fric -> connectParameters(apvts);
}

SpeechSynthAudioProcessor::~SpeechSynthAudioProcessor()
{
}

//==============================================================================

void SpeechSynthAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    vocalSynth.setCurrentPlaybackSampleRate(sampleRate);
    voicedStopSynth.setCurrentPlaybackSampleRate(sampleRate);
    voicelessStopSynth.setCurrentPlaybackSampleRate(sampleRate);
    fricativeSynth.setCurrentPlaybackSampleRate(sampleRate);
    
    f1d = 0;
    f2d = 0;
    f3d = 0;
    
    for (int voice = 0; voice < voiceCount; voice++) {
        VoicedSynthVoice* v = dynamic_cast<VoicedSynthVoice*>(vocalSynth.getVoice(voice));
        v -> setSampleRate(sampleRate);
    }
    
    VoicedStopSynthVoice* v_vStop = dynamic_cast<VoicedStopSynthVoice*>(voicedStopSynth.getVoice(0));
    v_vStop -> setSampleRate(sampleRate);
    
    VoicelessStopSynthVoice* v_Stop = dynamic_cast<VoicelessStopSynthVoice*>(voicelessStopSynth.getVoice(0));
    v_Stop -> setSampleRate(sampleRate);
    
    FricativeSynthVoice* v_fric = dynamic_cast<FricativeSynthVoice*>(fricativeSynth.getVoice(0));
    v_fric -> setSampleRate(sampleRate);
    
}


void SpeechSynthAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    vocalSynth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    
    fricativeSynth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    
    voicedStopSynth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
    
    voicelessStopSynth.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
}


//==============================================================================
const juce::String SpeechSynthAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool SpeechSynthAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SpeechSynthAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SpeechSynthAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SpeechSynthAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int SpeechSynthAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SpeechSynthAudioProcessor::getCurrentProgram()
{
    return 0;
}

void SpeechSynthAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String SpeechSynthAudioProcessor::getProgramName (int index)
{
    return {};
}

void SpeechSynthAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================

void SpeechSynthAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SpeechSynthAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif


//==============================================================================
bool SpeechSynthAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* SpeechSynthAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void SpeechSynthAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void SpeechSynthAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
        }
    }
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpeechSynthAudioProcessor();
}
