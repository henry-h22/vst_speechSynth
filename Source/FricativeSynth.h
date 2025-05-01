#pragma once

#include <cmath>

// ===========================
// ===========================
// SOUND
class FricativeSynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote      (int midiNote) override      {
        return ((midiNote == 79) || (midiNote == 81) || (midiNote == 83) || (midiNote == 84) || (midiNote == 86));
    }
    //--------------------------------------------------------------------------
    bool appliesToChannel   (int) override      { return true; }
};

// =================================
// =================================
// Synthesiser Voice - your synth code goes in here

/*!
 @class FricativeSynthVoice
 @abstract struct defining the DSP associated with a specific voice.
 
 @namespace none
 @updated 2019-06-18
 */
class FricativeSynthVoice : public juce::SynthesiserVoice
{
public:
    FricativeSynthVoice() {}
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
        
        rand.setSeed(222);
        
        switch(midiNoteNumber)
        {
            case 79:
                currentPlace = Place::labiodental;
                noiseFilterBand = 3000;
                noiseFilterQ = 2;
                break;
            case 81:
                currentPlace = Place::interdental;
                noiseFilterBand = 8222;
                noiseFilterQ = 2;
                break;
            case 83:
                currentPlace = Place::alveolar;
                noiseFilterBand = 5500;
                noiseFilterQ = 4;
                break;
            case 84:
                currentPlace = Place::postalveolar;
                noiseFilterBand = 4000;
                noiseFilterQ = 3;
                break;
            case 86:
                currentPlace = Place::glottal;
                noiseFilterBand = 3222;
                noiseFilterQ = 0.5;
                break;
            
            default:
                //this should NEVER happen. using dummy values.
                currentPlace = Place::null;
                noiseFilterBand = 5000; // we throw these here just in case
                noiseFilterQ = 3;
        }
        
        noiseFilter.setCoefficients(juce::IIRCoefficients::makeBandPass(double(sampleRate), noiseFilterBand, noiseFilterQ));
        noiseFilter.reset();
        
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
        if (!env.isActive())
        {
            clearCurrentNote();
            currentPlace = Place::null;
            return;
        }
        
        float envSample, randomSample, filteredSample, currentSample, outputSample;
        
        // iterate through the necessary number of samples (from startSample up to startSample + numSamples)
        for (int sampleIndex = startSample;   sampleIndex < (startSample+numSamples);   sampleIndex++)
        {
            
            envSample = env.getNextSample();
            randomSample = noiseFilter.processSingleSampleRaw((rand.nextFloat() - 1));
            filteredSample = lowPass.processSingleSampleRaw(outputBuffer.getSample(0, sampleIndex));
            
            // for each channel, write the currentSample float to the output
            for (int chan = 0; chan<outputBuffer.getNumChannels(); chan++)
            {
                currentSample = outputBuffer.getSample(chan, sampleIndex);
                
                outputSample = ((1 - envSample) * currentSample) +
                (envSample * *gain * ((0.02 * randomSample) + (0.7 * currentSample)));
                                 
                outputBuffer.setSample(chan, sampleIndex, outputSample);
            }
        }
    }
    
    void connectParameters(juce::AudioProcessorValueTreeState& apvts)
    {
        gain = apvts.getRawParameterValue("gain");
    }
    
    void setSampleRate(double sampleRate)
    {
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
     @return sound cast as a pointer to an instance of FricativeSynthSound
     */
    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<FricativeSynthSound*> (sound) != nullptr;
    }
    
    //--------------------------------------------------------------------------
private:
    /// ADSR
    juce::ADSR env;
    
    juce::IIRFilter lowPass;
    static inline float LOW_PASS_THRESHOLD = 1100.0f;
    
    juce::IIRFilter noiseFilter;
    float noiseFilterBand; // in Hz, where is the aperiodic energy concentrated?
    float noiseFilterQ;
    
    /// Parameters pointers
    std::atomic<float>* gain;
    
    double sampleRate;
    int sampleCounter;
    
    enum class Place { null, labiodental, interdental, alveolar, postalveolar, glottal };
    Place currentPlace = Place::null;
    
    juce::Random rand;
};
