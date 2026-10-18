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

#include "Tally.h"

#include "TestSuite.h"

#include <iostream>

void test_Tally( TestSuite & test_suite )
{
    std::cout << "Now running tests for Tally." << std::endl;
    {
    Tally< size_t > tally;
    test_suite.test_equality( tally.size(), 0, "Tally 01" );
    tally.add( 5 );
    test_suite.test_equality( tally.counts( 5 ), 1, "Tally 02" );
    tally.add( 9, 1  );
    tally.add( 7, 0 );
    tally.add( 5 );
    test_suite.test_equality( tally.size(), 2, "Tally 03" );
    test_suite.test_equality( tally.counts( 5 ), 2, "Tally 04" );
    tally.add( 5, 33 );
    test_suite.test_equality( tally.counts( 5 ), 35, "Tally 05" );
    test_suite.test_equality( tally.counts( 9 ), 1, "Tally 06" );
    test_suite.test_equality( tally.contains( 7 ), false, "Tally 07" );
    tally.add( 7, 10 );
    std::vector< size_t > values = tally.values();
    test_suite.test_equality( values[ 0 ], 5, "Tally 08" );
    test_suite.test_equality( values[ 1 ], 7, "Tally 09" );
    test_suite.test_equality( values[ 2 ], 9, "Tally 10" );
    }
}

