//
//  BITSequencerThread.hpp
//  Bitalino
//
//  Created by James on 08/12/2017.
//
//

#ifndef BITSequencerThread_hpp
#define BITSequencerThread_hpp

#include <stdio.h>
#include <functional>

#include "Timer.h"
#include "SigTools.hpp"
#include "RapidBitalino.h"
#include "RapidMixThread.h"
#include "BITSynth.hpp"
#include "ThreadedProcess.h"
#include "RingBufferAny.hpp"
#include "LinkedBuffer.hpp"

namespace BIT
{
class BITSequencerThread : public ThreadedProcess
{
// Functions as the model
public:
    LinkedBuffer< double > bitalinoChannel;
    LinkedBuffer< double > loopDataBuffers[ NUM_SYNTH_PARAMS ];
    
    
    // Todo move back to protected/private (public for testing w/o bitalino)
    void buttonPressed ( void );
    void buttonReleased ( void );

    // BITalino thread, TODO: threadsafe accessors etc
    RapidBitalino rapidBitalino;
    // Synth which can only be changed by ringbuffer (threadsafe)
    Synth synthesizer;
    
    // Threadsafe queue of events
    RingBufferAny toEnvironment;
    
    BITSequencerThread ( void );
    ~BITSequencerThread ( void );
    
    void setup ( std::string nameOfBitalino );
    void setState ( State state ); // Atomic set
    State getState ( void ) const; // Atomic get
    const char* getStateString ( void );
    
    void stop ( void );
    void audioOut(float * output, int bufferSize, int nChannels);
    double getTransport ( void );
    
    struct RealtimeViewData {
        double transportPosition = 0.0;
    };
    
protected:
    //void buttonPressed ( void );
    //void buttonReleased ( void );
    void mainThreadCallback ( void );
    void generateFirstData ( void );
    void generateFirstRandomData ( void );
    void sendRunDataToOFX ( void );
    
private:
    // Rapidmix stuff
    RapidMixThread rapidMixThread;
    rapidmix::trainingData trainingData;
    
    bool gatheringTrainingData = false;
    bool trainingDone = false;
    
    // Other stuff
    uint32_t sampleRate = 44100;
    uint32_t controlRate = 15; // 15ms
    uint32_t samplesPerExample = 50;
    
    // DEBUGDEBUG
    //--
    
    // State
    std::atomic< State > currentState;
    State previousState; // (synced with Data thread)
    
    std::atomic< bool > recording;
    std::atomic< bool > selectingData;
    
    // Data thread synced
    BIT::SynthControlData currentControlData;
    std::vector< BIT::SynthControlData > loop;
    int loopSize;
    int loopPosition;
    
    // Timers
    Timer dataPlayTimer;
    Timer longSelectionTimer;
    
    uint32_t longPress = 400; // 400ms
    double maxValueEMG = 512.0;
    
    std::atomic< double > transport;
};

}
#endif /* BITSequencerThread_hpp */
