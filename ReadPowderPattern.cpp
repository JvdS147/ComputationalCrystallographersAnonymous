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

#include "ReadPowderPattern.h"
#include "BasicMathsFunctions.h"
#include "FileName.h"
#include "PowderPattern.h"
#include "RingBuffer.h"
#include "StringFunctions.h"
#include "TextFileReader.h"
#include "TextFileReader_2.h"
#include "Utilities.h"

#include <stdexcept>

// ********************************************************************************

//      <SubScans>
//        <SubScanInfo Steps="1051" MeasuredSteps="1051" StartStepNo="0" MeasuredTimePerStep="260" PlannedTimePerStep="2" />
//      </SubScans>
//      <Datum>260,1,2,1,662</Datum>
//      <Datum>260,1,2.0409,1.0205,599</Datum>
//      <Datum>260,1,2.0819,1.0409,603</Datum>
PowderPattern read_brml( const FileName & file_name )
{
    PowderPattern result;
    // This is lab data (is that always true?), the wavelength is fine.
    TextFileReader text_file_reader( file_name );
    text_file_reader.set_skip_empty_lines( false );
    Splitter splitter( "," );
    std::string line;
    while ( text_file_reader.get_next_line( line ) )
    {
        line = strip( line );
        line = to_upper( line );
        if ( line.substr( 0, 7 ) != "<DATUM>" )
            continue;
        line = extract_delimited_text( line, "<DATUM>", "</DATUM>" );
        std::vector< std::string > words = splitter.split( line );
        result.push_back( Angle::from_degrees( string2double( words[2] ) ), string2double( words[4] ) );
    }
    return result;
}

// ********************************************************************************

// We have a couple of problems here:
// The cif may be huge and full of other stuff that we do not need (it may even have multiple structures
// with multiple powder diffraction patterns)
//_pd_meas_2theta_range_min  0.50000
//_pd_meas_2theta_range_max  49.99654
//_pd_meas_2theta_range_inc  0.00100
//_pd_meas_number_of_points  49575
//
//loop_
//   _pd_meas_intensity_total
//   _pd_calc_intensity_total
//   _pd_proc_intensity_bkg_calc
//   _pd_proc_ls_weight
//  878.115218   0            0           0
//  845.112609   0            0           0
//  830.491604   0            0           0
//  848.287054   0            0           0
//  874.178024   0            0           0
//  834.93539    0            0           0
//
//
//_pd_meas_2theta_range_min     5.0019
//_pd_meas_2theta_range_inc     0.0025
//_pd_meas_2theta_range_max     40.0019
//_pd_meas_number_of_points     14001
//
//loop_
//      _pd_meas_counts_total
//32393   32393   32357   32330   32321   32406   32472   32505   32492   32484   #   5.0519
//32484   32519   32512   32454   32442   32433   32426   32386   32380   32410   #   5.0769
//32487   32525   32526   32508   32489   32469   32518   32530   32509   32492   #   5.1019
//32352   32361   32416   32393   32376   32386   32388   32390   32391   32392   #   5.0269
//32309   32331   32335   32310   32293   32289   32307   32350   32377   32370   #   5.0019
PowderPattern read_cif( const FileName & file_name )
{
    PowderPattern result;
    TextFileReader text_file_reader( file_name );
    text_file_reader.set_skip_empty_lines( true );
    std::vector< std::string > words;
    Angle two_theta_start;
    Angle two_theta_step;
    Angle two_theta_end;
    size_t number_of_points( 0 );
    bool found_pd_meas_2theta_range_min( false );
    bool found_pd_meas_2theta_range_max( false );
    bool found_pd_meas_2theta_range_inc( false );
    bool found_pd_meas_number_of_points( false );
    std::vector< double > counts;
    while ( text_file_reader.get_next_line( words ) )
    {
        if ( words[0] == "_pd_meas_2theta_range_min" )
        {
            if ( words.size() != 2 )
                throw std::runtime_error( "PowderPattern::read_cif(): keyword _pd_meas_2theta_range_min does not have value." );
            two_theta_start = Angle::from_degrees( string2double( words[1] ) );
            found_pd_meas_2theta_range_min = true;
            continue;
        }
        if ( words[0] == "_pd_meas_2theta_range_max" )
        {
            if ( words.size() != 2 )
                throw std::runtime_error( "PowderPattern::read_cif(): keyword _pd_meas_2theta_range_max does not have value." );
            two_theta_end = Angle::from_degrees( string2double( words[1] ) );
            found_pd_meas_2theta_range_max = true;
            continue;
        }
        if ( words[0] == "_pd_meas_2theta_range_inc" )
        {
            if ( words.size() != 2 )
                throw std::runtime_error( "PowderPattern::read_cif(): keyword _pd_meas_2theta_range_inc does not have value." );
            two_theta_step = Angle::from_degrees( string2double( words[1] ) );
            found_pd_meas_2theta_range_inc = true;
            continue;
        }
        if ( words[0] == "_pd_meas_number_of_points" )
        {
            if ( words.size() != 2 )
                throw std::runtime_error( "PowderPattern::read_cif(): keyword _pd_meas_number_of_points does not have value." );
            number_of_points = string2integer( words[1] );
            found_pd_meas_number_of_points = true;
            continue;
        }
        if ( words[0] == "loop_" )
        {
            if ( words.size() != 1 )
                throw std::runtime_error( "PowderPattern::read_cif(): loop_ should be the only keyword on a line." );
            if ( ! text_file_reader.get_next_line( words ) )
                throw std::runtime_error( "PowderPattern::read_cif(): loop_ not followed by keywords." );
            std::vector< std::string > keywords;
            while ( ( words.size() == 1 ) && ( words[0].substr( 0, 1 ) == "_" ) )
            {
                keywords.push_back( words[0] );
                if ( ! text_file_reader.get_next_line( words ) )
                    throw std::runtime_error( "PowderPattern::read_cif(): loop_ followed by keyword but not by values." );
            }
            text_file_reader.push_back_last_line();
            // Find either _pd_meas_counts_total or _pd_meas_intensity_total.
            bool _pd_meas_counts_total_found( false );
            bool _pd_meas_intensity_total_found( false );
            for ( size_t i( 0 ); i != keywords.size(); ++i )
            {
                if ( keywords[i] == "_pd_meas_counts_total" )
                {
                    if ( _pd_meas_counts_total_found )
                        throw std::runtime_error( "PowderPattern::read_cif(): _pd_meas_counts_total found twice." );
                    _pd_meas_counts_total_found = true;
                }
                if ( keywords[i] == "_pd_meas_intensity_total" )
                {
                    if ( _pd_meas_intensity_total_found )
                        throw std::runtime_error( "PowderPattern::read_cif(): _pd_meas_intensity_total found twice." );
                    _pd_meas_intensity_total_found = true;
                }
            }
            if ( ( ! _pd_meas_counts_total_found ) && ( ! _pd_meas_intensity_total_found ) )
                throw std::runtime_error( "PowderPattern::read_cif(): either _pd_meas_counts_total or _pd_meas_intensity_total must be present." );
            size_t _pd_meas_counts_total_index;
            size_t _pd_meas_intensity_total_index;
            for ( size_t i( 0 ); i != keywords.size(); ++i )
            {
                if ( keywords[i] == "_pd_meas_counts_total" )
                    _pd_meas_counts_total_index = i;
                if ( keywords[i] == "_pd_meas_intensity_total" )
                    _pd_meas_intensity_total_index = i;
            }
            if ( _pd_meas_intensity_total_found && ( ! _pd_meas_counts_total_found ) )
                _pd_meas_counts_total_index = _pd_meas_intensity_total_index;
            std::string line;
            RingBuffer< double > ring_buffer( 100 );
            while ( text_file_reader.get_next_line( line ) )
            {
                line = remove_from( line, '#' );
                words = split( line );
                try
                {
                    for ( size_t i( 0 ); i != words.size(); ++i )
                        ring_buffer.write( string2double( words[ i ] ) );
                }
                catch ( std::exception & e )
                {
                    break;
                }
                while ( ring_buffer.size() >= keywords.size() )
                {
                    std::vector< double > next_batch;
                    for ( size_t i( 0 ); i != keywords.size(); ++i )
                        next_batch.push_back( ring_buffer.read() );
                    if ( _pd_meas_counts_total_found && _pd_meas_intensity_total_found )
                        if ( ! nearly_equal( next_batch[ _pd_meas_counts_total_index ], next_batch[ _pd_meas_intensity_total_index ] ) )
                            throw std::runtime_error( "PowderPattern::read_cif(): _pd_meas_counts_total and _pd_meas_intensity_total were both supplied, but with different values." );
                    counts.push_back( next_batch[ _pd_meas_counts_total_index ] );
                }
            }
            if ( ! ring_buffer.empty() )
                std::cout << "PowderPattern::read_cif(): Warning: ring buffer not empty." << std::endl;
            continue;
        }
    }
    if ( ! found_pd_meas_2theta_range_min )
        throw std::runtime_error( "PowderPattern::read_cif(): keyword _pd_meas_2theta_range_min not found." );
    if ( ! found_pd_meas_2theta_range_max )
        throw std::runtime_error( "PowderPattern::read_cif(): keyword _pd_meas_2theta_range_max not found." );
    if ( ! found_pd_meas_2theta_range_inc )
        throw std::runtime_error( "PowderPattern::read_cif(): keyword _pd_meas_2theta_range_inc not found." );
    // Consistency check.
    if ( found_pd_meas_number_of_points )
    {
        if ( number_of_points != counts.size() )
            throw std::runtime_error( "PowderPattern::read_cif(): _pd_meas_number_of_points inconsistent with the actual number of points." );
        Angle two_theta_step_should_be = ( two_theta_end - two_theta_start ) / ( number_of_points - 1 );
    }
    if ( counts.size() < 2 )
        throw std::runtime_error( "PowderPattern::read_cif(): there are fewer than two points in the pattern." );
    Angle two_theta_step_should_be = ( two_theta_end - two_theta_start ) / ( counts.size() - 1 );
    std::cout << "two_theta_step_should_be = " << two_theta_step_should_be << std::endl;
    std::cout << "two_theta_step           = " << two_theta_step << std::endl;
    for ( size_t i( 0 ); i != counts.size(); ++i )
    {
        result.push_back( two_theta_start + ( ( i * ( two_theta_end - two_theta_start ) ) / ( counts.size() - 1 ) ), counts[ i ] );
    }
    return result;
}

// ********************************************************************************

//11.0000   0.0200 111.0000
//  46.84    40.68    44.25    45.15    43.20    42.69    45.76    44.00
//  44.64    44.61    45.30    44.31    42.18    43.71    41.58    43.17
//  ...
PowderPattern read_dat( const FileName & file_name )
{
    PowderPattern result;
    // This is lab data (is that always true?), the wavelength is fine.
    TextFileReader text_file_reader( file_name );
    text_file_reader.set_skip_empty_lines( false );
    std::vector< std::string > words;
    if ( ! text_file_reader.get_next_line( words ) )
        throw std::runtime_error( "PowderPattern::read_dat(): First line is empty." );
    if ( words.size() != 3 )
        throw std::runtime_error( "PowderPattern::read_dat(): unexpected format." );
    Angle two_theta_start = Angle::from_degrees( string2double( words[0] ) );
    Angle two_theta_step = Angle::from_degrees( string2double( words[1] ) );
    Angle two_theta_end = Angle::from_degrees( string2double( words[2] ) );
    if ( two_theta_step < Angle::from_degrees( 0.0001 ) )
        throw std::runtime_error( "PowderPattern::read_dat(): 2theta step < 0.0001." );
    if ( two_theta_end < two_theta_start )
        throw std::runtime_error( "PowderPattern::read_dat(): 2theta end < 2theta start." );
    size_t npoints = round_to_int( ( ( two_theta_end - two_theta_start ) / two_theta_step ) ) + 1;
    if ( npoints < 2 )
        throw std::runtime_error( "PowderPattern::read_dat(): No data." );
    size_t i( 0 );
    while ( text_file_reader.get_next_line( words ) )
    {
        for ( size_t j( 0 ); j != words.size(); ++j )
        {
            result.push_back( ( i * two_theta_step ) + two_theta_start, string2double( words[j] ) );
            ++i;
        }
    }
    if ( i != npoints )
        std::cout << "PowderPattern::read_dat(): Warning: the number of data points in the file (" + size_t2string( i ) + ") disagrees with the number in the header (" + size_t2string( npoints ) + ")." << std::endl;
    else
        std::cout << "PowderPattern::read_dat(): the number of data points in the file (" + size_t2string( i ) + ") agrees with the number in the header (" + size_t2string( npoints ) + ")." << std::endl;
    std::cout << "PowderPattern::read_dat(): two_theta_end as calculated     = " << ( i * two_theta_step ) + two_theta_start << std::endl;
    std::cout << "PowderPattern::read_dat(): two_theta_end as read from file = " << two_theta_end << std::endl;
    return result;
}

// ********************************************************************************

//08/06/2018 07:47:49  DIF       : t=   600s
//  0.4460 0.0460  1.0 US 1.5418     87.5240  1893
//      3      2      4      3      4      3      6      4
//      3      2      6      9      8      6      7     15
//      9     11     21     23     24     64    188    323
// The start is 0.4460, the step is 0.0460 and the end is 87.5240-0.0460. There are 1893 points
// The 2theta end value is wrong, because the last data point is a dummy data point with a value of -1, and it is *not* counted towards the number of data points,
// but it *is* counted towards the end 2theta value.
PowderPattern read_mdi( const FileName & file_name )
{
    PowderPattern result;
    // This is lab data (is that always true?), the wavelength is fine.
    TextFileReader text_file_reader( file_name );
    text_file_reader.set_skip_empty_lines( false );
    std::vector< std::string > words;
    if ( ( ! text_file_reader.get_next_line( words ) ) || ( words.size() == 0 ) )
        throw std::runtime_error( "PowderPattern::read_mdi(): First line is empty." );
    if ( ! text_file_reader.get_next_line( words ) )
        throw std::runtime_error( "PowderPattern::read_mdi(): File is empty." );
    if ( words.size() != 7 )
        throw std::runtime_error( "PowderPattern::read_mdi(): unexpected format." );
    if ( words[3] != "US" )
        throw std::runtime_error( "PowderPattern::read_mdi(): unexpected format." );
    size_t ndata_points = string2integer( words[6] );
    if ( ndata_points == 0 )
        throw std::runtime_error( "PowderPattern::read_mdi(): No data." );
    Angle two_theta_start = Angle::from_degrees( string2double( words[0] ) );
    Angle two_theta_step = Angle::from_degrees( string2double( words[1] ) );
    Angle two_theta_end = Angle::from_degrees( string2double( words[5] ) );
    size_t i( 0 );
    bool we_are_done( false );
    while ( text_file_reader.get_next_line( words ) )
    {
        for ( size_t j( 0 ); j != words.size(); ++j )
        {
            // There is one dummy data point with value -1 after the last valid data point.
            if ( ( i == ndata_points ) && ( words[j] == "-1" ) && ( j == ( words.size() - 1 ) ) )
            {
                we_are_done = true;
            }
            else
            {
                if ( we_are_done )
                    throw std::runtime_error( "PowderPattern::read_mdi(): data found after last data point." );
               result.push_back( ( i * two_theta_step ) + two_theta_start, string2double( words[j] ) );
                ++i;
            }
        }
    }
    if ( i != ndata_points )
        std::cout << "PowderPattern::read_mdi(): Warning: the number of data points in the file (" + size_t2string( i ) + ") disagrees with the number in the header (" + size_t2string( ndata_points ) + ")." << std::endl;
    else
        std::cout << "PowderPattern::read_mdi(): the number of data points in the file (" + size_t2string( i ) + ") agrees with the number in the header (" + size_t2string( ndata_points ) + ")." << std::endl;
    std::cout << "PowderPattern::read_mdi(): two_theta_end as calculated     = " << ( i * two_theta_step ) + two_theta_start << std::endl;
    std::cout << "PowderPattern::read_mdi(): two_theta_end as read from file = " << two_theta_end << std::endl;
    return result;
}

// ********************************************************************************

//BANK       1    3501     350  CONST    23.20    2.30     0.0     0.0         STD
//       1       3       1       0       3       2       4       2       2       2
//       1       6       4       3       6       6       5       3       2       4
//       7      11      10      28      35      69     119     160     272     342
// The start is 0.232, the 2theta step size is 0.0230
PowderPattern read_raw( const FileName & file_name )
{
    PowderPattern result;
    // This is lab data (is that always true?), the wavelength is fine.
    TextFileReader text_file_reader( file_name );
    text_file_reader.set_skip_empty_lines( false );
    std::vector< std::string > words;
    if ( ! text_file_reader.get_next_line( words ) )
        throw std::runtime_error( "PowderPattern::read_raw(): File is empty." );
    if ( ! text_file_reader.get_next_line( words ) )
        throw std::runtime_error( "PowderPattern::read_raw(): File is empty." );
    if ( words.size() != 10 )
        throw std::runtime_error( "PowderPattern::read_raw(): unexpected format 1." );
    if ( words[0] != "BANK" )
        throw std::runtime_error( "PowderPattern::read_raw(): unexpected format 2." );
    if ( words[4] != "CONST" )
        throw std::runtime_error( "PowderPattern::read_raw(): unexpected format 3." );
    bool read_ESDs( false );
    if ( words[9] == "ESD" )
        read_ESDs = true;
    else if ( words[9] != "STD" )
        throw std::runtime_error( "PowderPattern::read_raw(): unexpected format 4." );
    size_t ndata_points = string2integer( words[2] );
    if ( ndata_points == 0 )
        throw std::runtime_error( "PowderPattern::read_raw(): No data." );
    Angle two_theta_start = Angle::from_degrees( string2double( words[5] ) / 100.0 );
    Angle two_theta_step = Angle::from_degrees( string2double( words[6] ) / 100.0 );
    size_t i( 0 );
    std::string line;
    Splitter splitter;
    splitter.split_by_length( 8 );
    while ( text_file_reader.get_next_line( line ) )
    {
        std::vector< std::string > temp_words = splitter.split( line );
        words.clear();
        bool one_word_was_empty( false );
        for ( size_t j( 0 ); j != temp_words.size(); ++j )
        {
            temp_words[j] = strip( temp_words[j] );
            if ( temp_words[j].empty() )
                one_word_was_empty = true;
            else
            {
                if ( one_word_was_empty )
                    throw std::runtime_error( "PowderPattern::read_raw(): non-empty word after empty word." );
                words.push_back( temp_words[j] );
            }
        }
        if ( read_ESDs )
        {
            if ( is_odd( words.size() ) )
                throw std::runtime_error( "PowderPattern::read_raw(): intensities plus ESDs stored, but number of values is odd." );
            for ( size_t j( 0 ); j != words.size(); j += 2 )
            {
                result.push_back( ( i * two_theta_step ) + two_theta_start, string2double( words[j] ), string2double( words[j+1] ) );
                ++i;
            }
        }
        else
        {
            for ( size_t j( 0 ); j != words.size(); ++j )
            {
                result.push_back( ( i * two_theta_step ) + two_theta_start, string2double( words[j] ) );
                ++i;
            }
        }
    }
    if ( i != ndata_points )
        std::cout << "PowderPattern::read_raw(): Warning: the number of data points in the file disagrees with the number in the header." << std::endl;
    return result;
}

// ********************************************************************************

PowderPattern read_txt( const FileName & file_name )
{
    PowderPattern result;
    TextFileReader text_file_reader( file_name );
    text_file_reader.set_skip_empty_lines( true );
    std::vector< std::string > words;
    size_t stage( 1 );
    Angle two_theta_start;
    Angle two_theta_step;
    Angle two_theta_end;
    size_t i( 0 );
    while ( text_file_reader.get_next_line( words ) )
    {
        if ( words[0] == "DATE" ||
             words[0] == "ACQTIME" ||
             words[0] == "VOLTAGE" ||
             words[0] == "CURRENT" ||
             words[0] == "WAVELENGTH" ||
             words[0] == "COMMENT1" ||
             words[0] == "COMMENT2" ||
             words[0] == "COMMENT3" )
        {
            if ( stage == 1 )
                continue;
            else
                throw std::runtime_error( "PowderPattern::read_txt(): keyword out of place." );
        }
        if ( words.size() == 3 )
        {
            if ( stage != 1 )
                throw std::runtime_error( "PowderPattern::read_txt(): keyword out of place." );
            two_theta_start = Angle::from_degrees( string2double( words[0] ) );
            two_theta_step  = Angle::from_degrees( string2double( words[1] ) );
            two_theta_end   = Angle::from_degrees( string2double( words[2] ) );
            stage = 3;
            continue;
        }
        if ( stage != 3 )
            throw std::runtime_error( "PowderPattern::read_txt(): keyword out of place." );
        if ( words.size() != 1 )
            throw std::runtime_error( "PowderPattern::read_txt(): keyword out of place." );
        result.push_back( ( i * two_theta_step ) + two_theta_start, string2double( words[0] ) );
        ++i;
    }
    return result;
}

// ********************************************************************************

PowderPattern read_xrdml( const FileName & file_name )
{
    PowderPattern result;
    TextFileReader_2 text_file_reader( file_name );
    size_t iPos = text_file_reader.find( "<positions axis=\"2Theta\" unit=\"deg\">" );
    if ( iPos == std::string::npos )
        throw std::runtime_error( "PowderPattern::read_xrdml(): 2theta not found." );
    std::string two_theta_start_str = extract_delimited_text( text_file_reader.line( iPos + 1 ), "<startPosition>", "</startPosition>");
    std::string two_theta_end_str   = extract_delimited_text( text_file_reader.line( iPos + 2 ), "<endPosition>", "</endPosition>" );
    Angle two_theta_start = Angle::from_degrees( string2double( two_theta_start_str ) );
    Angle two_theta_end   = Angle::from_degrees( string2double( two_theta_end_str   ) );
    std::vector< std::string > divergence_corrections;
    iPos = text_file_reader.find( "<divergenceCorrections>" );
    if ( iPos != std::string::npos )
        divergence_corrections = split( extract_delimited_text( text_file_reader.line( iPos ), "<divergenceCorrections>", "</divergenceCorrections>" ) );
    std::vector< std::string > counts;
    iPos = text_file_reader.find( "<intensities unit=\"counts\">" );
    if ( iPos != std::string::npos )
        counts = split( extract_delimited_text( text_file_reader.line( iPos ), "<intensities unit=\"counts\">", "</intensities>" ) );
    else
    {
        iPos = text_file_reader.find( "<counts unit=\"counts\">" );
        if ( iPos != std::string::npos )
            counts = split( extract_delimited_text( text_file_reader.line( iPos ), "<counts unit=\"counts\">", "</counts>" ) );
        else
            throw std::runtime_error( "PowderPattern::read_xrdml(): Counts not found." );
    }
    if ( counts.empty() )
        throw std::runtime_error( "PowderPattern::read_xrdml(): no data points." );
    if ( counts.size() == 1 )
        throw std::runtime_error( "PowderPattern::read_xrdml(): only one data point." );
    result.reserve( counts.size() );
    Angle two_theta_step = ( two_theta_end - two_theta_start ) / ( counts.size() - 1 );
    if ( divergence_corrections.empty() )
    {
        for ( size_t i( 0 ); i != counts.size(); ++i )
            result.push_back( ( i * two_theta_step ) + two_theta_start, string2double( counts[i] ) );
    }
    else
    {
        if ( divergence_corrections.size() != counts.size() )
            throw std::runtime_error( "PowderPattern::read_xrdml(): number of counts and number of divergence corrections differ." );
        std::cout << "Note that the .xrdml file contains divergence corrections, which will be applied to the counts." << std::endl;
        for ( size_t i( 0 ); i != counts.size(); ++i )
            result.push_back( ( i * two_theta_step ) + two_theta_start, string2double( divergence_corrections[i] ) * string2double( counts[i] ) );
    }
    return result;
}

// ********************************************************************************

void generate_code( const PowderPattern & powder_pattern, const bool include_estimated_standard_deviation )
{
    std::cout << "    PowderPattern powder_pattern;" << std::endl;
    if ( include_estimated_standard_deviation  )
    {
        for ( size_t i( 0 ); i != powder_pattern.size(); ++i )
            std::cout << "    powder_pattern.push_back( Angle::from_degrees( " << powder_pattern.two_theta( i ) << " ), " << powder_pattern.intensity( i ) << ", " << powder_pattern.estimated_standard_deviation( i ) << " );" << std::endl;
    }
    else
    {
        for ( size_t i( 0 ); i != powder_pattern.size(); ++i )
            std::cout << "    powder_pattern.push_back( Angle::from_degrees( " << powder_pattern.two_theta( i ) << " ), " << powder_pattern.intensity( i ) << " );" << std::endl;
    }
}

// ********************************************************************************

