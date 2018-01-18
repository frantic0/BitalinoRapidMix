//
//  SigTools.cpp
//  
//
//  Created by James on 08/01/17.
//
//

#include "SigTools.hpp"

double MTOF ( unsigned char midi ) {
    return pow( 2, static_cast< double >( midi-69 ) / 12.0 ) * 440.0;
}

double FTOM ( double freq ) {
    return 12.0 * log2( freq / 440.0 ) + 69;
}
