#ifndef RINGBUFFER_H
#define RINGBUFFER_H

/* *********************************************
Copyright (c) 2013-2026, Cornelis Jan (Jacco) van de Streek
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of my employers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL CORNELIS JAN VAN DE STREEK BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************* */

#include <stdexcept>
#include <vector>

/*
  A ring buffer. Writing appends at the end, reading reads from the front.
  @@@ Size should really be infinite, but that requires a bit of programming.
*/
template <class T>
class RingBuffer
{
public:

    RingBuffer( const size_t maximum_size )
    : maximum_size_(maximum_size),
    head_(0),
    tail_(0)
    {
        if ( maximum_size == 0 )
            throw std::runtime_error( "RingBuffer::RingBuffer(): maximum size cannot be zero." );
        data_.reserve( maximum_size );
    }

    size_t size() const
    {
        if ( head_ < tail_ )
            return maximum_size_ - tail_ + head_;
        return head_ - tail_;
    }

    bool empty() const { return ( size() == 0 ); }

    bool full() const { return ( size() == maximum_size_ ); }

    T read()
    {
        if ( empty() )
            throw std::runtime_error( "RingBuffer::read(): buffer is empty." );
        T result = data_[ tail_ ];
        cyclic_increment( tail_ );
        return result;
    }

    void write( const T & t )
    {
        if ( full() )
            throw std::runtime_error( "RingBuffer::write(): buffer is full." );
        if ( data_.size() < maximum_size_ )
        {
            if ( data_.size() != head_ )
                throw std::runtime_error( "RingBuffer::write(): programming error." );
            data_.push_back( t );
        }
        else
            data_[ head_ ] = t;
        cyclic_increment( head_ );
    }

private:
    std::vector< T > data_;
    size_t maximum_size_;
    size_t head_;
    size_t tail_;

    void cyclic_increment( size_t & i )
    {
    if ( i == ( maximum_size_ - 1 ) )
        i = 0;
    else
        ++i;
    }

};

#endif // RINGBUFFER_H

