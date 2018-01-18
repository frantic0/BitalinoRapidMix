//
//  GlobalDefs.h
//  Bitalino
//
//  Created by James on 08/12/2017.
//
//

#ifndef GlobalDefs_h
#define GlobalDefs_h

namespace BIT
{
    // Synth parameters
    // Internal states
    struct RecordingSelection {
        double position;
    };
    enum State {
        TRAINING,
        PERFORMING,
        RECORDING,
        PLAYING,
        IDLE,
        NUMBER_OF_STATES
    };
    static const char* stateStrings[NUMBER_OF_STATES]
    {
        "Training",
        "Performing",
        "Recording",
        "Playing",
        "Idle"
    };
    //
    
    enum SynthControl
    {
        CARRIER,
        MODULATOR,
        AMP,
        CUTOFF,
        RESONANCE,
        SIZE_A,
        SIZE_B,
        FB_A,
        FB_B,
        NUM_SYNTH_PARAMS
    };
    static const char* ParamNames[NUM_SYNTH_PARAMS] =
    {
        "1 Carrier",
        "2 Modulator",
        "3 Amp",
        "4 Cutoff",
        "5 Resonance",
        "6 SizeA",
        "7 SizeB",
        "8 FeedbackA",
        "9 FeedbackB"
    };
    
    // Major Scale
    static const uint8_t majorScaleSize = 7;
    static const uint8_t majorScale[majorScaleSize]
    {
        0, 2, 4, 5, 7, 9, 11
    };

    
    // Synth ctrldata struct
    struct SynthControlData
    {
        double values[BIT::NUM_SYNTH_PARAMS];
        SynthControlData ( void )
        {
        }
        
        SynthControlData ( std::vector<double> data )
        {
            setValues (data);
        }
        
        double& operator[] (const BIT::SynthControl index)
        {
            return values[static_cast<uint32_t>(index)];
        }
        
        int32_t setValues ( std::vector<double>& data )
        { // Returns number of elements which exceeds limit of synth param size
            std::copy(data.begin(), data.begin() + BIT::NUM_SYNTH_PARAMS, values);
            return data.size() - BIT::NUM_SYNTH_PARAMS;
        }
        
        std::vector<double> getValuesAsVector ( void )
        {
            return std::vector<double>(std::begin(values), std::end(values));
        }
    };
}

#endif /* GlobalDefs_h */
