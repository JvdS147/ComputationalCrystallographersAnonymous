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

#include "Complex.h"
#include "Angle.h"

#include "TestSuite.h"

#include <iostream>

    void test_one_complex( TestSuite & test_suite,
                           const Complex & lhs,
                           const Complex & rhs,
                           const std::string & error_message )
    {
        test_suite.test_equality_double( lhs.real()     , rhs.real()      , error_message + " (real)" );
        test_suite.test_equality_double( lhs.imaginary(), rhs.imaginary() , error_message + " (imaginary)" );
    }


void test_Complex( TestSuite & test_suite )
{
    std::cout << "Now running tests for Complex." << std::endl;

    {
    Complex dummy;
    test_suite.test_equality( dummy.real(), 0.0, "Complex() 01" );
    test_suite.test_equality( dummy.imaginary(), 0.0, "Complex() 02" );
    }
    {
    Complex dummy( 5.0 );
    test_suite.test_equality( dummy.real(), 5.0, "Complex() 03" );
    test_suite.test_equality( dummy.imaginary(), 0.0, "Complex() 04" );
    }
    {
    Complex dummy( 3.0, 7.0 );
    test_suite.test_equality( dummy.real(), 3.0, "Complex() 05" );
    test_suite.test_equality( dummy.imaginary(), 7.0, "Complex() 06" );
    }
    {
    test_one_complex( test_suite, exponential( Complex::i() * CONSTANT_PI ), Complex( -1.0 ), "expontential( Complex )" );
    }
    {
    Complex dummy( 3.0, 7.0 );
    test_one_complex( test_suite, square( dummy ), dummy * dummy, "square( Complex )" );
    }
    {
    Complex dummy_1( 3.0, 7.0 );
    Complex dummy_2( 3.0, 7.0 );
    Complex dummy_3( 3.0, 7.0 );
    test_suite.test_equality( triquality( dummy_1, dummy_2, dummy_3 ), true, "triquality( Complex ) 01" );
    }
    {
    Complex dummy_1( 3.0, 7.0 );
    Complex dummy_2( 3.0, 7.0 );
    Complex dummy_3( 4.0, 8.0 );
    test_suite.test_equality( triquality( dummy_1, dummy_2, dummy_3 ), false, "triquality( Complex ) 02" );
    }
    {
    Complex dummy_1( 3.0, 7.0 );
    Complex dummy_2( 4.0, 8.0 );
    Complex average_1 = average( dummy_1, dummy_2 );
    test_one_complex( test_suite, average_1, Complex( 3.5, 7.5 ), "average( Complex ) 01" );
    Complex dummy_3( 5.0, 9.0 );
    average_1 = average( dummy_3, average_1, 2.0 );
    test_one_complex( test_suite, average_1, Complex( 4.0, 8.0 ), "average( Complex ) 02" );
    }

}

