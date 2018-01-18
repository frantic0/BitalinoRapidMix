//
//  BITSynth.hpp
//  Bitalino
//
//  Created by James on 07/12/2017.
//
//

#ifndef BITSynth_hpp
#define BITSynth_hpp

#include <stdio.h>
#include <vector>
#include "ofxMaxim.h"
#include "RingBuffer.hpp"
#include "GlobalDefs.h"

namespace BIT {
// Synthesizer
    
class Synth {
public:
    // From data thread to Audio thread
    RingBuffer<BIT::SynthControlData> controlDataBuffer;
    
    Synth ( void );
    void audioOut( float *output, int bufferSize, int nChannels );
    
private:
    // ofMaxim audio stuff
    maxiOsc VCO1, VCO2;
    maxiSVF SVF;
    
    maxiFractionalDelay DL1, DL2;
    
    // For smoothing parameter changes:
    maxiFilter dFilt1, dFilt2, dFilt3;
    
    // Audio thread synced
    BIT::SynthControlData audioControlData;
};

}

#endif /* BITSynth_hpp */
