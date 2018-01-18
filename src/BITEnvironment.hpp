//
//  BITEnvironment.hpp
//  Bitalino
//
//  Created by James on 23/11/2017.
//
//

#ifndef BITEnvironment_hpp
#define BITEnvironment_hpp

#include <stdio.h>
#include <vector>
#include <atomic>

#include "ofMain.h"
#include "BITSequencerThread.hpp"
#include "PokeGraph.hpp"

namespace BIT
{
// ML Bitalino Synth environment
// Functions as the view + a portion of the controller ( PokeGraphs )
class Environment
{
public:
    BITSequencerThread bst;
    
    void setup ( std::string nameOfBitalino, ofRectangle posAndSize );
    void update ( void );
    bool isMouseInside ( ofVec2f mousePos );
    void mouseActive ( ofVec2f mousePos, int8_t mouseState, bool mouseStateChanged );
    void draw ( void );
    void drawSelectedTrainingParts ( void );
    void resize ( ofRectangle posAndSize );
    void stop ( void );
    void audioOut(float * output, int bufferSize, int nChannels);
    
private:
    std::vector< double > selectedTrainingParts;
    
    ofRectangle posAndSize;
    
    PokeGraph* previousActive = nullptr;

    PokeGraph bitalinoGraph; // To show the input in sync with output parameters
    PokeGraph graphs[ NUM_SYNTH_PARAMS ]; // To show and allow editing of parameters
    
    double transport;
    
    State currentState;
    
};
    
} // End namespace BIT::
    
#endif /* BITSynth_hpp */
