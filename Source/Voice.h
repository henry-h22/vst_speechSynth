#pragma once
#include <vector>
#include "LFGlottis.h"
#include "VocalTract.h"

/**
 Class built to perform source-filter model speech synthesis
 Has a Glottis and a Vocal Tract, like you!
 */
class Voice
{
public:
    
    /**
     Constructor of the Voice class
     @param sampleRate in Hz
     @param f0 fundamental frequency (frquency of the impulse train/glottis) in Hz
     @param formants size 3 array of floats corresponding to formant frequencies f1, f2, and f3 of the vocal tract
     */
    Voice(float sampleRate, float f0, float formants[3])
    {
        this -> sampleRate = sampleRate;
        frequency = f0;
        glottis = LFGlottis(sampleRate, f0);
        vocalTract.setSampleRateAndFormants(sampleRate, formants[0], formants[1], formants[2]);
    }
    
    /**
     Default constructor using 44100 Hz sample rate and 87.307 Hz frequency
     */
    Voice()
    {
        float I[3] = {260, 2465, 3375};
        Voice(44100.0f, 87.307f, I);
    }
    
    void resetVoice(float sampleRate, float f0, float f1, float f2, float f3)
    {
        this -> sampleRate = sampleRate;
        frequency = f0;
        resetGlottis(sampleRate, f0);
        vocalTract.setSampleRateAndFormants(sampleRate, f1, f2, f3);
    }
    
    void resetGlottis(float sampleRate_, float f0)
    {
        LFGlottis newGlot = LFGlottis(sampleRate_, f0);
        glottis = newGlot;
    }
    
    /**
     Process the next sample of the voice
     @return the next sample, [-1,1]
     */
    float process()
    {
        return vocalTract.processSample(glottis.process());
    }
    
    /**
     @param sampleRate in Hz
     */
    void setSampleRate(float sampleRate)
    {
        this -> sampleRate = sampleRate;
        glottis.setSampleRate(sampleRate);
        vocalTract.setSampleRate(sampleRate);
    }

    /**
     @param formants size 3 array of floats corresponding to formant frequencies f1, f2, and f3 of the vocal tract
     */
    void setFormants(float formants[3])
    {
        vocalTract.setFormants(formants[0], formants[1], formants[2]);
    }
    
    void setF1(float f1)
    {
        vocalTract.setF1(f1);
    }
    
    void setF2(float f2)
    {
        vocalTract.setF2(f2);
    }
   
   void setF3(float f3)
    {
       vocalTract.setF3(f3);
    }
    
    /**
     We need the vocal tract to know what the current rounding parameter is, in order to modify the weights of the formant bands.
     This function just passes that along.
     */
    void updateRoundingParameter(float rounding)
    {
        vocalTract.updateRoundingParameter(rounding);
    }
    
    /**
     A function that maps the three generally understandable parameters to concrete formant values, in Hz.
     */
    static std::vector<float> mapParametersToFormants(float closure, float advancement, float rounding)
    {
        std::vector<float> formants;
        
        // Calculate F1
        float f1 = (57.8 * closure) + 222;
        
        // Calculate a temporary representation of F2
        float f2_slope = ((-182.28 * (closure-4.3)) + 1517) / 10; // relationship between closure and the desired maximum value of advancement
        float f2_temp = f1 + (f2_slope * advancement) + 222;
        
        // Calculate F3
        float f3 = f2_temp + 1000;
        
        // Finish calculating F2
        float f2 = f2_temp * ( 1 - (0.05 * rounding) );
        
        formants.push_back(f1);
        formants.push_back(f2);
        formants.push_back(f3);
        
        return formants;
    }
    
private:
    float sampleRate;
    float frequency;
    LFGlottis glottis;
    VocalTract vocalTract;
};
