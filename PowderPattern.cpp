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

#include "PowderPattern.h"
#include "BasicMathsFunctions.h"
#include "FileName.h"
#include "MathsFunctions.h"
#include "RandomNumberGenerator.h"
#include "RunningAverageAndESD.h"
#include "TextFileReader.h"
#include "TextFileWriter.h"
#include "Utilities.h"

// ********************************************************************************

PowderPattern::PowderPattern()
{
}

// ********************************************************************************

PowderPattern::PowderPattern( const Angle two_theta_start, const Angle two_theta_end, const Angle two_theta_step )
{
    size_t npoints = round_to_int( ( ( two_theta_end - two_theta_start ) / two_theta_step ) ) + 1;
    if ( ( ( ( two_theta_start + (npoints-1)*two_theta_step ) - two_theta_end ) / two_theta_step ) > 0.1 )
        std::cout << "PowderPattern::PowderPattern(): Warning: start and end not commensurate with step." << std::endl;
    two_theta_values_.reserve( npoints );
    for ( size_t i( 0 ); i != npoints; ++i )
        two_theta_values_.push_back( ( i * two_theta_step ) + two_theta_start );
    intensities_ = std::vector< double >( npoints, 0.0 );
    estimated_standard_deviations_ = std::vector< double >( npoints, 0.0 );
}

// ********************************************************************************

PowderPattern::PowderPattern( const FileName & file_name )
{
    read_xye( file_name );
}

// ********************************************************************************

void PowderPattern::reserve( const size_t nvalues )
{
    two_theta_values_.reserve( nvalues );
    intensities_.reserve( nvalues );
    estimated_standard_deviations_.reserve( nvalues );
}

// ********************************************************************************

void PowderPattern::push_back( const Angle two_theta, const double intensity )
{
    two_theta_values_.push_back( two_theta );
    intensities_.push_back( intensity );
    estimated_standard_deviations_.push_back( calculate_estimated_standard_deviation( intensity ) );
}

// ********************************************************************************

void PowderPattern::push_back( const Angle two_theta, const double intensity, const double estimated_standard_deviation )
{
    two_theta_values_.push_back( two_theta );
    intensities_.push_back( intensity );
    estimated_standard_deviations_.push_back( estimated_standard_deviation );
}

// ********************************************************************************

void PowderPattern::rebin( const size_t bin_size )
{
    if ( bin_size < 2 )
        return;
    if ( empty() )
        return;
    PowderPattern result;
    RunningAverageAndESD< Angle > current_two_theta( two_theta( 0 ) );
    RunningAverageAndESD< double > current_intensity( intensity( 0 ) );
    double current_ESD( square( estimated_standard_deviation( 0 ) ) );
    for ( size_t i( 1 ); i != size() ; ++ i )
    {
        current_two_theta.add_value( two_theta( i ) );
        current_intensity.add_value( intensity( i ) );
        current_ESD += square( estimated_standard_deviation( i ) );
        if ( ( ( i+1 ) % bin_size ) == 0 )
        {
            result.push_back( current_two_theta.average(), current_intensity.average(), std::sqrt( current_ESD ) );
            current_two_theta.clear();
            current_intensity.clear();
            current_ESD = 0.0;
        }
    }
    if ( current_two_theta.nvalues() != 0 )
        result.push_back( current_two_theta.average(), current_intensity.average(), std::sqrt( current_ESD ) );
    *this = result;
}

// ********************************************************************************

size_t PowderPattern::find_two_theta( const Angle two_theta_value ) const
{
    if ( empty() )
        throw std::runtime_error( "PowderPattern::find_two_theta(): no data points." );
    if ( two_theta_value < two_theta_start() )
        return 0;
    if ( two_theta_value > two_theta_end() )
        return size()-1;
    // Initialise with guess based on uniform 2 theta step.
    size_t lower_index = ( two_theta_value - two_theta_start() ) / average_two_theta_step();
    while ( ( lower_index != 0 ) && ( two_theta( lower_index ) >= two_theta_value ) )
    {
        --lower_index;
    }
    size_t upper_index = lower_index;
    while ( ( upper_index != size()-1 ) && ( two_theta( upper_index ) <= two_theta_value ) )
    {
        ++upper_index;
    }
    lower_index = upper_index;
    while ( ( lower_index != 0 ) && ( two_theta( lower_index ) >= two_theta_value ) )
    {
        --lower_index;
    }
    for ( size_t i( lower_index ); i != upper_index+1; ++i )
    {
        if ( nearly_equal( two_theta( i ), two_theta_value ) )
            return i;
    }
    if ( ( upper_index - lower_index ) != 1 )
        throw std::runtime_error( "PowderPattern::find_two_theta(): programming error." );
    if ( ( two_theta_value - two_theta( lower_index ) ) < ( two_theta( upper_index ) - two_theta_value ) )
        return lower_index;
    else
        return upper_index;
}

// ********************************************************************************

// Multiplies intensities and ESDs by factor.
void PowderPattern::scale( const double factor )
{
    for ( size_t i( 0 ); i != size(); ++i )
    {
        intensities_[i] *= factor;
        estimated_standard_deviations_[i] *= factor;
    }
}

// ********************************************************************************

Angle PowderPattern::two_theta( const size_t i ) const
{
    if ( i < size() )
        return two_theta_values_[i];
    throw std::runtime_error( "PowderPattern::two_theta(): index out of bounds." );
}

// ********************************************************************************

double PowderPattern::intensity( const size_t i ) const
{
    if ( i < size() )
        return intensities_[i];
    throw std::runtime_error( "PowderPattern::intensity(): index out of bounds." );
}

// ********************************************************************************

double PowderPattern::estimated_standard_deviation( const size_t i ) const
{
    if ( i < size() )
        return estimated_standard_deviations_[i];
    throw std::runtime_error( "PowderPattern::estimated_standard_deviation(): index out of bounds." );
}

// ********************************************************************************

void PowderPattern::set_two_theta( const size_t i, const Angle value )
{
    if ( i < size() )
        two_theta_values_[i] = value;
    else
        throw std::runtime_error( "PowderPattern::set_two_theta(): index out of bounds." );
}

// ********************************************************************************

// ESD is NOT updated.
void PowderPattern::set_intensity( const size_t i, const double value )
{
    if ( i < size() )
        intensities_[i] = value;
    else
        throw std::runtime_error( "PowderPattern::set_intensity(): index out of bounds." );
}

// ********************************************************************************

void PowderPattern::set_estimated_standard_deviation( const size_t i, const double value )
{
    if ( i < size() )
        estimated_standard_deviations_[i] = value;
    else
        throw std::runtime_error( "PowderPattern::set_estimated_standard_deviation(): index out of bounds." );
}

// ********************************************************************************

Angle PowderPattern::average_two_theta_step() const
{
    if ( empty() )
        throw std::runtime_error( "PowderPattern::average_two_theta_step(): no data points." );
    if ( size() == 1 )
        throw std::runtime_error( "PowderPattern::average_two_theta_step(): only one data point." );
    return ( ( two_theta( size()-1 ) - two_theta( 0 ) ) / ( size() - 1 ) );
}

// ********************************************************************************

Angle PowderPattern::two_theta_start() const
{
    if ( empty() )
        throw std::runtime_error( "PowderPattern::two_theta_start(): no data points." );
    return two_theta_values_[ 0 ];
}

// ********************************************************************************

Angle PowderPattern::two_theta_end() const
{
    if ( empty() )
        throw std::runtime_error( "PowderPattern::two_theta_end(): no data points." );
    return two_theta_values_[ size()-1 ];
}

// ********************************************************************************

// Uses average_two_theta_step() to add new points. Intensities and ESDs are initialised to 0.0.
void PowderPattern::set_two_theta_start( const Angle two_theta_start )
{
    if ( empty() )
        throw std::runtime_error( "PowderPattern::set_two_theta_start(): no data points." );
    if ( ( two_theta_values_[ 0 ] - 0.5*average_two_theta_step() ) > two_theta_start )
        throw std::runtime_error( "PowderPattern::set_two_theta_start(): adding data points not implemented yet." );
}

// ********************************************************************************

// Uses average_two_theta_step() to add new points. Intensities and ESDs are initialised to 0.0.
void PowderPattern::set_two_theta_end( const Angle two_theta_end )
{
    if ( empty() )
        throw std::runtime_error( "PowderPattern::set_two_theta_end(): no data points." );
    if ( ( two_theta_values_[ size()-1 ] + 0.5*average_two_theta_step() ) < two_theta_end )
        throw std::runtime_error( "PowderPattern::set_two_theta_end(): adding data points not implemented yet." );
}

// ********************************************************************************

void PowderPattern::reduce_range_to( const Angle two_theta_start, const Angle two_theta_end )
{
    size_t iStart = find_two_theta( two_theta_start );
    size_t iEnd = find_two_theta( two_theta_end );
    if ( iStart != 0 )
    {
        for ( size_t i( 0 ); i != iEnd + 1 - iStart; ++i )
        {
            two_theta_values_[ i ] = two_theta_values_[ i + iStart ];
            intensities_[ i ] = intensities_[ i + iStart ];
            estimated_standard_deviations_[ i ] = estimated_standard_deviations_[ i + iStart ];
        }
    }
    two_theta_values_.resize( iEnd - iStart );
    intensities_.resize( iEnd - iStart );
    estimated_standard_deviations_.resize( iEnd - iStart );
}

// ********************************************************************************

double PowderPattern::cumulative_intensity() const
{
    return add_doubles( intensities_ );
}

// ********************************************************************************

double PowderPattern::cumulative_intensity( const Angle two_theta_start, const Angle two_theta_end ) const
{
    size_t iStart = find_two_theta( two_theta_start );
    size_t iEnd = find_two_theta( two_theta_end );
    double result( 0.0 );
    for ( size_t i( iStart ); i != iEnd + 1 ; ++i )
        result += intensities_[i];
    return result;
}

// ********************************************************************************

void PowderPattern::read_xye( const FileName & file_name )
{
    *this = PowderPattern();
    TextFileReader text_file_reader( file_name );
    std::vector< std::string > words;
    // The first line could contain the wavelength.
    if ( text_file_reader.get_next_line( words ) )
    {
        if ( words.size() == 1 )
            wavelength_ = Wavelength::determine_from_wavelength( string2double( words[0] ) );
        else
            text_file_reader.push_back_last_line();
    }
    else
        return;
    while ( text_file_reader.get_next_line( words ) )
    {
        if ( ( words.size() < 2 ) || ( words.size() > 3 ) )
            throw std::runtime_error( "PowderPattern::read(): cannot interpret line \"" + text_file_reader.get_line() + "\"" );
        two_theta_values_.push_back( Angle( string2double( words[0] ), Angle::DEGREES ) );
        intensities_.push_back( string2double( words[1] ) );
        if ( words.size() == 2 )
            estimated_standard_deviations_.push_back( sqrt( intensities_[two_theta_values_.size()-1] ) );
        else
            estimated_standard_deviations_.push_back( string2double( words[2] ) );
    }
}

// ********************************************************************************

void PowderPattern::save_xye( const FileName & file_name, const bool include_wave_length ) const
{
    TextFileWriter text_file_writer( file_name );
    if ( include_wave_length )
        text_file_writer.write_line( double2string( wavelength_.wavelength_1() ) );
    for ( size_t i( 0 ); i != size(); ++i )
        text_file_writer.write_line( double2string( two_theta_values_[i].value_in_degrees(), 5 ) + "  " + double2string( intensities_[i] ) + "  " + double2string( estimated_standard_deviations_[i] ) );
}

// ********************************************************************************

// We assume a uniform step size.
PowderPattern & PowderPattern::operator+=( const PowderPattern & rhs )
{
    if ( ! same_range( *this, rhs ) )
        throw std::runtime_error( "PowderPattern::operator+=( const PowderPattern & ): ranges not same." );
    for ( size_t i( 0 ); i != size(); ++i )
        intensities_[i] += rhs.intensities_[i];
    return *this;
}

// ********************************************************************************

// We assume a uniform step size.
PowderPattern & PowderPattern::operator-=( const PowderPattern & rhs )
{
    if ( ! same_range( *this, rhs ) )
        throw std::runtime_error( "PowderPattern::operator-=( const PowderPattern & ): ranges not same." );
    for ( size_t i( 0 ); i != size(); ++i )
        intensities_[i] -= rhs.intensities_[i];
    return *this;
}

// ********************************************************************************

double PowderPattern::normalise_highest_peak( const double highest_peak )
{
    // Find the highest intensity.
    double max_intensity = calculate_maximum( intensities_ );
    if ( nearly_zero( max_intensity ) )
        throw std::runtime_error( "PowderPattern::normalise_highest_peak(): highest peak is 0.0." );
    double scale_factor = highest_peak / max_intensity;
    scale( scale_factor );
    return scale_factor;
}

// ********************************************************************************

// Normalises the total signal = area under the pattern = cumulative_intensity() .
double PowderPattern::normalise_total_signal( const double total_signal )
{
    double current_total_signal = cumulative_intensity();
    if ( nearly_zero( current_total_signal ) )
        throw std::runtime_error( "PowderPattern::normalise_total_signal(): total signal is 0.0." );
    // Scale to total_signal.
    double scale_factor = total_signal / current_total_signal;
    scale( scale_factor );
    return scale_factor;
}

// ********************************************************************************

void PowderPattern::correct_zero_point_error( const Angle two_theta_value )
{
    for ( size_t i( 0 ); i != size(); ++i )
        two_theta_values_[i] -= two_theta_value;
}

// ********************************************************************************

void PowderPattern::recalculate_estimated_standard_deviations()
{
    for ( size_t i( 0 ); i != size(); ++i )
        estimated_standard_deviations_[i] = calculate_estimated_standard_deviation( intensities_[i] );
}

// ********************************************************************************

// I(fixed slit) = I(variable slit) / sin(theta).
void PowderPattern::convert_to_fixed_slit()
{
    for ( size_t i( 0 ); i != size(); ++i )
    {
        intensities_[i] /= ( two_theta_values_[i] / 2.0 ).sine(); // @@ We should check for divide by zero
        estimated_standard_deviations_[i] /= ( two_theta_values_[i] / 2.0 ).sine();
    }
}

// ********************************************************************************

// I(fixed slit) = I(variable slit) / sin(theta).
void PowderPattern::convert_to_variable_slit()
{
    for ( size_t i( 0 ); i != size(); ++i )
    {
        intensities_[i] *= ( two_theta_values_[i] / 2.0 ).sine();
        estimated_standard_deviations_[i] *= ( two_theta_values_[i] / 2.0 ).sine();
    }
}

// ********************************************************************************

void PowderPattern::add_constant_background( const double background )
{
    for ( size_t i( 0 ); i != size(); ++i )
        intensities_[i] += background;
}

// ********************************************************************************

void PowderPattern::add_Poisson_noise()
{
    for ( size_t i( 0 ); i != size(); ++i )
        intensities_[i] = Poisson_distribution( round_to_int( intensities_[i] ) );
}

// ********************************************************************************

void PowderPattern::add_Poisson_noise_including_zero( const size_t threshold )
{
    for ( size_t i( 0 ); i != size(); ++i )
    {
        if ( intensities_[i] < threshold )
        {
            int intensity = round_to_int( intensities_[i] ) + threshold;
            intensity = Poisson_distribution( intensity ) - static_cast<int>(threshold);
            intensities_[i] = std::abs( intensity );
        }
        else
            intensities_[i] = Poisson_distribution( round_to_int( intensities_[i] ) );
    }
}

// ********************************************************************************

void PowderPattern::make_counts_integer()
{
    for ( size_t i( 0 ); i != size(); ++i )
        intensities_[i] = round_to_int( intensities_[i] );
}

// ********************************************************************************

// Should not be necessary. Introduced to manipulate data from a tool that extracted a powder pattern from a bitmap picture.
void PowderPattern::sort_two_theta()
{
    if ( size() < 2 )
        return;
    bool changed( true );
    while ( changed )
    {
        changed = false;
        for ( size_t i( size()-1 ); i != 0; --i )
        {
            if ( two_theta_values_[i] < two_theta_values_[i-1] )
            {
                // @@ The following code strongly suggests that we should have used a struct to hold each triplet of values...
                std::swap( two_theta_values_[i], two_theta_values_[i-1] );
                std::swap( intensities_[i], intensities_[i-1] );
                std::swap( estimated_standard_deviations_[i], estimated_standard_deviations_[i-1] );
                changed = true;
            }
        }
    }
}

// ********************************************************************************

// Should not be necessary. Introduced to manipulate data from a tool that extracted a powder pattern from a bitmap picture.
void PowderPattern::average_if_two_theta_equal()
{
    std::vector< Angle > new_two_theta_values;
    std::vector< double > new_intensities;
    std::vector< double > new_estimated_standard_deviations;
    size_t iPos1 = 0;
    while ( iPos1 < size() )
    {
        Angle sum_two_theta_values( two_theta_values_[iPos1] );
        double sum_intensity( intensities_[iPos1] );
        double sum_estimated_standard_deviation_2( square( estimated_standard_deviations_[iPos1] ) );
        size_t iPos2 = iPos1 + 1;
        while ( ( iPos2 < size() ) && nearly_equal( two_theta_values_[iPos1], two_theta_values_[iPos2] ) )
        {
            sum_two_theta_values += two_theta_values_[iPos2];
            sum_intensity += intensities_[iPos2];
            sum_estimated_standard_deviation_2 += square( estimated_standard_deviations_[iPos2] );
            ++iPos2;
        }
        new_two_theta_values.push_back( sum_two_theta_values / ( iPos2 - iPos1 ) );
        new_intensities.push_back( sum_intensity / ( iPos2 - iPos1 ) );
        new_estimated_standard_deviations.push_back( sqrt( sum_estimated_standard_deviation_2 ) );
        iPos1 = iPos2;
    }
    two_theta_values_ = new_two_theta_values;
    intensities_ = new_intensities;
    estimated_standard_deviations_ = new_estimated_standard_deviations;
}

// ********************************************************************************

// ESD is std::max( sqrt( intensity ), intensity / 100.0 ), or 4.4 if intensity < 20.
double calculate_estimated_standard_deviation( const double intensity )
{
    if ( intensity < 20.0 )
        return 4.4;
    if ( intensity > 10000.0 )
        return intensity / 100.0;
    return sqrt( intensity );
}

// ********************************************************************************

// Check that the two patterns have the same range and 2theta step.
bool same_range( const PowderPattern & lhs, const PowderPattern & rhs )
{
    if ( lhs.size() != rhs.size() )
        return false;
    if ( lhs.size() == 0 )
        return true;
    return ( nearly_equal( lhs.two_theta( 0 )           , rhs.two_theta( 0 ) ) &&
             nearly_equal( lhs.two_theta( lhs.size()-1 ), rhs.two_theta( rhs.size()-1 ) ) );
}

// ********************************************************************************

double weighted_cross_correlation( const PowderPattern & lhs, const PowderPattern & rhs, Angle l )
{
    if ( l < Angle() )
        throw std::runtime_error( "weighted_cross_correlation( PowderPattern, PowderPattern, Angle ): l must be non-negative." );
    int m = round_to_int( l / lhs.average_two_theta_step() );
    if ( m == 0 )
        m = 1;
    double result( 0.0 );
    for ( int i( 0 ); i != lhs.size(); ++i )
    {
        for ( int j( -m + 1 ); j != m; ++j )
        {
            if ( ( ( i + j ) >= 0 ) && ( ( i + j ) < lhs.size() ) )
            {
                double w = 1.0 - absolute( j ) / static_cast<double>( m );
                if ( (true) )
                    result += w * lhs.intensity( i ) * rhs.intensity( i + j );
                else
                    result += w * ( lhs.intensity( i ) / lhs.estimated_standard_deviation( i ) ) * ( rhs.intensity( i + j ) / rhs.estimated_standard_deviation( i + j ) );
            }
        }
    }
    return result;
}

// ********************************************************************************

double normalised_weighted_cross_correlation( const PowderPattern & lhs, const PowderPattern & rhs, Angle l )
{
    if ( ! same_range( lhs, rhs ) )
        throw std::runtime_error( "normalised_weighted_cross_correlation( const PowderPattern &, const PowderPattern & ): ranges not same." );
    return weighted_cross_correlation( lhs, rhs, l ) / sqrt( weighted_cross_correlation( lhs, lhs, l ) * weighted_cross_correlation( rhs, rhs, l ) );
}

// ********************************************************************************

double Rwp( const PowderPattern & lhs, const PowderPattern & rhs )
{
    double numerator( 0.0 );
    double denominator( 0.0 );
    for ( size_t i( 0 ); i != lhs.size(); ++i )
    {
        numerator   += square( lhs.intensity( i ) - rhs.intensity( i ) ) / square( lhs.estimated_standard_deviation( i ) );
        denominator += square( lhs.intensity( i ) ) / square( lhs.estimated_standard_deviation( i ) );
    }
    return sqrt( numerator / denominator );
}

// ********************************************************************************

PowderPattern calculate_Brueckner_background( const PowderPattern & powder_pattern,
                                              const size_t niterations,
                                              const size_t window,
                                              const bool apply_smoothing,
                                              const size_t smoothing_window )
{
    if ( powder_pattern.empty() )
        return powder_pattern;
    PowderPattern result( powder_pattern );
    size_t size( powder_pattern.size() );
    if ( apply_smoothing )
    {
        for ( size_t i( 0 ); i < size; ++i )
        {
            double new_value = powder_pattern.intensity( i );
            for ( size_t j( 1 ); j <= smoothing_window; ++j )
            {
                new_value += powder_pattern.intensity( std::max( int(i)-int(j), int(0) ) );
                new_value += powder_pattern.intensity( std::min( i+j, size-1 ) );
            }
            result.set_intensity( i, new_value / ( 2.0 * smoothing_window + 1.0 ) );
        }
    }
    if ( true )
    {
        RunningAverageAndESD< double > I_average;
        double I_minimum = result.intensity( 0 );
        for ( size_t i( 0 ); i < size; ++i )
        {
            if ( result.intensity( i ) < I_minimum )
                I_minimum = result.intensity( i );
            I_average.add_value( result.intensity( i ) );
        }
        for ( size_t i( 0 ); i < size; ++i )
        {
            if ( result.intensity( i ) > ( I_average.average() + 2.0 * ( I_average.average() - I_minimum ) ) )
                result.set_intensity( i, ( I_average.average() + 2.0 * ( I_average.average() - I_minimum ) ) );
        }
    }
    for ( size_t iter( 0 ); iter < niterations; ++iter )
    {
        PowderPattern pp_old = result;
        for ( size_t i( 0 ); i < size; ++i )
        {
            double average_value( 0.0 );
            for ( size_t j( 1 ); j <= window; ++j )
            {
                average_value += pp_old.intensity( std::max( int(i)-int(j), int(0) ) );
                average_value += pp_old.intensity( std::min( i+j, size-1 ) );
            }
            average_value /= 2.0 * window;
            result.set_intensity( i, average_value );
        }
        for ( size_t i( 0 ); i < size; ++i )
            result.set_intensity( i, std::min( pp_old.intensity( i ), result.intensity( i ) ) );
    }
    return result;
}

// ********************************************************************************

PowderPattern calculate_Poisson_noise( const PowderPattern & powder_pattern )
{
    PowderPattern result;
    result.set_wavelength( powder_pattern.wavelength() );
    result.reserve( powder_pattern.size() );
    for ( size_t i( 0 ); i != powder_pattern.size(); ++i )
        result.push_back( powder_pattern.two_theta( i ), Poisson_distribution( round_to_int( powder_pattern.intensity( i ) ) ) - powder_pattern.intensity( i ), 0.0 );
    return result;
}

// ********************************************************************************

// If the number of counts is less than threshold, adds threshold, then calculates the Poisson noise,
// then subtracts the threshold, then makes the remainder positive.
// If the maximum is scaled to be 10,000 counts, a good threshold value is 20.
PowderPattern calculate_Poisson_noise_including_zero( const PowderPattern & powder_pattern, const size_t threshold )
{
    PowderPattern result;
    result.set_wavelength( powder_pattern.wavelength() );
    result.reserve( powder_pattern.size() );
    for ( size_t i( 0 ); i != powder_pattern.size(); ++i )
    {
        int old_intensity = powder_pattern.intensity( i );
        int new_intensity;
        if ( old_intensity < threshold )
            new_intensity = std::abs( Poisson_distribution( old_intensity + threshold ) - static_cast<double>(threshold) );
        else
            new_intensity = Poisson_distribution( old_intensity );
        result.push_back( powder_pattern.two_theta( i ), new_intensity - old_intensity, 0.0 );
    }
    return result;
}

// ********************************************************************************

PowderPattern add_powder_patterns( const std::vector< PowderPattern > & powder_patterns, const std::vector< double > & noscp2ts )
{
    if ( powder_patterns.empty() )
        throw std::runtime_error( "add_powder_patterns(): Error: no powder patterns provided." );
    if ( powder_patterns.size() != noscp2ts.size() )
        throw std::runtime_error( "add_powder_patterns(): Error: powder_patterns and noscp2ts not the same size." );
    // Check that they have the same wavelength and average_two_theta_step.
    for ( size_t i( 1 ); i != powder_patterns.size(); ++i )
    {
        if ( ! nearly_equal( powder_patterns[0].wavelength(), powder_patterns[i].wavelength() ) )
            throw std::runtime_error( "add_powder_patterns(): Error: wavelengths not the same." );
        if ( ! nearly_equal( powder_patterns[0].average_two_theta_step(), powder_patterns[i].average_two_theta_step() ) )
            throw std::runtime_error( "add_powder_patterns(): Error: average_two_theta_step not the same." );
    }
    // Find the smallest and largest 2theta values.
    Angle two_theta_min = powder_patterns[0].two_theta_start();
    Angle two_theta_max = powder_patterns[0].two_theta_end();
    for ( size_t i( 1 ); i != powder_patterns.size(); ++i )
    {
        if ( powder_patterns[i].two_theta_start() < two_theta_min )
            two_theta_min = powder_patterns[i].two_theta_start();
        if ( powder_patterns[i].two_theta_end() > two_theta_max )
            two_theta_max = powder_patterns[i].two_theta_end();
    }
    Angle two_theta_step = powder_patterns[0].average_two_theta_step();
    PowderPattern result( two_theta_min, two_theta_max, two_theta_step );
    for ( size_t j( 0 ); j != result.size(); ++j )
    {
        bool at_least_one_contribution( false );
        double sum_of_intensities( 0.0 );
        double sum_of_noscp2ts( 0.0 );
        for ( size_t i( 0 ); i != powder_patterns.size(); ++i )
        {
            // @@ The following is wrong because "result" includes the min and max 2theta, which may not exist in the current pattern.
            size_t index = powder_patterns[i].find_two_theta( result.two_theta( j ) );
            if ( absolute( powder_patterns[i].two_theta( index ) - result.two_theta( j ) ) < ( two_theta_step / 2.0 ) )
            {
                at_least_one_contribution = true;
                sum_of_intensities += powder_patterns[i].intensity( index );
                sum_of_noscp2ts += noscp2ts[i];
            }
        }
        if ( at_least_one_contribution )
        {
            result.set_intensity( j, sum_of_intensities / sum_of_noscp2ts );
            result.set_estimated_standard_deviation( j, std::max( sqrt( sum_of_intensities ), sum_of_intensities / 100.0 ) / sum_of_noscp2ts );
        }
        else
            std::cout << "add_powder_patterns(): Warning, no contribution." << std::endl;
    }
    return result;
}

// ********************************************************************************

std::vector< PowderPattern > split( const PowderPattern & powder_pattern, const size_t n, const bool recalculate_ESDs )
{
    std::vector< PowderPattern > result;
    for ( size_t j( 0 ); j != n; ++j )
    {
        result.push_back( PowderPattern() );
        result[j].set_wavelength( powder_pattern.wavelength() );
    }
    RandomNumberGenerator_integer rng;
    for ( size_t i( 0 ); i != powder_pattern.size(); ++i )
    {
        int old_intensity_int = round_to_int( powder_pattern.intensity( i ) );
        if ( old_intensity_int < 0 )
            throw std::runtime_error( "split(PowderPattern): Error: intensity is negative." );
        size_t old_intensity = old_intensity_int;
        double old_ESD = powder_pattern.estimated_standard_deviation( i ) / n;
        size_t target_average = round_to_int( powder_pattern.intensity( i ) / n );
        // If round_to_int( powder_pattern.intensity( i ) / n ) == 0 then Poisson_distribution( target_average ); returns 0.
        std::vector< size_t > intensities;
        size_t sum( 0 );
        if ( target_average == 0 )
        {
            if ( old_intensity > ( n / 2 ) )
            {
                sum = n;
                for ( size_t j( 0 ); j != n; ++j )
                    intensities.push_back( 1 );
            }
            else
            {
                for ( size_t j( 0 ); j != n; ++j )
                    intensities.push_back( 0 );
            }
        }
        else
        {
            for ( size_t j( 0 ); j != n; ++j )
            {
                size_t new_intensity = Poisson_distribution( target_average );
                sum += new_intensity;
                intensities.push_back( new_intensity );
            }
            if ( sum != 0 )
            {
                // We scale all intensities.
                double scale_factor = powder_pattern.intensity( i ) / sum;
                sum = 0;
                for ( size_t j( 0 ); j != n; ++j )
                {
                    intensities[j] = round_to_int( intensities[j] * scale_factor );
                    sum += intensities[j];
                }
            }
        }
        if ( sum != old_intensity )
        {
            // We randomly add / remove counts until we have reached the target.
            int step = ( sum > old_intensity ) ? -1 : 1;
            while ( sum != old_intensity )
            {
                // Randomly pick a pattern.
                size_t j = rng.next_number( 0, n-1 );
                if ( ! ( ( step == -1 ) && ( intensities[j] == 0 ) ) )
                {
                    intensities[j] += step;
                    sum += step;
                }
            }
        }
        for ( size_t j( 0 ); j != n; ++j )
        {
            if ( recalculate_ESDs )
                result[j].push_back( powder_pattern.two_theta( i ), intensities[j] );
            else
                result[j].push_back( powder_pattern.two_theta( i ), intensities[j], old_ESD );
        }
    }
    return result;
}

// ********************************************************************************

