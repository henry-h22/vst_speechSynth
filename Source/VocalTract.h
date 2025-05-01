#pragma once
#include <vector>

/**
 Class to handle the vector of filters that are meant to simulate the resonance of a human vocal tract
 */
class VocalTract
{
public:

    /**
     Constructor for the Vocal Tract class, uses default formants. Do all setup with setter functions please, thanks!
     */
    VocalTract()
    {
        for (int i = 0; i < 3; i++)
        {
            juce::IIRFilter quinn;
            filtersVector.push_back(quinn);
        }
        setSampleRateAndFormants(44100.0f, 650, 1080, 2650);
    }
    
    /**
     Change the sample rate, in Hz
     @param sampleRate the new sample rate in Hz
     */
    void setSampleRate(float sampleRate)
    {
        setSampleRateAndFormants(sampleRate, formants[0], formants[1], formants[2]);
    }
    
    /**
     Change the formant frequencies
     @param f1 the first formant in Hz
     @param f2 the second formant in Hz
     @param f3 the third formant in Hz
     */
    void setFormants(float f1, float f2, float f3)
    {
        setSampleRateAndFormants(sampleRate, f1, f2, f3);
    }
    
    void setF1(float f1)
    {
        setSampleRateAndFormants(sampleRate, f1, formants[1], formants[2]);
    }
    
    void setF2(float f2)
    {
        setSampleRateAndFormants(sampleRate, formants[0], f2, formants[2]);
    }
    
    void setF3(float f3)
    {
        setSampleRateAndFormants(sampleRate, formants[0], formants[1], f3);
    }
    
    /**
     Change both the sample rate and the formants. This function is called by both setSampleRate() and setFormants(), because the work perfomed here needs to be done regardless
     @param sampleRate the new sample rate in Hz
     @param f1 the first formant in Hz
     @param f2 the second formant in Hz
     @param f3 the third formant in Hz
     */
    void setSampleRateAndFormants(float sampleRate, float f1, float f2, float f3)
    {
        this -> sampleRate = sampleRate;
        formants.clear();
        formants.push_back(f1);
        formants.push_back(f2);
        formants.push_back(f3);
        
        filtersVector[0].setCoefficients(juce::IIRCoefficients::makeBandPass(double(sampleRate), f1, QfromFormant(f1)));
        filtersVector[1].setCoefficients(juce::IIRCoefficients::makeBandPass(double(sampleRate), f2, QfromFormant(f2)));
        filtersVector[2].setCoefficients(juce::IIRCoefficients::makeBandPass(double(sampleRate), f3, QfromFormant(f3)));
    }
    
    /**
     Overloaded version of setFormants to take an array rather than 3 floats.
     @param formants the 3 formant frequencies in Hz
     */
    void setFormants(float formants[3])
    {
        setFormants(formants[0], formants[1], formants[2]);
    }
    
    void updateRoundingParameter(float rounding)
    {
        roundingParameter = rounding;
    }
    
    /** @return a vector of the three formants of the vocal tract, F1, F2, and F3 */
    std::vector<float> getFormants() {return formants;}
    
    /**
     Takes in a raw sample, ideally from the glottal impulse train, and applies each of the filters in the filter vector in parallel, in order to simulate the glottis
     @param rawSample the sample to be modified by the filter.
     */
    float processSample(float rawSample)
    {
        return      filtersVector[0].processSingleSampleRaw(rawSample) * (0.45 + (roundingParameter / 50))
                  + filtersVector[1].processSingleSampleRaw(rawSample) * 0.3
                  + filtersVector[2].processSingleSampleRaw(rawSample) * (0.25 - (roundingParameter / 50));
    }
    
private:
    std::vector<juce::IIRFilter> filtersVector;
    std::vector<float> formants;
    float sampleRate;
    float roundingParameter; // represents how round the vowel is, [0,10].
    
    /**
     Helper function to determine the bandwidth of a formant filter given the frequency
     This is based on the idea that lower formants have wider bands than higher filters
     @param formant in Hz
     @return Q value for the filter, based on the function 0.002x + 2.48
     */
    static float QfromFormant(float formant) {
        return (0.002 * formant) + 2.48;
    }
};
