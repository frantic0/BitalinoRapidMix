//
//  RapidMixThread/h
//  This implementation of rapidMix trains in a different thread and has a synchronized
//  update process with your main thread, which triggers a ( sync ) callback on doneTraining.
//  Allows you to train one model and keep using it while the next one trains, the new
//  model will be swapped in sync with the thread the update is called in once it's done.
//  Created by James on 19/11/2017.
//
//

#ifndef RapidMixThread_h
#define RapidMixThread_h

#include <algorithm>
#include <functional>
#include <unistd.h> // usleep
#include "ThreadedProcess.h"
#include "rapidmix.h"

typedef rapidmix::staticRegression selectedTraining;

class RapidMixThread : public ThreadedProcess
{
public:
    std::function< void ( void ) > doneTrainingCallback = nullptr;
    selectedTraining* model = nullptr; // The model to be accessed from outside this thread
    
    RapidMixThread ( void ) : ThreadedProcess (  )
    {
        training.store( false );
        doneTraining.store( false );
    }
    
    ~RapidMixThread ( void )
    {
        stopThread(  );
        if ( newModel )
            delete newModel;
        if ( model )
            delete model;
        printf ( "Stop rapidmixthread\n" );
    }
    
    void setup ( void )
    {
        model = createModel(  );
        startThread( 100000 ); // start training thread with sleep time of 100 ms
    }
    
    bool train ( rapidmix::trainingData& data )
    {
        if ( !training.load(  ) )
        {
            this->trainingData = data;
            training.store( true );
            return true;
        }
        return false;
    }
    
    bool isTraining ( void ) const
    {
        return training.load(  );
    }
    
    void update ( void )
    { // To be called in your main loop
        if ( doneTraining.load(  ) )
        {
            if ( newModel )
            {
                std::swap( model, newModel );
            } else
                throw std::runtime_error( "New model was not created successfully" );
            
            if ( doneTrainingCallback )
                doneTrainingCallback(  );
            
            //printf( "Done training\n" );
            doneTraining.store( false );
        }
    }
    
protected:
    selectedTraining* createModel ( void )
    {
        // Doing this to prevent repetitive stuff when testing different models
        selectedTraining* t = new selectedTraining(  );
        t->setNumHiddenNodes( 18 );
        return t;
    }
    
    void mainThreadCallback ( void )
    { // Training thread
        if ( isTraining(  ) )
        {
            if ( newModel )
                delete newModel;
            
            newModel = createModel(  );
            
            //printf( "Started training in thread... \n" );
            if ( !trainingData.trainingSet.empty(  ) && trainingData.trainingSet[0].elements.size(  ) > 1 )
                if ( !newModel->train( trainingData ) )
                    throw std::runtime_error( "Training failed" );;
            
            training.store( false );
            doneTraining.store( true );
        }
    }
    
    std::atomic< bool > training;
    std::atomic< bool > doneTraining;
    selectedTraining* newModel = nullptr;
    rapidmix::trainingData trainingData;
    
private:
};

#endif /* RapidMixThread_h */
