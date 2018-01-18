//
//  PokeGraph.hpp
//  Bitalino
//
//  Created by James on 21/12/2017.
//
//

// A grapb which allows input and makes use of a threadsafe non-locking buffer

#ifndef PokeGraph_hpp
#define PokeGraph_hpp

#include <stdio.h>
#include "ofMain.h"
#include "LinkedBuffer.hpp"

class PokeGraph {
public:
    LinkedBuffer< double > memory; // Threadsafe non-(b)locking, linkable buffer
    
    void setup ( std::string label, ofColor textColor, ofColor graphColor, ofColor backgroundColor, ofRectangle posRect, bool fillGraph, bool dontDrawZero, uint32_t memorySize );
    
    void setResolution ( uint32_t memorySize );
    uint32_t getResolution ( void ) const;
    
    void pokeValue ( double position, double value );
    
    bool isMouseInside ( ofVec2f mousePos );
    void mouseActive ( ofVec2f mousePos, int8_t mouseState, bool mouseStateChanged );
    
    void update ( void );
    void draw ( void );
    
    void resize ( ofVec2f wh );
    void setRect ( ofRectangle posRect );
    
    bool fillGraph = false;
    bool dontDrawZero = false;
    ofColor textColor, graphColor, backgroundColor;
    
    void redraw ( void );
    
protected:
    void calculateScaling ( void );
    void renderToMemory ( void );
    
private:
    ofFbo rendered;
    
    ofVec2f memoryToScreenScaling;
    ofVec2f screenToMemoryScaling;
    
    ofVec2f initialClickpos;
    ofVec2f previousPosition;
    
    ofRectangle graphRect;
    ofRectangle posRect;
    
    std::string label;
    
    double maxVal = 1.0;
    
};

#endif /* PokeGraph_hpp */
