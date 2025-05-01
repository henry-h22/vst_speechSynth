#pragma once

#include <cmath>

// ===========================
// ===========================
// SOUND
class VoicedStopSynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote      (int midiNote) override      {
        return ((midiNote == 73) || (midiNote == 75) || (midiNote == 77));
    }
    //--------------------------------------------------------------------------
    bool appliesToChannel   (int) override      { return true; }
};


// =================================
// =================================
// Synthesiser Voice - your synth code goes in here

/*!
 @class VoicedStopSynthVoice
 @abstract struct defining the DSP associated with a specific voice.
 
 @namespace none
 @updated 2019-06-18
 */
class VoicedStopSynthVoice : public juce::SynthesiserVoice
{
public:
    VoicedStopSynthVoice() {}
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
        
        switch(midiNoteNumber) {
            case 73:
                currentPhone = Phone::b;
                break;
            case 75:
                currentPhone = Phone::d;
                break;
            case 77:
                currentPhone = Phone::g;
                break;
            default:
                //this should NEVER happen.
                currentPhone = Phone::null;
        }
        
        env.setParameters(juce::ADSR::Parameters(0.035f, 0.25f, 1.0f, 0.022f));
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
            currentPhone = Phone::null;
            return;
        }
        
        float envSample, filteredSample, currentSample;
        
        // iterate through the necessary number of samples (from startSample up to startSample + numSamples)
        for (int sampleIndex = startSample;   sampleIndex < (startSample+numSamples);   sampleIndex++)
        {
            
            envSample = env.getNextSample();
            
            switch(currentPhone) {
                case Phone::b:
                    
                    *f2d = -formantChangeThreshold * 0.9 * std::pow(envSample, 0.02);
                    
                    *f3d = -formantChangeThreshold * 0.9 * std::pow(envSample, 0.02);
                    
                    break;
                    
                case Phone::d:
                    
                    *f3d = formantChangeThreshold * 1.22 * std::pow(envSample, 0.12);
                    
                    break;
                    
                case Phone::g:
                    
                    *f2d = formantChangeThreshold * std::pow(envSample, 0.12);
                    
                    *f3d = -formantChangeThreshold * std::pow(envSample, 0.12);
                    
                    break;
                    
                default:
                    //this should NEVER happen.
                    currentPhone = Phone::null;
            }
            
            filteredSample = lowPass.processSingleSampleRaw(outputBuffer.getSample(0, sampleIndex));
            
            // for each channel, write the currentSample float to the output
            for (int chan = 0; chan<outputBuffer.getNumChannels(); chan++)
            {
                currentSample = outputBuffer.getSample(chan, sampleIndex);
                outputBuffer.setSample(chan, sampleIndex,
                                       ( currentSample * (1 - envSample) ) + (filteredSample * (std::pow(envSample, 0.22) * 0.22)) );
            }
        }
    }
    
    void connectFormantDrift(float* f1Drift, float* f2Drift, float* f3Drift)
    {
        f1d = f1Drift;
        f2d = f2Drift;
        f3d = f3Drift;
    }
    
    void setSampleRate(double sampleRate) {
        this -> sampleRate = sampleRate;
        env.setSampleRate(sampleRate);
        lowPass.setCoefficients(juce::IIRCoefficients::makeLowPass(sampleRate, LOW_PASS_THRESHOLD));
    }
    
    //--------------------------------------------------------------------------
    void pitchWheelMoved(int) override {}
    //--------------------------------------------------------------------------
    void controllerMoved(int, int) override {}
    //--------------------------------------------------------------------------
    /**
     Can this voice play a sound. I wouldn't worry about this for the time being

     @param sound a juce::SynthesiserSound* base class pointer
     @return sound cast as a pointer to an instance of VoicedStopSynthSound
     */
    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<VoicedStopSynthSound*> (sound) != nullptr;
    }
    
    //--------------------------------------------------------------------------
private:
    /// ADSR
    juce::ADSR env;
    
    juce::IIRFilter lowPass;
    static inline float LOW_PASS_THRESHOLD = 80.0f;
    
    float* f1d;
    float* f2d;
    float* f3d;
    
    float formantChangeThreshold = 0.7f; // this number is then multiplied by the formant being modified.
    
    double sampleRate;
    int sampleCounter;
    
    enum class Phone { null, b, d, g };
    Phone currentPhone = Phone::null;
};
