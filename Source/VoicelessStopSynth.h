#pragma once

#include <cmath>

// ===========================
// ===========================
// SOUND
class VoicelessStopSynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote      (int midiNote) override      {
        return ((midiNote == 72) || (midiNote == 74) || (midiNote == 76));
    }
    //--------------------------------------------------------------------------
    bool appliesToChannel   (int) override      { return true; }
};



// =================================
// =================================
// Synthesiser Voice - your synth code goes in here

/*!
 @class VoicelessStopSynthVoice
 @abstract struct defining the DSP associated with a specific voice.
 
 @namespace none
 @updated 2019-06-18
 */
class VoicelessStopSynthVoice : public juce::SynthesiserVoice
{
public:
    VoicelessStopSynthVoice() {}
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
        
        burst = false;
        
        switch(midiNoteNumber) {
            case 60:
                currentPhone = Phone::p;
                
                noiseFilterBand = 2500;
                noiseFilterQ = 1;
                VOT = 8;
                
                break;
                
            case 62:
                currentPhone = Phone::t;
                
                noiseFilterBand = 5555;
                noiseFilterQ = 4;
                VOT = 12;
                
                break;
                
            case 64:
                currentPhone = Phone::k;
                
                noiseFilterBand = 1500;
                noiseFilterQ = 2;
                VOT = 22;
                
                break;
                
            default:
                //this should NEVER happen. using dummy values.
                currentPhone = Phone::null;
                noiseFilterBand = 1500;
                noiseFilterQ = 0.9;
                VOT = 22;
        }
        
        noiseFilter.setCoefficients(juce::IIRCoefficients::makeBandPass(double(sampleRate), noiseFilterBand, noiseFilterQ));
        noiseFilter.reset();
        
        env.setParameters(juce::ADSR::Parameters(0.035f, 0.25f, 1.0f, 0.1f));
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
        
        float envSample, randomSample, noiseSample, currentSample, outputSample;
        
        // iterate through the necessary number of samples (from startSample up to startSample + numSamples)
        for (int sampleIndex = startSample;   sampleIndex < (startSample+numSamples);   sampleIndex++)
        {
            
            envSample = env.getNextSample();
            if (envSample == 1.0) { burst = true; }
            
            randomSample = rand.nextFloat() - 1;
            noiseSample = noiseFilter.processSingleSampleRaw(randomSample);
            
            switch(currentPhone) {
                case Phone::p:
                    
                    *f2d = -formantChangeThreshold * 0.9 * std::pow(envSample, 0.12);
                    
                    *f3d = -formantChangeThreshold * 0.9 * std::pow(envSample, 0.12);
                    
                    break;
                    
                case Phone::t:
                    
                    *f3d = formantChangeThreshold * 1.22 * std::pow(envSample, 0.12);
                    
                    break;
                    
                case Phone::k:
                    
                    *f2d = formantChangeThreshold * std::pow(envSample, 0.12);
                    
                    *f3d = -formantChangeThreshold * std::pow(envSample, 0.12);
                    
                    break;
                    
                default:
                    //this should NEVER happen.
                    currentPhone = Phone::null;
            }
            
            // for each channel, write the currentSample float to the output
            for (int chan = 0; chan<outputBuffer.getNumChannels(); chan++)
            {
                currentSample = outputBuffer.getSample(chan, sampleIndex);
                
                if (burst && (envSample < 1.0))
                {
                    float noiseComponent = (0.99 * noiseSample) + (0.01 * randomSample);
                    float burstComponent = (-(std::pow(2 * envSample - 1, 4) + 1)) * noiseComponent;
                    float voiceOnsetComponent = currentSample * std::pow((1 - envSample), VOT);
                    outputSample = ( *gain * 0.05 * burstComponent ) + voiceOnsetComponent;
                }
                else
                {
                    outputSample = currentSample * (1 - envSample);
                }
                outputBuffer.setSample(chan, sampleIndex, outputSample);
            }
        }
    }
    
    void connectParameters(juce::AudioProcessorValueTreeState& apvts) {
        gain = apvts.getRawParameterValue("gain");
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
    }
    
    //--------------------------------------------------------------------------
    void pitchWheelMoved(int) override {}
    //--------------------------------------------------------------------------
    void controllerMoved(int, int) override {}
    //--------------------------------------------------------------------------
    /**
     Can this voice play a sound. I wouldn't worry about this for the time being

     @param sound a juce::SynthesiserSound* base class pointer
     @return sound cast as a pointer to an instance of VoicelessStopSynthSound
     */
    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<VoicelessStopSynthSound*> (sound) != nullptr;
    }
    
    //--------------------------------------------------------------------------
private:
    /// ADSR
    juce::ADSR env;
    
    juce::IIRFilter noiseFilter;
    float noiseFilterBand; // in Hz, where is the aperiodic energy concentrated?
    float noiseFilterQ;
    
    bool burst; // a flag that tells us if the mouth has fully closed, if yes, we start playing the burst when it opens again.
    float VOT; // voice onset time correlate-- really, it's an exponent that tells the voiced content how quickly to fade in.
    
    std::atomic<float>* gain;
    
    float* f1d;
    float* f2d;
    float* f3d;
    
    inline static float formantChangeThreshold = 0.7f; // this number is then multiplied by the formant being modified.
    
    double sampleRate;
    int sampleCounter;
    
    enum class Phone { null, p, t, k };
    Phone currentPhone = Phone::null;
    
    juce::Random rand;
};
