//
//  PokeGraph.cpp
//  Bitalino
//
//  Created by James on 21/12/2017.
//
//

#include "PokeGraph.hpp"

void PokeGraph::setup ( std::string label, ofColor textColor, ofColor graphColor, ofColor backgroundColor,
                       ofRectangle posRect, bool fillGraph, bool dontDrawZero, uint32_t memorySize )
{
    this->label = label;
    this->textColor = textColor;
    this->graphColor = graphColor;
    this->backgroundColor = backgroundColor;
    this->posRect = posRect;
    graphRect = posRect;
    graphRect.scaleFromCenter( 0.99f, 0.75f );
    
    this->fillGraph = fillGraph;
    this->dontDrawZero = dontDrawZero;
    
    memory.setup( memorySize );
    calculateScaling(  );
    
    renderToMemory(  );
}

void PokeGraph::setResolution ( uint32_t memorySize )
{
    memory.resize( memorySize );
    calculateScaling(  );
    renderToMemory(  );
}

uint32_t PokeGraph::getResolution ( void ) const
{
    return memory.size(  );
}

void PokeGraph::pokeValue ( double position, double value )
{
    memory[ position * memory.size(  ) ] = value;
    renderToMemory(  );
}

bool PokeGraph::isMouseInside ( ofVec2f mousePos)
{
    return graphRect.inside( mousePos );
}

void PokeGraph::mouseActive ( ofVec2f mousePos, int8_t mouseState, bool mouseStateChanged )
{
    switch ( mouseState ) {
        case 0:
            // Left button pressed
            if ( mouseStateChanged )
            { // Pressed
                initialClickpos = mousePos;
            } else {
                // Held
                memory.setLine( ( initialClickpos.x - graphRect.x ) * screenToMemoryScaling.x,                // startIndex
                               maxVal - ( initialClickpos.y - graphRect.y ) * screenToMemoryScaling.y,           // startValue
                               ( mousePos.x - graphRect.x ) * screenToMemoryScaling.x,                        // StopIndex
                               maxVal - ( mousePos.y - graphRect.y ) * screenToMemoryScaling.y );                // StopValue
            }
            break;
            
        case 1:
            // Fallthrough to right mouse button
        
        case 2:
            // Right button pressed
            if ( mouseStateChanged )
            {
                previousPosition = mousePos;
            }
            
            if ( std::fabs( previousPosition.x - mousePos.x ) > memoryToScreenScaling.x )
            { // Smooth out sudden changes
                memory.setLine( ( previousPosition.x - graphRect.x ) * screenToMemoryScaling.x,                // startIndex
                               maxVal - ( previousPosition.y - graphRect.y ) * screenToMemoryScaling.y,           // startValue
                               ( mousePos.x - graphRect.x ) * screenToMemoryScaling.x,                        // StopIndex
                               maxVal - ( mousePos.y - graphRect.y ) * screenToMemoryScaling.y );                // StopValue
            } else {
                memory[ ( mousePos.x - graphRect.x ) * screenToMemoryScaling.x ] =
                                    maxVal - ( mousePos.y - graphRect.y ) * screenToMemoryScaling.y;
            }
            previousPosition = mousePos;
            break;
            
        default:
            // Reset
            previousPosition = ofVec2f(  );
            initialClickpos = ofVec2f(  );
            // Mouse released
            break;
    }
}

void PokeGraph::update ( void )
{
    if ( memory.update(  ) )
    {
        for ( int i = 0; i < memory.size(  ); ++i )
        {
            double value = std::fabs( memory[ i ] );
            if ( value > maxVal )
                maxVal = value;
        }
        calculateScaling(  );
        renderToMemory(  );
        // Needs to update
        // Redraw
    }
}

void PokeGraph::draw ( void )
{
    rendered.draw( posRect.position );
}


void PokeGraph::resize ( ofVec2f wh )
{
    this->posRect.width = wh.x;
    this->posRect.height = wh.y;
    graphRect = posRect;
    graphRect.scaleFromCenter( 0.99f, 0.8f );
    calculateScaling(  );
    renderToMemory(  );
}

void PokeGraph::setRect ( ofRectangle posRect )
{
    this->posRect = posRect;
    graphRect = posRect;
    graphRect.scaleFromCenter( 0.99f, 0.8f );
    calculateScaling(  );
    renderToMemory(  );
}

void PokeGraph::redraw ( void )
{
    calculateScaling(  );
    renderToMemory(  );
}

void PokeGraph::calculateScaling ( void )
{
    memoryToScreenScaling = ofVec2f( graphRect.width / static_cast< float > ( memory.size(  ) - 1 ), graphRect.height / maxVal );
    screenToMemoryScaling = ofVec2f( static_cast< float > ( memory.size(  ) - 1 ) / graphRect.width, maxVal / graphRect.height );
}

void PokeGraph::renderToMemory ( void )
{
    ofRectangle relativePosRect = ofRectangle( 2, 0, posRect.width - 2, posRect.height - 2 );
    ofRectangle relativeGraphRect = ofRectangle( graphRect.x - posRect.x,
                                                graphRect.y - posRect.y,
                                                graphRect.width,
                                                graphRect.height );
    
    rendered.clear(  );
    ofFbo::Settings settings;
    settings.width = posRect.width;
    settings.height = posRect.height;
    settings.useDepth = false;
    settings.useStencil = false;
    rendered.allocate( settings );
    rendered.begin(  );
    
    ofClear( 255, 255, 255, 0 );
    ofSetLineWidth( 2 );
    
    //ofBackground(255, 100, 100, 255);
    
    // Background
    // Fill
    ofFill(  );
    ofSetColor( backgroundColor, 100 );
    ofDrawRectRounded( relativePosRect, 5 );
    
    // Label
    ofSetColor( textColor );
    ofDrawBitmapString( label, relativePosRect.x + 2, relativePosRect.y + 14 );
    
    
    float absWidth = relativeGraphRect.width ;
    // Graph
    if ( fillGraph )
    {
        ofBeginShape(  );
        // Fill
        ofFill(  );
        ofSetColor( graphColor, 100 );
        ofVertex( relativeGraphRect.x, ( relativeGraphRect.y + relativeGraphRect.height ) );
        for ( int32_t i = 0; i < memory.size(  ); ++i )
        {
            double value = memory[ i ];
            if ( !dontDrawZero || ( dontDrawZero && value != 0 ) )
                ofVertex( relativeGraphRect.x + i * memoryToScreenScaling.x,
                         ( relativeGraphRect.y + relativeGraphRect.height ) - value * memoryToScreenScaling.y );
            else
            {
                ofVertex( relativeGraphRect.x + i * memoryToScreenScaling.x,
                         ( relativeGraphRect.y + relativeGraphRect.height ) );
            }
        }
        ofVertex( relativeGraphRect.x + absWidth, ( relativeGraphRect.y + relativeGraphRect.height ) );
        
        ofEndShape(  );
    }
    ofBeginShape(  );
    // Outline
    ofNoFill(  );
    ofSetColor( graphColor );
    
    ofVertex( relativeGraphRect.x, ( relativeGraphRect.y + relativeGraphRect.height ) );
    for ( int32_t i = 0; i < memory.size(  ); ++i )
    {
        double value = std::fabs( memory[ i ] );
        if ( !dontDrawZero || ( dontDrawZero && value != 0 ) )
            ofVertex( relativeGraphRect.x + i * memoryToScreenScaling.x,
                     ( relativeGraphRect.y + relativeGraphRect.height ) - value * memoryToScreenScaling.y );
        else
        {
            ofEndShape(  );
            ofBeginShape(  );
        }
    }
    ofVertex( relativeGraphRect.x + absWidth, ( relativeGraphRect.y + relativeGraphRect.height ) );
    
    ofEndShape(  );
    ofSetColor( backgroundColor, 175 );
    ofSetLineWidth( 1.5f );
    ofDrawLine( relativeGraphRect.x, relativeGraphRect.y + relativeGraphRect.height,
               relativeGraphRect.x + relativeGraphRect.width, relativeGraphRect.y + relativeGraphRect.height );
    
    // Outline
    ofNoFill(  );
    ofSetColor( backgroundColor );
    ofDrawRectRounded( relativePosRect, 5 );
    
    rendered.end(  );
    
    ofSetColor( 255, 255, 255 );
}
