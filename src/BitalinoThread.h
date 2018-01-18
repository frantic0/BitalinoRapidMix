//
//  BitalinoThread.h
//  Bitalino
//
//  Created by James on 19/11/2017.
//
//

#ifndef BitalinoThread_h
#define BitalinoThread_h

#include <stdio.h>
#include "ThreadedProcess.h"
#include "RingBuffer.hpp"
#include "bitalino.h"

class BitalinoThread : public ThreadedProcess {
public:
    RingBuffer< BITalino::Frame > data; // The data gathered in the main thread, safely accessable from outside
    
    BitalinoThread ( void ) : ThreadedProcess (  )
    {
        connected.store( false );
        recording.store( false );
        tries.store( 0 );
        // NOTE THAT DEVICE NAME CAN BE SET IN SETUP
        this->deviceName = "/dev/cu.BITalino-DevB"; // Debug init ( Mac OS virt. serial over bt )
    }
    
    ~BitalinoThread ( void )
    {
    }
    
    void setup ( std::string deviceName, uint64_t bufferSize,
                uint16_t sampleRate, std::vector<int32_t> channels,
                uint16_t frameCount=100 )
    {
        this->deviceName = deviceName;
        this->sampleRate = sampleRate;
        this->channels = channels;
        frameBuffer.resize( frameCount );
        pollTestBuffer.resize( 1 );
        data.setup( bufferSize );
        startThread( 500 ); // sleep for 0.5 ms after each callback
    }
    
    void setRecording ( bool v )
    { // Allow data to be pushed in to the ringbuffer
        recording.store( v );
    }
    
    bool isRecording ( void ) const
    {
        return recording.load(  );
    }

    bool isConnected ( void ) const
    {
        return connected.load(  );
    }
    
    uint32_t getConnectionTries( void ) const
    {
        return tries.load(  );
    }
    
protected:
    void mainThreadCallback ( void ) override
    {
        try {
            switch ( threadState )
            {
                case SEARCHING_FOR_DEVICE:
                    ++currentTries;
                    tries.store( currentTries );
                    if ( !dev )
                        dev = new BITalino( deviceName.c_str(  ) );
                    dev->start( sampleRate, channels );
                    threadState = IDLE_BUT_CONNECTED;
                    connected.store( true );
                    currentTries = 0;
                    break;
                case IDLE_BUT_CONNECTED:
                    if ( recording.load(  ) )
                    {
                        threadState = RECORDING_DATA;
                    } else {
                        dev->read( pollTestBuffer ); // Poll small data to check if alive
                        usleep( 100000 ); // Wait 100 ms
                    }
                    break;
                case RECORDING_DATA:
                    if ( !recording.load(  ) )
                    {
                        threadState = IDLE_BUT_CONNECTED;
                    } else {
                        dev->read( frameBuffer );
                        if ( data.items_available_for_write(  ) >= frameBuffer.size(  ) )
                            data.push( &frameBuffer[ 0 ], frameBuffer.size(  ) );
                        // Else skipped frame... notify?
                    }
                    break;
                default:
                    break;
            }
        } catch ( BITalino::Exception &e ) {
            // printf( "BITalino exception: %s\n", e.getDescription(  ) );
            // TODO: check which exact exception is communication lost etc.
            threadState = SEARCHING_FOR_DEVICE;
            connected.store( false );
            if ( dev )
            {
                delete dev;
                dev = nullptr;
            }
            usleep( 500000 ); // 500ms Timeout before trying to reconnect again
        }
    }
    
    std::atomic< bool > connected;
    std::atomic< bool > recording;
    std::string deviceName;
    
    BITalino* dev = nullptr;
    uint16_t sampleRate = 1000;
    std::vector< int32_t > channels;
    
    std::atomic< uint32_t > tries;
    uint32_t currentTries = 0;
    
private:
    enum State {
        SEARCHING_FOR_DEVICE,
        IDLE_BUT_CONNECTED,
        RECORDING_DATA
    };
    
    State threadState = SEARCHING_FOR_DEVICE;
    
    BITalino::VFrame frameBuffer;
    BITalino::VFrame pollTestBuffer;
};



#endif /* BitalinoThread_h */
