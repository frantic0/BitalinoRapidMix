//
//  Tools.h
//  Bitalino
//
//  Created by James on 22/11/2017.
//
//

#ifndef Tools_h
#define Tools_h

#include "ofMain.h"

namespace ofxBitmapText {

    enum class TextAlignment {
        LEFT,
        CENTER,
        RIGHT
    };

    static inline void drawTextLabel ( std::string label, ofVec2f position, ofColor labelBackgroundColor, ofColor stringColor, TextAlignment alignment, bool drawAbove )
    {
        uint32_t strLenPix = label.length(  )*8;
        
        switch ( alignment ) {
            case TextAlignment::LEFT:
                // No exception
                break;
            case TextAlignment::CENTER:
                position.x -= strLenPix / 2;
                break;
            case TextAlignment::RIGHT:
                position.x -= strLenPix;
                break;
            default:
                break;
        }
        
        ofFill(  );
        ofSetColor( labelBackgroundColor );
        ofRectangle bmpStringRect( position.x - 2,
                                  position.y + ( ( drawAbove ) ? -4 : 12 ) + 2,
                                  strLenPix + 4, -12 );
        ofDrawRectangle( bmpStringRect );
        ofSetColor( stringColor );
        ofDrawBitmapString( label, position.x, position.y + ( ( drawAbove ) ? -4 : 12 ) );
        ofSetColor( 255, 255, 255, 255 );
    }

}

#endif /* Tools_h */
