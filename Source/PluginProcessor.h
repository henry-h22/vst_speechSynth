/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "VoicedSynth.h"
#include "FricativeSynth.h"
#include "VoicedStopSynth.h"
#include "VoicelessStopSynth.h"

//==============================================================================
/**
*/
class SpeechSynthAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    SpeechSynthAudioProcessor();
    ~SpeechSynthAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

private:
    
    double sampleRate;
    
    using APVTS = juce::AudioProcessorValueTreeState;
    int voiceCount = 4;
    juce::Synthesiser vocalSynth;
    juce::Synthesiser voicedStopSynth;
    juce::Synthesiser voicelessStopSynth;
    juce::Synthesiser fricativeSynth;
    APVTS apvts;
    
    // These three are hanging out here so that we can allow consonant synths make formants dip and rise
    float f1d;
    float f2d;
    float f3d;
    
    APVTS::ParameterLayout setupParameters()
    {
        APVTS::ParameterLayout layout;
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("gain", 22), "Gain", 0.0f, 1.0f, 0.9f));
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("closure", 22), "Closure", 0.0f, 10.0f, 10.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("advancement", 22), "Advancement", 0.0f, 10.0f, 1.76f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("rounding", 22), "Rounding", 0.0f, 10.0f, 0.0f));
        
        layout.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID("manual", 22), "Manual Formant Control", false));
        
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("f1", 22), "1st Formant", 285.0f, 700.0f, 290.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("f2", 22), "2nd Formant", 400.0f, 1900.0f, 1870.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("f3", 22), "3rd Formant", 1532.0f, 3400.0f, 3375.0f));
        
        return layout;
    }
    
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpeechSynthAudioProcessor)
};
