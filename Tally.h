#ifndef TALLY_H
#define TALLY_H

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

#include <iostream>
#include <map>
#include <vector>

/*
 * Counts number of items.
 * 
 * Basically gives a C++ container an old-fashioned index-based interface, probably not good.
 */
template <class T>
class Tally
{
public:

    Tally() {}

    void add( const T & t, const size_t counts = 1 )
    {
        if ( counts == 0 )
            return;
        if ( contains( t ) )
            counts_[ t ] += counts;
        else
            counts_[ t ] = counts;
    }

    size_t size() const { return counts_.size(); }

    bool contains( const T & t ) const
    {
        return ( counts_.find( t ) != counts_.end() );
    }

    std::vector< T > values() const
    {
        std::vector< T > result;
        for ( typename std::map< T, size_t >::const_iterator it( counts_.begin() ); it != counts_.end(); ++it )
        {
            result.push_back( it->first );
        }
        return result;
    }

    size_t counts( const T & t ) const
    {
        typename std::map< T, size_t >::const_iterator it = counts_.find( t );
        if ( it == counts_.end() )
            return 0;
        return it->second;
    }

    void show() const
    {
        for ( typename std::map< T, size_t >::const_iterator it( counts_.begin() ); it != counts_.end(); ++it )
        {
            std::cout << it->first << " " << it->second << std::endl;
        }
    }

private:

    std::map< T, size_t > counts_;
};

#endif // TALLY_H

