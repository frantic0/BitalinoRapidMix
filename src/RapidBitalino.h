//
//  RapidBitalino.h
//  Bitalino
//
//  Created by James on 20/11/2017.
//
//

#ifndef RapidBitalino_h
#define RapidBitalino_h

#include <algorithm>
#include <functional>

#include "ofMain.h"
#include "BitalinoThread.h"
#include "rapidmix.h"

// TODO ifdefs for osc debug

#include "ofxOsc.h"
#include "Tools.h"

#define HOST "localhost"
#define PORT 8338

#define NUM_INPUTS_FOR_TRAINING 1

class RapidBitalino {
public:
    BitalinoThread bitalinoThread; // For isConnected etc
    std::function< void ( void ) > buttonPressedCallback = nullptr;
    std::function< void ( void ) > buttonReleasedCallback = nullptr;
    
    RapidBitalino ( void );
    ~RapidBitalino ( void );
    
    void setup ( std::string nameOfBitalino, uint32_t sampleEveryN );
    void setDownsample ( uint32_t downsample );
    std::vector< double > update ( void ); // returns averages of all data that is currently waiting in buffer
    void draw ( void );
    void stop ( void );
    
private:
    // OSC
    ofxOscSender sender;
    
    // Private member funcs
    bool processBitalinoOutput( BITalino::Frame& frame );
    
    // My own stuff
    uint32_t samplesToCollect;
    std::vector< double > currentTrainingOutput;

    // Processing the BITalino output
    double discreteVal = 0;
    double discreteValCalibrated = 0;
    double N = 1000;
    double M = 100;
    
    double testCheapRMSdiscrete = 0;
    double buttonState = 0;
    double previousAccelZ = 0.0;
    
    std::vector< double > bitalinoProcessedOutput;
    bool previousButtonPress = false;
    
    // rapidStream for Bayesian filter
    rapidmix::rapidStream rapidProcess;
    
    //debug
    uint32_t sampleValue = 0;
    std::atomic< uint32_t > downsample;
    double previousValue = 0;
};


#endif /* RapidBitalino_h */
