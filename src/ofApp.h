#pragma once

#include "ofMain.h"
#include "ofxMaxim.h"
#include "RapidBitalino.h"
#include "BITEnvironment.hpp"

class ofApp : public ofBaseApp{

public:
    void setup ( void );
    void update ( void );
    void draw ( void );
    void audioOut ( float * output, int bufferSize, int nChannels );
    void exit ( void );
    
    void keyPressed ( int key );
    void keyReleased ( int key );
    
    void updateMouse ( int x, int y, int8_t mouseButtonState );
    
    void mouseMoved ( int x, int y );
    void mouseDragged ( int x, int y, int button );
    void mousePressed ( int x, int y, int button );
    void mouseReleased ( int x, int y, int button );
    void mouseEntered ( int x, int y );
    void mouseExited ( int x, int y );
    void windowResized ( int w, int h );
    void dragEvent ( ofDragInfo dragInfo );
    void gotMessage ( ofMessage msg );
    
		
private:
    int8_t mouseButtonState = -1;
    bool mouseButtonChanged = false;
    
    //DEBUG
    bool pr = false;
    //---
    
    BIT::Environment bitEnvironment;
    int		bufferSize; 
    int		sampleRate;
};
