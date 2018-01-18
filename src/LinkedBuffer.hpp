//
//  LinkedBuffer.hpp
//  Lock-free (except when linking atm) buffer which propagates its changes to linked buffers
//  All linked buffers can be in different threads (read: should be)
//  Requires an update function to be called in the thread which it resides in
//  Concurrent sets must be ordered to have synchronized operations, accumulation can be nonsync
//
//  Created by James on 04/01/2018.
//  Copyright © 2017 James Frink. All rights reserved.
//

/*
 TODOS:
 - Cache line optimization!!!
 - Write own underlying allocator so ( AUTO ) resizing can be lock-free
 - !!!!! Add iterators ( But not the c++17 deprecated one )
 - ACCUMULATOR!! ( This and certain operations can be concurrent without passing a + b, just + and b)
 - Make linking lock-free
 - Check for feedback loops when linking and throw error / return false if thats the case
 - !! Add more instructions, such as:
    -  Curves?
    - !Summing
    - !Checksum sharing
    - Function?
    - Make something useful of double operator setter? < Test new implementation
    - Resizing!
    - Allow different sized buffers with relative - interpolated - sets?
 
  allow all integer and float types in subscript operator
 */

// This is mostly just an experiment

#ifndef LinkedBuffer_hpp
#define LinkedBuffer_hpp

#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "Spinlock.hpp"
#include "RingBufferAny.hpp"

template < class T >
class LinkedBuffer {
public:
    
    template < class subT >
    struct Subscript {
        subT index;
        LinkedBuffer< T >* parent;
        T operator= ( const T& val )
        {
            parent->set( index, val );
            return val;
        }
        operator T ( void ) const
        {
            return parent->get( index );
        }
    };
    
    const T& get ( size_t index ) const;
    const T& get ( double index ) const;
    
    Subscript< size_t > operator[  ] ( const int index ); // Set single value
    Subscript< double > operator[  ] ( const double index );  // Set interpolated
    void set ( size_t index, T value );
    void set ( double index, T value ); // For interpolated setting
    
    //void accumulate ( size_t index, T value );
    void assign ( size_t startIndex, T* values, size_t numItems );
    void setRangeToConstant ( size_t startIndex, size_t stopIndex, T value );
    void setLine ( double startIndex, T startValue, double stopIndex, T stopValue );
    
    void setup ( size_t bufferSize ); // Sets up buffer size and queue sizes
    void resize ( size_t bufferSize ); // TODO: propegate resize message?
    void resizeBounded ( size_t bufferSize ); // Resize without trying to alloc
    
    size_t size ( void ) const;
    
    // TODO: Check for possible feedback loops
    bool linkTo ( LinkedBuffer* const other ); // Returns true on link (false on == this or if null)
    bool unLinkFrom ( LinkedBuffer* const other ); // Returns true on unlink, false if doesn't exist
    
    bool update ( void );
    
    uint32_t maxDataToParsePerUpdate = 4096; // Set this to a value that doesn't block your thread for forever
    
    size_t trueSize;
    
private:
    template < class TT >
    void propagate ( const LinkedBuffer* from, TT* const data, size_t numItems );
    
    RingBufferAny comQueue; // Lock-free linked buffer communications queue
    
    std::vector< LinkedBuffer< T >* > links;
    Spinlock editLinks;
    
    std::vector< T > localMemory;
    size_t memorySize;
    
    enum MessageTypes {
        BOUNDED_MEMORY_RESIZE,
        SINGLE_INDEX,
        SET_RANGE_TO_CONSTANT,
        SET_LINE,
        INDEX_SPECIFIER_ACCUM, // TODO
        DATA,
        NONE
    };
    
    // Command struct headers
    struct ControlMessage {
        const LinkedBuffer* lastSource; // For propagation ( don't send back to source )
        MessageTypes type = NONE;
        
        union message {
            struct BoundedResizeHeader {
                size_t size;
            } boundedResizeHeader;
            
            struct SingleIndexHeader {
                size_t index;
            } singleIndexHeader;
            
            struct SetRangeToConstantHeader {
                size_t startIndex;
                size_t stopIndex;
                T value;
            } setRangeToConstantHeader;
            
            struct SetLineHeader {
                double startIndex;
                T startValue;
                double distance;
                T deltaIncrement;
            } setLineHeader;
        } containedMessage;
    };
    // Add instructions such as resize buffer etc
    
    // TODO: Union or something like it to create better header instruction storage
    ControlMessage lastIndexSpecifier;
    
    static const size_t tSize = sizeof ( T );
};

template < class T >
const T& LinkedBuffer< T >::get ( const size_t index ) const
{
    return localMemory[ index ];
}
template < class T >
const T& LinkedBuffer< T >::get ( const double index ) const
{
    size_t truncPos = index; // Truncate
    double fractAmt = index - truncPos;
    return localMemory[ truncPos ] * ( 1.0 - fractAmt ) + localMemory[ ++truncPos ] * fractAmt;
}

template < class T >
typename LinkedBuffer< T >::template Subscript< size_t > LinkedBuffer< T >::operator[  ] ( const int index )
{ // Single value setting through subscript operator
    return Subscript< size_t > { static_cast< size_t >( index ), this };
}
template < class T >
typename LinkedBuffer< T >::template Subscript< double > LinkedBuffer< T >::operator[  ] ( const double index )
{ // Interpolated setting through subscript operator
    return Subscript< double > { index , this };
}

template < class T >
void LinkedBuffer< T >::set ( size_t index, T value )
{
    // Single value
    ControlMessage hm;
    hm.lastSource = this;
    hm.type = SINGLE_INDEX;
    hm.containedMessage.singleIndexHeader.index = index;
    if ( !comQueue.push( &hm, 1 ) )
        throw std::runtime_error( "Item(s) does not fit in queue, add auto-resizing here once == non-blocking" );
    // Propagate the value on next update
    if ( !comQueue.push( &value, 1 ) )
        throw std::runtime_error( "Item(s) does not fit in queue, add auto-resizing here once == non-blocking" );
}
template < class T >
void LinkedBuffer< T >::set ( double index, T value )
{ // Linear interpolated set
    T values[2]; // Linear interpolated values starting at index x
    size_t truncPos = index; // Truncate
    
    if ( truncPos >= 0 && truncPos < memorySize )
    { // Only set value if within bounds ( unsigned )
        double fractAmt = index - truncPos;
        size_t secondPos = truncPos + 1;
        bool fits = ( secondPos < memorySize );
        if ( fits )
        {
            double iFractAmt = ( 1.0 - fractAmt );
            values[ 0 ] = ( localMemory[ truncPos ] * fractAmt ) + ( value * iFractAmt );
            values[ 1 ] =  ( localMemory[ secondPos ] * iFractAmt ) + ( value * fractAmt );
        } else {
            values[0] = value;
        }
            
        ControlMessage hm;
        hm.lastSource = this;
        hm.type = SINGLE_INDEX;
        hm.containedMessage.singleIndexHeader.index = truncPos;
        // Propagate the header on next update
        if ( !comQueue.push( &hm, 1 ) )
            throw std::runtime_error( "Item(s) does not fit in queue, add auto-resizing here once == non-blocking" );
        // Propagate the value(s) on next update
        if ( !comQueue.push( values, ( fits ) ? 2 : 1 ) )
            throw std::runtime_error( "Item(s) does not fit in queue, add auto-resizing here once == non-blocking" );
    }
}

template < class T >
void LinkedBuffer< T >::assign ( size_t startIndex, T* values, size_t numItems )
{
    ControlMessage hm;
    hm.lastSource = this;
    hm.type = SINGLE_INDEX;
    hm.containedMessage.singleIndexHeader.index = index;
    // Propagate the header on next update
    if ( !comQueue.push( &hm, 1 ) )
        throw std::runtime_error( "Item(s) does not fit in queue, add auto-resizing here once == non-blocking" );
    // Propagate the value(s) on next update
    if ( !comQueue.push( &values, numItems ) )
        throw std::runtime_error( "Item(s) does not fit in queue, add auto-resizing here once == non-blocking" );
}

template < class T >
void LinkedBuffer< T >::setRangeToConstant( size_t startIndex, size_t stopIndex, T value )
{
    ControlMessage hm;
    hm.lastSource = this;
    hm.type = SET_RANGE_TO_CONSTANT;
    hm.containedMessage.setRangeToConstantHeader.startIndex = startIndex;
    hm.containedMessage.setRangeToConstantHeader.stopIndex = stopIndex;
    hm.containedMessage.setRangeToConstantHeader.value = value;
    // Propagate header
    if ( !comQueue.push( &hm, 1 ) )
        throw std::runtime_error( "Item(s) does not fit in queue, add auto-resizing here once == non-blocking" );
}

template < class T >
void LinkedBuffer< T >::setLine ( double startIndex, T startValue, double stopIndex, T stopValue )
{
    double distance = stopIndex - startIndex;
    if ( distance < 0 )
    {
        distance = std::abs( distance );
        std::swap( startValue, stopValue );
        std::swap( startIndex, stopIndex );
    }
    if ( distance > 0 )
    { // Disallow div by zero
        ControlMessage hm;
        hm.lastSource = this;
        hm.type = SET_LINE;
        hm.containedMessage.setLineHeader.startIndex = startIndex;
        hm.containedMessage.setLineHeader.startValue = startValue;
        hm.containedMessage.setLineHeader.distance = distance;
        hm.containedMessage.setLineHeader.deltaIncrement = ( stopValue - startValue ) / distance;
        // Propagate the header on next update
        if ( !comQueue.push( &hm, 1 ) )
            throw std::runtime_error( "Item(s) does not fit in queue, add auto-resizing here once == non-blocking" );
    }
}

template < class T >
void LinkedBuffer< T >::setup ( size_t bufferSize )
{
    resize( bufferSize );
}

template < class T >
void LinkedBuffer< T >::resize ( size_t bufferSize )
{
    localMemory.resize( bufferSize );
    memorySize = bufferSize;
    trueSize = memorySize;
    comQueue.resize( ( bufferSize * tSize ) * 4 ); // This is the memory overhead
}

template < class T >
void LinkedBuffer< T >::resizeBounded ( size_t bufferSize )
{ // Just sets memory size ( unless > trueSize )
    ControlMessage hm;
    hm.lastSource = this;
    hm.type = BOUNDED_MEMORY_RESIZE;
    hm.containedMessage.boundedResizeHeader.size = bufferSize;
    // Propagate the header on next update
    if ( !comQueue.push( &hm, 1 ) )
        throw std::runtime_error( "Item(s) does not fit in queue, add auto-resizing here once == non-blocking" );
}

template < class T >
size_t LinkedBuffer< T >::size ( void ) const
{
    return memorySize;
}

template < class T >
bool LinkedBuffer< T >::linkTo ( LinkedBuffer< T >* const other )
{
    if ( other == this || other == nullptr )
        return false;
    
    // TODO: Check here for feedback loop
    
    // TODO: Should effort be made to make this lock-free?
    // Reasoning: live linking, not only in setup?
    editLinks.lock(  );
    links.push_back( other );
    editLinks.unlock(  );
    
    return true;
}

template < class T >
bool LinkedBuffer< T >::unLinkFrom ( LinkedBuffer< T >* const other )
{
    if ( other == this || other == nullptr )
        return false;
    
    bool success = false;
    
    // TODO: Should effort be made to make this lock-free?
    // Reasoning: live linking, not only in setup?
    editLinks.lock(  );
    
    auto it = std::find( links.begin(  ), links.end(  ), other );
    if ( it != links.end(  ) )
    {
        links.erase( it );
        success = true;
    }
    
    editLinks.unlock(  );
    return success;
}

template < class T >
bool LinkedBuffer< T >::update ( void )
{
    uint32_t parsedData = 0;
    bool anyNewData = false;
    
    RingBufferAny::VariableHeader headerInfo;
    while ( comQueue.anyAvailableForPop( headerInfo ) && ( ++parsedData < maxDataToParsePerUpdate ) )
    {
        if ( headerInfo.type_index == typeid( ControlMessage ) ) {
            
            comQueue.pop( &lastIndexSpecifier, 1 );
            
            switch ( lastIndexSpecifier.type )
            {
                case SET_RANGE_TO_CONSTANT:
                {
                    auto m = lastIndexSpecifier.containedMessage.setRangeToConstantHeader;
                    if ( m.stopIndex <= memorySize )
                        std::fill( localMemory.begin(  ) + m.startIndex,
                                   localMemory.begin(  ) + m.stopIndex,
                                    m.value );
                    
                    anyNewData = true;
                    break;
                }
                case SET_LINE:
                {
                    auto m = lastIndexSpecifier.containedMessage.setLineHeader;
                    double currentIndex = m.startIndex;
                    if ( m.startIndex >= 0 && m.startIndex < memorySize )
                    {
                        T startValue = m.startValue;
                        T deltaIncrement = m.deltaIncrement;
                        for ( size_t i = 0; i < m.distance; ++i )
                        {
                            size_t truncPos = currentIndex; // Truncate
                            double fractAmt = currentIndex - truncPos;
                            size_t secondPos = truncPos + 1;
                            T value = startValue + deltaIncrement * i;
                            
                            if ( secondPos < memorySize )
                            {
                                double iFractAmt = ( 1.0 - fractAmt );
                                localMemory[ truncPos ] = ( localMemory[ truncPos ] * fractAmt ) + ( value * iFractAmt );
                                localMemory[ secondPos ] =  ( localMemory[ secondPos ] * iFractAmt ) + ( value * fractAmt );
                            } else {
                                localMemory[ truncPos ] = value;
                            }
                            currentIndex += 1;
                        }
                        
                        anyNewData = true;
                    }
                    break;
                }
                case BOUNDED_MEMORY_RESIZE:
                {
                    size_t newSize = lastIndexSpecifier.containedMessage.boundedResizeHeader.size;
                    if ( newSize <= trueSize )
                    {
                        memorySize = newSize;
                    } else {
                        // TODO: Allocator
                        resize( newSize );
                    }
                    break;
                }
                    
                default:
                    break;
            }
            
            // Propagate header
            ControlMessage nextIndexSpecifier( lastIndexSpecifier );
            nextIndexSpecifier.lastSource = this;
            propagate( lastIndexSpecifier.lastSource, &nextIndexSpecifier, 1 );
            
        } else if ( headerInfo.type_index == typeid( T ) ) {
            
            if ( lastIndexSpecifier.type == SINGLE_INDEX )
            {
                auto m = lastIndexSpecifier.containedMessage.singleIndexHeader;
                if ( m.index < memorySize )
                {
                    // Set the value(s) locally
                    auto setIndex = ( localMemory.begin(  ) + m.index ).operator->();
                    comQueue.pop( setIndex, headerInfo.valuesPassed );
                    
                    propagate( lastIndexSpecifier.lastSource, setIndex, headerInfo.valuesPassed );

                    anyNewData = true;
                }
            } else {
                std::runtime_error( "Data was not expected" );
            }
            
            lastIndexSpecifier = ControlMessage(  ); // Reset
            
        } else {
            throw std::runtime_error( "Uncaught data" );
        }
    }
    
    return anyNewData;
}

template < class T >
template < class TT >
void LinkedBuffer< T >::propagate ( const LinkedBuffer* from, TT* const data, size_t numItems )
{
    if ( from == nullptr )
        throw std::runtime_error( " Propagation source is null " );
    
    // Propagation is spread across update methods of multiple threads
    for ( LinkedBuffer* lb : links )
    {
        if ( lb != from )
        {
            if ( !lb->comQueue.push( data, numItems ) )
                throw std::runtime_error( "Item(s) does not fit in queue, add auto-resizing here once == non-blocking" );
        }
    }
}


#endif /* LinkedBuffer_hpp */
