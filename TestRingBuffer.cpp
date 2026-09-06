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

#include "RingBuffer.h"

#include "TestSuite.h"

#include <iostream>

void test_RingBuffer( TestSuite & test_suite )
{
    std::cout << "Now running tests for RingBuffer." << std::endl;

    {
    RingBuffer< size_t > dummy( 10 );
    test_suite.test_equality( dummy.empty(), true, "RingBuffer 01" );
    dummy.write( 5 );
    test_suite.test_equality( dummy.size(), 1, "RingBuffer 02" );
    test_suite.test_equality( dummy.read(), 5, "RingBuffer 03" );
    test_suite.test_equality( dummy.size(), 0, "RingBuffer 04" );
    }
std::cout << "I got here 1" << std::endl;

    {
        try
        {
        RingBuffer< size_t > dummy( 10 );
        dummy.read();
        test_suite.log_error( "RingBuffer::read() should have thrown 01" );
        }
        catch ( std::exception & e )
        {
        }
    }
std::cout << "I got here 2" << std::endl;
    {
        try
        {
        RingBuffer< size_t > dummy( 0 );
        test_suite.log_error( "RingBuffer::RingBuffer( 0 ) should have thrown 01" );
        }
        catch ( std::exception & e )
        {
        }
    }
std::cout << "I got here 3" << std::endl;
    {
        RingBuffer< size_t > dummy( 10 );
        dummy.write(  1 );
        dummy.write(  2 );
        dummy.write(  3 );
        dummy.write(  4 );
        dummy.write(  5 );
        dummy.write(  6 );
        test_suite.test_equality( dummy.full(), false, "RingBuffer 05" );
        test_suite.test_equality( dummy.empty(), false, "RingBuffer 06" );
        dummy.write(  7 );
        dummy.write(  8 );
        dummy.write(  9 );
        dummy.write( 10 );
        test_suite.test_equality( dummy.read(), 1, "RingBuffer 07" );
        test_suite.test_equality( dummy.read(), 2, "RingBuffer 08" );
        dummy.write(  1 );
        test_suite.test_equality( dummy.size(), 9, "RingBuffer 09" );
        dummy.write(  2 );
        test_suite.test_equality( dummy.size(), 10, "RingBuffer 10" );
        test_suite.test_equality( dummy.full(), true, "RingBuffer 11" );
        try
        {
        dummy.write( 3 );
        test_suite.log_error( "RingBuffer::write() should have thrown 01" );
        }
        catch ( std::exception & e )
        {
        }
    }
}

