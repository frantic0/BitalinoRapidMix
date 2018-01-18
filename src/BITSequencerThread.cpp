//
//  BITSequencerThread.cpp
//  Bitalino
//
//  Created by James on 08/12/2017.
//
//

#include "BITSequencerThread.hpp"

namespace BIT
{
    
BITSequencerThread::BITSequencerThread ( void ) : ThreadedProcess ()
{
    setState(IDLE);
    recording.store(false);
    selectingData.store(false);
    transport.store(0.0);
}

BITSequencerThread::~BITSequencerThread ( void )
{
    
}

void BITSequencerThread::setup( std::string nameOfBitalino )
{
    
    // Set callback for when model is done training (called in rapidMixThread.update method)
    rapidMixThread.doneTrainingCallback = [&] ( void )
    {
        //printf("New Model Trained\n");
        sendRunDataToOFX();
        trainingDone = true;
        transport.store(0.0);
        setState( PERFORMING );
        //trainingText = "New Model Trained";
    };
    
    // Set buttonPressedCallback for rapidBitalino
    rapidBitalino.buttonPressedCallback = std::bind(&BITSequencerThread::buttonPressed, this);
    
    // Set buttonReasedCallback for rapidBitalino
    rapidBitalino.buttonReleasedCallback = std::bind(&BITSequencerThread::buttonReleased, this);
    
    // Set up training thread
    rapidMixThread.setup();
    
    // Set up BITalino accessor + thread
    rapidBitalino.setup( nameOfBitalino, 10 ); // downsampling 10 of the data (TODO experiment with different settings with bayes)
    
    toEnvironment.setup( 8192 ); // Leave room for 16kb of memory to be used in the ring-queue
    
    bitalinoChannel.setup( 2000 );
    
    for ( uint32_t i = 0; i < NUM_SYNTH_PARAMS; ++i )
    {
        loopDataBuffers[ i ].setup( 2000 );
    }
    
    State s = currentState.load(  );
    toEnvironment.push( &s, 1 ); // Send initial state
    
    // Start the thread
    startThread( 10000 ); // Sleep 10 ms
}

void BITSequencerThread::setState ( State state )
{
    toEnvironment.push ( &state, 1 );
    this->currentState.store(state);
}

State BITSequencerThread::getState ( void ) const
{
    return this->currentState.load();
}
    
const char* BITSequencerThread::getStateString ( void )
{
    return stateStrings[static_cast<uint8_t>(getState())];
}
    
void BITSequencerThread::stop( void )
{
    // Stop all asynchronous threads
    this->stopThread();
    rapidMixThread.stopThread();
    rapidBitalino.stop();
}

void BITSequencerThread::audioOut( float *output, int bufferSize, int nChannels )
{ // Audio thread sync callback
    synthesizer.audioOut(output, bufferSize, nChannels);
}

double BITSequencerThread::getTransport ( void )
{
    return transport.load(  );
}
    
void BITSequencerThread::buttonPressed ( void )
{ // Bitalino button press
    longSelectionTimer.reset(  );
    switch ( currentState.load( ) )
    { // Run update for state
        case IDLE:
            //printf( "Started \n" );
            generateFirstData(  );
            setState( PLAYING );
            break;
        case PERFORMING:
        {
            if ( trainingDone )
            {
                if ( recording.load(  ) )
                {
                    //printf( "Playing recorded gestures \n" );
                    recording.store( false );
                    setState( PLAYING );
                } else {
                    //printf( "Started recording \n" );
                    recording.store( true );
                    transport.store( 0.0 );
                    
                    // TODO: create extra state for just recording?
                    State recState = RECORDING; // Only used by environment for color change
                    toEnvironment.push( &recState, 1 );
                }
            }
            break;
        }
        case PLAYING:
        {
            if ( !selectingData.load(  ) )
            {
                RecordingSelection sel{ static_cast< double >( loopPosition ) / loopSize };
                toEnvironment.push( &sel, 1 );
                
                //printf( "Selecting data for example \n" );
                trainingData.createNewPhrase( "NewPhrase" );
                //printf( "Added training element, now size of %lu\n", trainingData.trainingSet.size(  ) );
                selectingData.store( true );
            }
            break;
        }
        default:
            break;
    }
}

void BITSequencerThread::buttonReleased( void )
{ // Bitalino button release
    State s = currentState.load();
    switch ( s )
    { // Run update for state
        case PLAYING:
        {
            if (selectingData.load())
            {
                if (longSelectionTimer.getTimeElapsed() > longPress)
                {
                    RecordingSelection sel{ static_cast< double >( loopPosition ) / loopSize };
                    toEnvironment.push( &sel, 1 );
                    //printf("Recorded example \n");
                    selectingData.store(false);
                } else {
                    //printf("Throwing away last training sample, press was for stopping \n");
                    trainingData.trainingSet.pop_back();
                    RecordingSelection sel{ -1.0 };
                    toEnvironment.push( &sel, 1 );
                    
                    bitalinoChannel.setRangeToConstant( 0, loopSize, 0.0 );
                    // Draw line here for next part ?
                    
                    if (trainingData.trainingSet.size() >= 1)
                    {
                        //printf("Training on selected data \n");
                        RecordingSelection sel{ -1.0 };
                        toEnvironment.push( &sel, 1 );
                        trainingDone = false;
                        rapidMixThread.train(trainingData);
                        setState( TRAINING );
                    } else {
                        //printf("Reinitializing");
                        generateFirstData();
                        toEnvironment.push(&s, 1); // Make sure everything gets reinitialized
                    }
                    selectingData.store(false);
                }
            }
            break;
        }
        default:
            break;
    }
    
}

void BITSequencerThread::mainThreadCallback( void )
{ // Process thread sync callback
    // Get input data (BITalino):
    RealtimeViewData viewData;
    std::vector< double > inputData = rapidBitalino.update(  ); // Update method uses ringbuffer and is threadsafe
    State curState = currentState.load(  );
    bool newState = ( curState != previousState );
    
    if ( newState )
    {
        dataPlayTimer.reset(  );
        longSelectionTimer.reset(  );
        switch ( curState )
        { // Run inits for states
            case PERFORMING:
                loop.clear(  );
                loopSize = 0;
                break;
            case PLAYING:
                loopPosition = 0;
                trainingData.trainingSet.clear(  );
                break;
            default:
                break;
        }
    } else
        switch ( curState )
    { // Run update for state
        case PERFORMING:
        { // Update case for performing state
            if (dataPlayTimer.getTimeElapsed(  ) > controlRate && trainingDone)
            {
                if ( !recording.load(  ) )
                    transport.store( inputData[0] / maxValueEMG );
                
                SynthControlData ctrlData( rapidMixThread.model->run( inputData ) );
                synthesizer.controlDataBuffer.push( &ctrlData, 1 );
                currentControlData = ctrlData;
                if ( recording.load(  ) )
                {
                    bitalinoChannel.resizeBounded( loopSize + 1 );
                    bitalinoChannel[ loopSize ] = inputData[ 0 ];
                    
                    for ( uint32_t p = 0; p < NUM_SYNTH_PARAMS; ++p )
                    {
                        loopDataBuffers[ p ].resizeBounded( loopSize + 1 );
                        loopDataBuffers[ p ][ loopSize ] = ctrlData.values[ p ];
                    }
                    ++loopSize;
                }
                dataPlayTimer.reset(  );
            }
            break;
        }
        case PLAYING:
        { // Update case for playing state
            if (dataPlayTimer.getTimeElapsed() > controlRate)
            {
                bitalinoChannel[ loopPosition ] = inputData[ 0 ];
                SynthControlData ctrlData;
                for ( uint32_t p = 0; p < NUM_SYNTH_PARAMS; ++p )
                {
                    ctrlData.values[ p ] = loopDataBuffers[ p ][ loopPosition ];
                }
                synthesizer.controlDataBuffer.push(&ctrlData, 1);
                currentControlData = ctrlData;
                if (++loopPosition >= loopSize)
                    loopPosition = 0;
                
                transport.store( static_cast< double >( loopPosition ) / loopSize );
                
                if (selectingData.load())
                { // Add element to trainingData
                    std::vector<double> outputTraining = ctrlData.getValuesAsVector(  );
                    trainingData.addElement(inputData, outputTraining);
                }
                dataPlayTimer.reset();
            }
            break;
        }
        default:
            break;
    }
    if ( currentState.load(  ) != IDLE )
    {
        viewData.transportPosition = transport.load(  );
        toEnvironment.push( &viewData, 1 );
    }
    
    bitalinoChannel.update(  );
    
    for ( uint32_t i = 0; i < NUM_SYNTH_PARAMS; ++i )
    {
        loopDataBuffers[ i ].update( );
    }
    
    rapidMixThread.update();
    previousState = curState;
}

void BITSequencerThread::generateFirstData ( void )
{
    uint8_t midiStart = 55;
    uint32_t doremiSize = 2;
    
    // Calculate new size of buffers
    uint32_t resizeSize = doremiSize * majorScaleSize * samplesPerExample;
    
    // Resize all buffers
    bitalinoChannel.resizeBounded( resizeSize );
    for ( uint32_t i = 0; i < NUM_SYNTH_PARAMS; ++i )
    {
        loopDataBuffers[ i ].resizeBounded( resizeSize );
    }
    
    // Calculate doremi ladder and also random data blocks for other parameters
    int index = 0;
    for (uint32_t i = 0; i < doremiSize; ++i)
    {
        for (uint32_t step = 0; step < majorScaleSize; ++step)
        {
            double frequency = MTOF( midiStart + ( i * 12 ) + majorScale[ step ] );
            SynthControlData ctrlData;
            ctrlData.values[ CARRIER ] = frequency;
            
            for ( uint16_t x = 0; x < NUM_SYNTH_PARAMS; ++x )
            {
                if ( x != static_cast< uint8_t >( CARRIER ) )
                    ctrlData.values[ x ] = ( ( double ) rand(  ) / RAND_MAX );
            }
            
            for ( uint32_t p = 0; p < NUM_SYNTH_PARAMS; ++p )
            {
                loopDataBuffers[ p ].setRangeToConstant( index, index + samplesPerExample, ctrlData.values[ p ] );
            }
            index += samplesPerExample;
        }
    }
    bitalinoChannel.setRangeToConstant( 0, resizeSize, 0 );
    loopPosition = 0;
    loopSize = resizeSize;
}

void BITSequencerThread::generateFirstRandomData ( void )
{ // NO LONGER USED
    for (uint32_t i = 0; i < 10; ++i)
    {
        SynthControlData ctrlData;
        for (uint16_t x = 0; x < NUM_SYNTH_PARAMS; ++x)
        {
            ctrlData.values[x] = ((double)rand()/RAND_MAX);
        }
        for (uint32_t y = 0; y < samplesPerExample; ++y)
            loop.push_back(ctrlData);
    }
}

void BITSequencerThread::sendRunDataToOFX ( void )
{   // Sweeps through the trained model and shows the input vs the outputs
    uint32_t numValues = 256;
    
    bitalinoChannel.resizeBounded( numValues );
    for ( uint32_t i = 0; i < NUM_SYNTH_PARAMS; ++i )
    {
        loopDataBuffers[ i ].resizeBounded( numValues );
    }
    
    for ( int i = 0.0; i < numValues; i += 1 )
    {
        double inp = static_cast< double >( i * 2 );
        std::vector< double > input( NUM_INPUTS_FOR_TRAINING, inp );
        bitalinoChannel[ i ] = inp;
        std::vector< double > outputs = rapidMixThread.model->run( input );
        for ( uint32_t p = 0; p < NUM_SYNTH_PARAMS; ++p )
        {
            loopDataBuffers[ p ][ i ] = outputs[ p ];
        }
    }
}
    
}
