//
//  BITEnvironment.cpp
//  Bitalino
//
//  Created by James on 23/11/2017.
//
//

#include "BITEnvironment.hpp"

namespace BIT
{
void Environment::setup( std::string nameOfBitalino, ofRectangle posAndSize )
{
    this->posAndSize = posAndSize;
    float graphHeight = posAndSize.height / ( NUM_SYNTH_PARAMS + 1 );
    bst.setup( nameOfBitalino );
    
    ofRectangle bitGraphRect( posAndSize.x, posAndSize.y, posAndSize.width, graphHeight );
    bitalinoGraph.setup( "Bitalino", ofColor( 100, 100, 255 ),
                        ofColor( 100, 255, 100 ), ofColor( 100, 100, 100 ), bitGraphRect, true, true, 2000 );
    
    for ( uint32_t i = 0; i < NUM_SYNTH_PARAMS; ++i )
    {
        ofRectangle graphRect( posAndSize.x, posAndSize.y + ( i + 1 ) * graphHeight, posAndSize.width, graphHeight );
        graphs[ i ].setup( ParamNames[ i ], ofColor( 255, 255, 255 ),
                          ofColor( 255, 100, 100 ), ofColor( 100, 100, 100 ), graphRect, true, false, 2000 );
        
        // Link buffers together
        bst.loopDataBuffers[ i ].linkTo( &graphs[ i ].memory );
        graphs[ i ].memory.linkTo( &bst.loopDataBuffers[ i ] );
    }
    
    // Only needs one way link
    bst.bitalinoChannel.linkTo( &bitalinoGraph.memory );
}
    
void Environment::update ( void )
{ // This runs concurrent with ofx
    // THIS can dissapear
    RingBufferAny::VariableHeader headerInfo;
    while ( bst.toEnvironment.anyAvailableForPop( headerInfo ) )
    {
        if ( headerInfo.type_index == typeid( RecordingSelection ) ) {
            RecordingSelection sel;
            bst.toEnvironment.pop( &sel, 1 );

            if ( sel.position < 0 )
                selectedTrainingParts.clear(  );
            else
                selectedTrainingParts.push_back( sel.position );
        } else if ( headerInfo.type_index == typeid( State ) ) {
            selectedTrainingParts.clear(  );
            bst.toEnvironment.pop( &currentState, 1 );
            switch ( currentState )
            {
                case PERFORMING:
                    bitalinoGraph.graphColor = ofColor( 100, 255, 100 );
                    bitalinoGraph.redraw(  );
                    for ( uint32_t i = 0; i < NUM_SYNTH_PARAMS; ++i )
                    {
                        graphs[ i ].graphColor = ofColor( 100, 100, 255 );
                        graphs[ i ].backgroundColor = ofColor( 255, 255, 255 );
                        graphs[ i ].redraw(  );
                    }
                    break;
                case RECORDING:
                    bitalinoGraph.graphColor = ofColor( 100, 255, 100 );
                    bitalinoGraph.redraw(  );
                    for ( uint32_t i = 0; i < NUM_SYNTH_PARAMS; ++i )
                    {
                        graphs[ i ].graphColor = ofColor( 255, 255, 255 );
                        graphs[ i ].backgroundColor = ofColor( 180, 100, 100 );
                        graphs[ i ].redraw(  );
                    }
                    break;
                case PLAYING:
                    bitalinoGraph.graphColor = ofColor( 100, 255, 100 );
                    bitalinoGraph.redraw(  );
                    for ( uint32_t i = 0; i < NUM_SYNTH_PARAMS; ++i )
                    {
                        graphs[ i ].graphColor = ofColor( 255, 100, 100 );
                        graphs[ i ].backgroundColor = ofColor( 100, 100, 100 );
                        graphs[ i ].redraw(  );
                    }
                    break;
                case TRAINING:
                    // Fallthrough style
                case IDLE:
                    bitalinoGraph.graphColor = ofColor( 100, 100, 100 );
                    bitalinoGraph.redraw(  );
                    for ( uint32_t i = 0; i < NUM_SYNTH_PARAMS; ++i )
                    {
                        graphs[ i ].graphColor = ofColor( 100, 100, 100 );
                        graphs[ i ].backgroundColor = ofColor( 100, 100, 100 );
                        graphs[ i ].redraw(  );
                    }
                    break;
                default:
                    break;
            }
            
        } else if ( headerInfo.type_index == typeid( BITSequencerThread::RealtimeViewData ) ) {
            BITSequencerThread::RealtimeViewData data[ headerInfo.valuesPassed ];
            bst.toEnvironment.pop( data, headerInfo.valuesPassed );
            for ( size_t i = 0; i < headerInfo.valuesPassed; ++i )
            {
                transport = data[ i ].transportPosition;
            }
        }
    }
    
    bitalinoGraph.update(  );
    
    for ( uint32_t i = 0; i < NUM_SYNTH_PARAMS; ++i )
    {
        graphs[ i ].update(  );
    }

}

bool Environment::isMouseInside ( ofVec2f mousePos)
{
    return posAndSize.inside( mousePos );
}

void Environment::mouseActive ( ofVec2f mousePos, int8_t mouseState, bool mouseStateChanged )
{
    if ( currentState == PLAYING && selectedTrainingParts.empty(  ) ) // Only then are buffers to be altered
    for ( uint32_t i = 0; i < NUM_SYNTH_PARAMS; ++i )
    {
        PokeGraph& currentGraph = graphs[ i ];
        if ( currentGraph.isMouseInside( mousePos ) )
        {
            if ( previousActive && previousActive != &currentGraph )
            {
                previousActive->mouseActive( mousePos, -1, true );
            }
            currentGraph.mouseActive( mousePos, mouseState, mouseStateChanged );
            previousActive = &currentGraph;
        }
    }
}

void Environment::drawSelectedTrainingParts( void )
{
    ofColor highlight( 255, 255, 255, 50 );
    ofColor pillars( 255, 255, 255, 150 );
    size_t size = selectedTrainingParts.size(  );
    
    int8_t complete = 0;
    
    double previousPoint = 0;
    
    for ( uint32_t i = 0; i < size; ++i )
    { // TODO fix wrap cases
        double point = selectedTrainingParts[ i ];
        complete = i % 2;
        
        if ( i + 1 == size && complete == 0)
        {
            double transp = ( transport - point );
            if ( transp < 0 )
            {
                selectedTrainingParts.insert( selectedTrainingParts.begin(  ) + ( i + 1 ),  1.0 );
                selectedTrainingParts.insert( selectedTrainingParts.begin(  ) + ( i + 2 ),  0.0 );
                i -= 1;
                size += 2;
                continue;
            }
            ofRectangle rect( posAndSize.x + point * posAndSize.width, posAndSize.y,
                            transp * posAndSize.width, posAndSize.height );
            ofSetColor( highlight );
            ofFill(  );
            ofDrawRectangle( rect );
            ofSetColor( pillars );
            ofSetLineWidth( 1.5f );
            ofNoFill(  );
            ofDrawRectangle( rect );
            ofSetColor( 255, 255, 255, 255 );
            return;
        }
        if ( complete == 1 )
        {
            ofRectangle rect( posAndSize.x + previousPoint * posAndSize.width, posAndSize.y,
                                ( point - previousPoint ) * posAndSize.width, posAndSize.height );
            ofSetColor( highlight );
            ofFill(  );
            ofDrawRectangle( rect );
            ofSetColor( pillars );
            ofSetLineWidth( 1.5f );
            ofNoFill(  );
            ofDrawRectangle( rect );
        }
        previousPoint = point;
    }
    ofSetColor( 255, 255, 255, 255 );
}
    
void Environment::draw ( void )
{ // This runs concurrent with ofx

    double xPosTransport = posAndSize.x + posAndSize.width * transport;
    ofDrawLine( xPosTransport, posAndSize.y, xPosTransport, posAndSize.y + posAndSize.height );
    
    bitalinoGraph.draw(  );
    
    if ( !selectedTrainingParts.empty(  ) )
        drawSelectedTrainingParts(  );
    
    for ( uint32_t i = 0; i < NUM_SYNTH_PARAMS; ++i )
    {
        graphs[ i ].draw(  );
    }
    
    bool connected = bst.rapidBitalino.bitalinoThread.isConnected(  );
    
    drawTextLabel( stateStrings[ static_cast< uint8_t >( currentState ) ], ofVec2f( posAndSize.x + 4, posAndSize.y + 21 ),
                  (( connected ) ? ofColor( 24, 219, 92 ) : ofColor( 255, 24, 24 ) ), ofColor( 10, 10, 10 ),
                  ofxBitmapText::TextAlignment::LEFT, false );
}

void Environment::resize ( ofRectangle posAndSize )
{
    this->posAndSize = posAndSize;
    
    float graphHeight = posAndSize.height / ( NUM_SYNTH_PARAMS + 1 );
    
    bitalinoGraph.setRect( ofRectangle( posAndSize.x, posAndSize.y, posAndSize.width, graphHeight ) );
    for ( uint32_t i = 0; i < NUM_SYNTH_PARAMS; ++i )
    {
        graphs[ i ].setRect( ofRectangle( posAndSize.x, posAndSize.y + ( i + 1 ) * graphHeight, posAndSize.width, graphHeight ) );
    }
}

void Environment::stop( void )
{
    bst.stop();
}

void Environment::audioOut( float *output, int bufferSize, int nChannels )
{ // Audio thread sync callback
    bst.audioOut(output, bufferSize, nChannels);
}
    
} // End namespace BIT::
