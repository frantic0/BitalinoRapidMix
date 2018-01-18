#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup ( void )
{
    ofSetWindowTitle( "Rapid Bitalino Example" );
    
    sampleRate 	= 44100; /* Sampling Rate */
    bufferSize	= 512; /* Buffer Size. you have to fill this buffer with sound using the for loop in the audioOut method */
    
    ofSetVerticalSync( true );
    ofEnableAlphaBlending(  );
    ofEnableSmoothing(  );
    
    ofBackground( 0, 0, 0 );
    
    // Enter the name of your BITalino board here
    bitEnvironment.setup( "/dev/cu.BITalino-DevB", ofRectangle( 0, 0, 1024, 768 ) );
    
    ofxMaxiSettings::setup( sampleRate, 2, bufferSize );
    
    // sets up and starts a global ofSoundStream.
    ofSoundStreamSetup( 2, 0, sampleRate, bufferSize, 4 );
}

//--------------------------------------------------------------
void ofApp::update ( void )
{
    bitEnvironment.update(  );
}

//--------------------------------------------------------------
void ofApp::draw ( void )
{
    ofBackground( 0, 0, 0 );
    bitEnvironment.draw(  );
}

//--------------------------------------------------------------
void ofApp::audioOut ( float *output, int bufferSize, int nChannels )
{
    bitEnvironment.audioOut ( output, bufferSize, nChannels );
}

//--------------------------------------------------------------
void ofApp::exit ( void )
{
    printf( "stopping...\n" );
//    rapidBitalino.stop(  );
    bitEnvironment.stop(  );
}

//--------------------------------------------------------------
void ofApp::keyPressed ( int key )
{
    if ( key == '1' && !pr ) {
        bitEnvironment.bst.buttonPressed(  );
        printf( "1 down\n" );
        pr = true;
    }
}

//--------------------------------------------------------------
void ofApp::keyReleased ( int key )
{
    if ( key == '1' ) {
        bitEnvironment.bst.buttonReleased(  );
        printf( "1 up\n" );
        pr = false;
    }
}

void ofApp::updateMouse ( int x, int y, int8_t mouseButtonState )
{
    // Only really one mouse function necessary for this example:
    if ( mouseButtonState != this->mouseButtonState )
    {
        mouseButtonChanged = true;
        this->mouseButtonState = mouseButtonState;
    } else
        mouseButtonChanged = false;
    
    ofVec2f mousePos( x, y );
    if ( bitEnvironment.isMouseInside( mousePos ) )
    {
        bitEnvironment.mouseActive( mousePos, mouseButtonState, mouseButtonChanged );
    }
}

//--------------------------------------------------------------
void ofApp::mouseMoved ( int x, int y )
{
    updateMouse( x, y, mouseButtonState );
}

//--------------------------------------------------------------
void ofApp::mouseDragged ( int x, int y, int button )
{
    updateMouse( x, y, button );
}

//--------------------------------------------------------------
void ofApp::mousePressed ( int x, int y, int button )
{
    //std::cout << "BUTTON: " <<  button << std::endl;
    updateMouse( x, y, button );
}

//--------------------------------------------------------------
void ofApp::mouseReleased ( int x, int y, int button )
{
    updateMouse( x, y, -1 );
}

//--------------------------------------------------------------
void ofApp::mouseEntered( int x, int y )
{
    updateMouse( x, y, mouseButtonState );
}

//--------------------------------------------------------------
void ofApp::mouseExited ( int x, int y )
{
    updateMouse( x, y, mouseButtonState );
}

//--------------------------------------------------------------
void ofApp::windowResized ( int w, int h )
{
    bitEnvironment.resize( ofRectangle( 0, 0, w, h ) );
}

//--------------------------------------------------------------
void ofApp::gotMessage ( ofMessage msg )
{

}

//--------------------------------------------------------------
void ofApp::dragEvent ( ofDragInfo dragInfo )
{

}
