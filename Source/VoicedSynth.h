#pragma once

#include "Voice.h"

#include <cmath>

// ===========================
// ===========================
// SOUND
class VoicedSynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote      (int midiNote) override      {
        return ((36 <= midiNote) && (midiNote <= 65));
    }
    //--------------------------------------------------------------------------
    bool appliesToChannel   (int) override      { return true; }
};




// =================================
// =================================
// Synthesiser Voice - your synth code goes in here

/*!
 @class VoicedSynthVoice
 @abstract struct defining the DSP associated with a specific voice.
 @discussion multiple VoicedSynthVoice objects will be created by the Synthesiser so that it can be played polyphicially
 
 @namespace none
 @updated 2019-06-18
 */
class VoicedSynthVoice : public juce::SynthesiserVoice
{
public:
    VoicedSynthVoice() {}
    //--------------------------------------------------------------------------
    /**
     What should be done when a note starts

     @param midiNoteNumber
     @param velocity
     @param SynthesiserSound unused variable
     @param / unused variable
     */
    void startNote (int midiNoteNumber, float velocity, juce::SynthesiserSound*, int /*currentPitchWheelPosition*/) override
    {
        
        f0 = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
        
        if (*manual == 0)
        {
            std::vector<float> formants = Voice::mapParametersToFormants(*closure, *advancement, *rounding);
            voice.resetVoice(float(sampleRate), f0, formants[0], formants[1], formants[2]);
        }
        else
        {
            voice.resetVoice(float(sampleRate), f0, *f1, *f2, *f3);
        }
        
        env.setParameters(juce::ADSR::Parameters(0.022f, 0.0f, 1.0f, 0.0022f));
        env.noteOn();
    }
    //--------------------------------------------------------------------------
    /// Called when a MIDI noteOff message is received
    /**
     What should be done when a note stops

     @param / unused variable
     @param allowTailOff bool to decie if the should be any volume decay
     */
    void stopNote(float /*velocity*/, bool allowTailOff) override
    {
        env.noteOff();
    }
    
    //--------------------------------------------------------------------------
    /**
     The Main DSP Block: Put your DSP code in here
     
     If the sound that the voice is playing finishes during the course of this rendered block, it must call clearCurrentNote(), to tell the synthesiser that it has finished

     @param outputBuffer pointer to output
     @param startSample position of frist sample in buffer
     @param numSamples number of smaples in output buffer
     */
    void renderNextBlock(juce::AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override
    {
        if (!env.isActive()) {
            clearCurrentNote();
            return;
        }
        
        float currentSample;

        // iterate through the necessary number of samples (from startSample up to startSample + numSamples)
        for (int sampleIndex = startSample;   sampleIndex < (startSample+numSamples);   sampleIndex++)
        {
            if (*manual == 0)
            {
                std::vector<float> formants = Voice::mapParametersToFormants(*closure, *advancement, *rounding);
                voice.setF1(formants[0] * (1 + *f1d));
                voice.setF2(formants[1] * (1 + *f2d));
                voice.setF3(formants[2] * (1 + *f3d));
            }
            else
            {
                voice.updateRoundingParameter(0.0);
                voice.setF1(*f1 * (1 + *f1d));
                voice.setF2(*f2 * (1 + *f2d));
                voice.setF3(*f3 * (1 + *f3d));
            }
            
            currentSample = voice.process();
            
            currentSample *= env.getNextSample() * (*gain);
            
            // for each channel, write the currentSample float to the output
            for (int chan = 0; chan<outputBuffer.getNumChannels(); chan++)
            {
                outputBuffer.addSample (chan, sampleIndex, currentSample);
            }
        }
    }
    
    void connectParameters(juce::AudioProcessorValueTreeState& apvts)
    {
        gain = apvts.getRawParameterValue("gain");
        
        closure = apvts.getRawParameterValue("closure");
        advancement = apvts.getRawParameterValue("advancement");
        rounding = apvts.getRawParameterValue("rounding");
        
        manual = apvts.getRawParameterValue("manual");
        
        f1 = apvts.getRawParameterValue("f1");
        f2 = apvts.getRawParameterValue("f2");
        f3 = apvts.getRawParameterValue("f3");
    }
    
    void connectFormantDrift(float* f1Drift, float* f2Drift, float* f3Drift)
    {
        f1d = f1Drift;
        f2d = f2Drift;
        f3d = f3Drift;
    }
    
    void setSampleRate(double sampleRate)
    {
        voice.setSampleRate(float(sampleRate));
        this -> sampleRate = sampleRate;
        env.setSampleRate(sampleRate);
    }
    
    //--------------------------------------------------------------------------
    void pitchWheelMoved(int) override {}
    //--------------------------------------------------------------------------
    void controllerMoved(int, int) override {}
    //--------------------------------------------------------------------------
    /**
     Can this voice play a sound. I wouldn't worry about this for the time being

     @param sound a juce::SynthesiserSound* base class pointer
     @return sound cast as a pointer to an instance of VoicedSynthSound
     */
    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<VoicedSynthSound*> (sound) != nullptr;
    }
    
    //--------------------------------------------------------------------------
private:
    //--------------------------------------------------------------------------
    // Set up any necessary variables here
    
    /// Voice
    Voice voice;
    
    float f0;
    double sampleRate;
    int sampleCounter;
    
    /// ADSR
    juce::ADSR env;
    
    /// Parameters pointers
    std::atomic<float>* gain;
    
    std::atomic<float>* closure;
    std::atomic<float>* advancement;
    std::atomic<float>* rounding;
    
    std::atomic<float>* manual;
    
    std::atomic<float>* f1;
    std::atomic<float>* f2;
    std::atomic<float>* f3;
    
    float* f1d;
    float* f2d;
    float* f3d;
};
