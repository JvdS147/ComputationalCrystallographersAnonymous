#include "Pressure.h"
#include "TestSuite.h"

void test_Pressure( TestSuite & test_suite )
{
    test_suite.test_equality_double(
        Pressure( 1.0, Pressure::GPa ).value_in_GPa(),
        1.0,
        "Pressure GPa conversion" );

    test_suite.test_equality_double(
        Pressure( 1.0, Pressure::MPa ).value_in_GPa(),
        0.001,
        "Pressure MPa conversion" );

    test_suite.test_equality_double(
        Pressure( 1.0, Pressure::bar ).value_in_GPa(),
        0.0001,
        "Pressure bar conversion" );

    test_suite.test_equality_double(
        Pressure( 1.0, Pressure::atm ).value_in_Pa(),
        101325.0,
        "Pressure atm conversion" );

    test_suite.test_equality_double(
        Pressure::ambient_pressure().value_in_Pa(),
        101325.0,
        "Pressure ambient pressure" );
}

