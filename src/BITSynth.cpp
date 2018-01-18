//
//  BITSynth.cpp
//  Bitalino
//
//  Created by James on 07/12/2017.
//
//

#include "BITSynth.hpp"

namespace BIT {
    // Simple fm synth
    Synth::Synth ( void )
    {
        // Set up the ringbuffer for controldata which is pushed in to the audio thread
        controlDataBuffer.setup( 100 ); // Leave room for 10 control data to sync over to audio thread
    }
    
    void Synth::audioOut ( float *output, int bufferSize, int nChannels )
    { // Audio thread sync callback
        for ( uint32_t i = 0; i < bufferSize; ++i )
        {
            if ( controlDataBuffer.items_available_for_read(  ) )
                controlDataBuffer.pop( &audioControlData, 1 );
            
            SVF.setCutoff( dFilt1.lopass( 110 + fmin( fabs( audioControlData[ CUTOFF ] ), 0.95 ) * 5000, 0.001 ) );
            SVF.setResonance( 0.1 + fmin( fabs( audioControlData[ RESONANCE ] ), 0.9 ) );
            
            double modulator = VCO2.sinewave( fabs( 20 + audioControlData[ MODULATOR ]  * 1000 ) ) * ( 500 * fabs( audioControlData[ AMP ] ) );
            double carrier = VCO1.sinewave( fabs( audioControlData[ CARRIER ] + modulator ) );
            double filtered = SVF.play( carrier, 1.0, 0, 0, 0 ) * 0.25;
            double delay1 = DL1.dl( filtered, dFilt2.lopass( fmin( fabs( audioControlData[ SIZE_A ] ) * 88200, 88199 ), 0.01 ),
                                   fabs( audioControlData[ FB_A ] ) );
            double delay2 = DL2.dl( filtered, dFilt3.lopass( fmin( fabs( audioControlData[ SIZE_B ] ) * 88200, 88199 ), 0.01 ),
                                   fabs( audioControlData[ FB_B ] ) );
            
            output[ i * nChannels ] = filtered * 0.75 + delay1 * 0.125 + delay2 * 0.0125;
            output[ i * nChannels + 1 ] = filtered * 0.75 + delay1 * 0.0125 + delay2 * 0.125;
        }
    }

}
