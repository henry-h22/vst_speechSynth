/*
  ==============================================================================

    LF-Glottis.h
    Created: 28 Apr 2025 12:43:07pm
    Author:  Henry Heyden

  ==============================================================================
*/

#pragma once

#include <cmath>
#include <map>

/**
 This class implements the model of the vocal folds/glottis described in J. P. Cabral, S. Renals, K. Richmond, and J. Yamagishi, “Towards an improved modeling of the glottal source in statistical parametric speech synthesis,” in 6th ISCA Workshop on Speech Synthesis (SSW 6), pp. 113–118, 2007.
 */
class LFGlottis
{
public:
    
    /**
     Constructor. Does a lot.
     @param sampleRate_ the sample rate, in Hz
     @param frequency_ the desired fundamental frequency, in Hz
     */
    LFGlottis(float sampleRate_, float frequency_)
    {
        sampleRate = sampleRate_;
        frequency = frequency_;
        
        calculateVariables();
        
        rand.setSeed(222);
    }
    
    /**
     Default constructor.
     */
    LFGlottis()
    {
        LFGlottis(44100.0f, 87.307f);
    }
    
    /**
     Our favorite function. Returns the next sample for the glottis model, and increments time.
     */
    float process()
    {
        float currentSample = 0.022 * (rand.nextFloat() - 1);
        if (t > t_e)
        {
            currentSample += DEF(t);
        }
        else
        {
            currentSample += EISW(t);
        }
        incrementTime();
        return currentSample;
    }
    
    void setSampleRate(float sampleRate_)
    {
        sampleRate = sampleRate_;
        tIncrement = 1 / sampleRate;
    }
    
private:
    float sampleRate;
    double tIncrement;
    float frequency;
    float OQ;
    float SQ;
    float RQ;
    
    float T_0; // fundamental period (1/frequency)
    float t_c; // the time at which the glottis closes. equal to T_0 in this implementation, as in Cabral et. al 2007.
    float t_e; // the time at which amplification is -1.
    float t_a; // amount of time between the maximal negative amplification and the point at which the tangent crosses the x axis.
    float t_p; // the time at which we cross the x axis.
    float alpha; // a constant related to the values of other parameters.
    float E_0; // a constant related to the values of other parameters.
    float eps; // a constant related to the values of other parameters.
    
    float t; // we operate directly with time here because it works well with our functions, which are defined with respect to time, rather than something more traditional like phase.
    
    juce::Random rand;
    
    void calculateVariables()
    {
        T_0 = 1 / frequency;
        t_c = T_0;
        
        OQ = getOQ(frequency);
        SQ = getSQ(frequency);
        RQ = getRQ(frequency);
        
        t_a = T_0 * RQ;
        t_e = (OQ * T_0) - t_a;
        t_p = (SQ * t_e) / (SQ + 1);
        alpha = SQ / (SQ + 1);
        E_0 = -1 / ( exp(alpha * t_e) * sin((M_PI * t_e) / t_p) );
        eps = epsilonMap[int(frequency)];
        t = 0;
        tIncrement = 1 / sampleRate;
        
    }
    
    void incrementTime()
    {
        t += tIncrement;
        if (t >= T_0)
        {
            t -= T_0;
        }
    }
    
    //! These two functions are the two pairwise functions that drive the glottis model:
    
    /**
     EISW, standing for Exponentially Decaying Sin Wave, is the first of the two pairwise equations, and is active up to and including t = t_e. _
     @param t the current time value for the glottis.
     */
    float EISW(float t)
    {
        return E_0 * exp(alpha * t) * sin((M_PI * t) / t_p);
    }
    
    /**
     DEF, standing for decaying exponential function, is the second of the two pairwise equations, and is active immedeately following t = t_e, _ until t = t_c. _
     @param t the current time value for the glottis.
     */
    float DEF(float t)
    {
        return ( -1 / (eps * t_p) ) * ( exp( -eps * (t - t_e) ) - exp( -eps * (t_c - t_e) ) );
    }
    
    // This map holds the epsilon (eps) value for each possible frequency. It's faster than estimating this value every time.
    // Hand estimated using Desmos and eyes, lining up the pairwise equations at t = t_e, where they meet at -1.0.
    inline static std::map<int, float> epsilonMap
    {
        //creaky notes
        {65, 416.11},
        {69, 440.86},
        {73, 467.07},
        {77, 495},
        
        // modal notes
        {82, 250.63705149}, // E4
        {87, 265.54049544}, // F4
        {92, 281.33052414}, // F#4
        {97, 298.05945066}, // G4
        {103, 315.78232536}, // etc..
        {110, 334.56028152},
        {116, 354.45445244},
        {123, 375.53}, // this was when i discovered there's no audible difference between a really specific estimation and 2 decimal points.
        {130, 397.86212},
        {138, 421.52},
        {146, 446.583},
        {155, 473.14},
        {164, 501.27},
        {174, 531.08},
        {184, 562.66},
        {195, 596.12},
        {207, 631.56},
        {220, 669.12},
        {233, 708.91},
        {246, 751.06},
        {261, 795.722},
        
        // breathy notes
        {277, -1711.28},
        {293, -1813.22},
        {311, -1920.82},
        {329, -2035.07},
        {349.228, -2156.07}
    };
    
    // These three functions return the right values for the Q glottal parameters:
    
    inline static int CREAKY_THRESHOLD = 80;
    inline static int BREATHY_THRESHOLD = 276;
    
    inline static float getOQ(float frequency)
    {
        if (frequency < CREAKY_THRESHOLD)
        {
            return 0.22f;
        }
        else if (frequency > BREATHY_THRESHOLD)
        {
            return 1.0f;
        }
        else
        {
            return 0.44f;
        }
    }
    
    inline static float getSQ(float frequency)
    {
        if (frequency < CREAKY_THRESHOLD)
        {
            return 2.9f;
        }
        else if (frequency > BREATHY_THRESHOLD)
        {
            return 1.5f;
        }
        else
        {
            return 2.22f;
        }
    }
    
    inline static float getRQ(float frequency)
    {
        if (frequency < CREAKY_THRESHOLD)
        {
            return 0.01f;
        }
        else if (frequency > BREATHY_THRESHOLD)
        {
            return 0.22f;
        }
        else
        {
            return 0.04f;
        }
    }
};
