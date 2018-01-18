//
//  RapidBitalino.cpp
//  Bitalino
//
//  Created by James on 20/11/2017.
//
//

#include "RapidBitalino.h"

RapidBitalino::RapidBitalino ( void )
{
}

RapidBitalino::~RapidBitalino ( void )
{
}

void RapidBitalino::setup ( std::string nameOfBitalino, uint32_t sampleEveryN )
{
    setDownsample( downsample );
    
    // Set up the bitalino
    bitalinoThread.setup( nameOfBitalino, 2000,
             1000, {0, 1, 2, 3, 4, 5},
             100 );
    // Start the bitalino's thread ( and allow it to start searching for the device )
    // Start receiving data as soon as it's connected
    bitalinoThread.setRecording( true );
    // Initialize with 0, 0
    bitalinoProcessedOutput = std::vector< double >( NUM_INPUTS_FOR_TRAINING, 0.0 );
    // Setup OSC
    sender.setup( HOST, PORT );
}

void RapidBitalino::setDownsample ( uint32_t downsample )
{
    this->downsample.store( downsample );
}

bool RapidBitalino::processBitalinoOutput( BITalino::Frame& frame )
{
    std::vector< double > output;
    discreteValCalibrated = static_cast< float >( frame.analog [ 0 ] - 512.0 );
    
    testCheapRMSdiscrete -= testCheapRMSdiscrete / M;
    testCheapRMSdiscrete += ( discreteValCalibrated * discreteValCalibrated ) / M;
    
    if ( sampleValue >= downsample.load(  ) )
    {
        double rms = sqrt( testCheapRMSdiscrete );
        double deltaRMS = rms - previousValue;
        double bayesianFiltered = rapidProcess.bayesFilter( discreteValCalibrated / 1000.0 );
        buttonState = static_cast< double >( frame.digital [ 0 ] );
        double accelZ = static_cast< double > ( frame.analog [ 4 ] );
        double deltaAccelZ = accelZ - previousAccelZ;
        
        bitalinoProcessedOutput = std::vector< double >( NUM_INPUTS_FOR_TRAINING, rms ); // Hack for more hidden nodes atm.
        
        // UNCOMMENT TO GET DEBUG OSC DATA
        // OSC MESSAGE ]
        /*
        ofxOscMessage d;
        d.setAddress( "/BitalinoRawDigital" );
        d.addIntArg( frame.digital [ 0 ] );
        d.addIntArg( frame.digital [ 1 ] );
        d.addIntArg( frame.digital [ 2 ] );
        d.addIntArg( frame.digital [ 3 ] );
        sender.sendMessage( d );
        ofxOscMessage a;
        a.setAddress( "/BitalinoRawAnalog" );
        a.addIntArg( frame.analog [ 0 ] );
        a.addIntArg( frame.analog [ 1 ] );
        a.addIntArg( frame.analog [ 2 ] );
        a.addIntArg( frame.analog [ 3 ] );
        a.addIntArg( frame.analog [ 4 ] );
        a.addIntArg( frame.analog [ 5 ] );
        
        sender.sendMessage( a );
        
        ofxOscMessage b;
        b.setAddress( "/BayesianFilter" );
        b.addDoubleArg( bayesianFiltered );
        sender.sendMessage( b );
        ofxOscMessage m;
        m.setAddress( "/RMS" );
        m.addDoubleArg( rms );
        sender.sendMessage( m );
        ofxOscMessage dt;
        dt.setAddress( "/DeltaRMS" );
        dt.addDoubleArg( deltaRMS );
        sender.sendMessage( dt );
        ofxOscMessage at;
        at.setAddress( "/DeltaACCELZ" );
        at.addDoubleArg( deltaAccelZ );
        sender.sendMessage( at );
        ofxOscMessage n;
        n.setAddress( "/ProcessedRaw" );
        n.addDoubleArg( discreteValCalibrated );
        sender.sendMessage( n );
         */
        // --
        
        
        sampleValue = 0;
        previousValue = rms;
        previousAccelZ = accelZ;
        
        return true;
    }
    ++sampleValue;
    return false;
}

std::vector< double > RapidBitalino::update ( void )
{
    std::vector< double > output = std::vector<double>( NUM_INPUTS_FOR_TRAINING, 0.0 );
    double numItemsToAverage = 0.0;
    bool buttonPress = previousButtonPress;
    
    uint32_t items = bitalinoThread.data.items_available_for_read(  );
    
    if ( items )
    {
        buttonPress = false;
        BITalino::Frame data [ items ];
        bitalinoThread.data.pop( data, items );
        
        for ( uint32_t i = 0; i < items; ++i )
        {
            if ( processBitalinoOutput( data [ i ] ) )
            { // Downsample scope
                if ( !data [ i ].digital [ 0 ] )
                    buttonPress = true;
                
                assert( output.size(  ) == bitalinoProcessedOutput.size(  ) );
                // Sum the data
                std::transform( output.begin(  ), output.end(  ),
                               bitalinoProcessedOutput.begin(  ), output.begin(  ), std::plus<double>(  ) );
                ++numItemsToAverage;
            }
        }
        // calculate the average
        if ( numItemsToAverage > 1.0 )
            std::transform( output.begin(  ), output.end(  ), output.begin(  ),
                       std::bind2nd( std::divides<double>(  ), numItemsToAverage ) );

    } else {
        output = bitalinoProcessedOutput;
    }
    
    if ( buttonPressedCallback && buttonPress && !previousButtonPress )
        buttonPressedCallback(  );
    
    else if ( buttonReleasedCallback && !buttonPress && previousButtonPress )
        buttonReleasedCallback(  );
    
    previousButtonPress = buttonPress;
    return output;
}

void RapidBitalino::draw ( void )
{
    // Todo, draw some trivial debug interface items
    // ie:
    /*
     - BITalino isConnected (  )
     - BITalino isRecording (  ) // if receiving data ?
     - RapidMixThread isTraining (  )
     - input and output values
     */
    //ofDrawBox( ofVec2f( 400,( tOutput/4 )*600 ), 50 );
    //ofDrawSphere( 512, 384, ( tOutput/4 )*600 );
    drawTextLabel( ( bitalinoThread.isConnected(  ) ) ? "Bitalino connected" : "Looking for BITalino... " + ofToString( bitalinoThread.getConnectionTries(  ) ) + " tries", ofVec2f( 0,0 ), ofColor( 0,0,0 ), ofColor( 255,255,255 ), ofxBitmapText::TextAlignment::LEFT, false );
}

void RapidBitalino::stop ( void )
{
    bitalinoThread.stopThread(  );
}


