/* *********************************************
Copyright (c) 2013-2025, Cornelis Jan (Jacco) van de Streek
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

#include "SudokuSolver.h"
#include "BasicMathsFunctions.h"
#include "GenerateCombinations.h"
#include "OneSudokuSlice.h"
#include "SetOfNumbers.h"
#include "Stack.h"
#include "Sudoku.h"
#include "TransSquareDependency.h"

#include <algorithm>
#include <stdexcept>
#include <iostream>
#include <string>

namespace {

static const size_t col_white_squares_indices[20] = {  1,  3,  5,  7, 19, 21, 23, 25, 37, 39, 41, 43, 55, 57, 59, 61, 73, 75, 77, 79 };
static const size_t row_white_squares_indices[20] = {  9, 11, 13, 15, 17, 27, 29, 31, 33, 35, 45, 47, 49, 51, 53, 63, 65, 67, 69, 71 };

static const size_t dark_blue_squares_indices[25] = {  0,  2,  4,  6,  8, 18, 20, 22, 24, 26, 36, 38, 40, 42, 44, 54, 56, 58, 60, 62, 72, 74, 76, 78, 80 };
static const size_t lite_blue_squares_indices[16] = { 10, 12, 14, 16, 28, 30, 32, 34, 46, 48, 50, 52, 64, 66, 68, 70 };

// ********************************************************************************

bool contains( const std::vector< size_t > & values, const size_t target )
{
    for ( size_t i( 0 ); i != values.size(); ++i )
    {
        if ( values[i] == target )
            return true;
    }
    return false;
}

// ********************************************************************************

// Returns true if a change was made.
bool check_if_we_are_the_only_possibility( OneSudokuSlice & slice )
{
    bool something_changed_on_first_pass( false );
 //   bool something_changed( true );
 //   while ( something_changed ) // @@ Using this causes errors, I do not understand why
    {
  //      something_changed = false;
        
        // If there are N (or more) numbers with a frequency of N, and they are in the same N squares, regardless of what else is in those squares,
        // those N numbers cannot be anywhere else, and those N squares can only contain those N numbers. E.g. | 12 | 129 | with no other squares with 1 or 2.
        // This also includes the case where frequency is 1.

        for ( size_t N( 1 ); N != 8; ++N )
        {
            std::vector< size_t > unsolved_squares_indices = slice.indices_of_unsolved_squares();
            if ( static_cast<int>( N ) > ( static_cast<int>( unsolved_squares_indices.size() ) - 1 ) )
                break;
                
            // For each number present in an unsolved square, count how many occurrences there are.
            SetOfNumbers values;
            for ( size_t i( 0 ); i != unsolved_squares_indices.size(); ++i )
            {
                OneSudokuSquare square = slice.square( unsolved_squares_indices[i] );
                values.add( square.values() );
            }
            std::vector< size_t > frequency_is_N;
            for ( size_t i( 1 ); i != 10; ++i )
            {
                if ( values.frequency( i ) == N )
                    frequency_is_N.push_back( i );
            }
            if ( ! ( frequency_is_N.size() < N ) )
            {
                // The fastest is actually to just take all k-in-N and go through all of them and reject the ones that do not fit.
                // There will be very few, and trying to generate exactly those few is going to be a nightmare.
                GenerateCombinations generate_combinations( unsolved_squares_indices, N );
                std::vector< size_t > combination;
                while ( generate_combinations.next_combination( combination ) )
                {
                    GenerateCombinations generate_combinations_2( frequency_is_N, N );
                    std::vector< size_t > combination_2;
                    while ( generate_combinations_2.next_combination( combination_2 ) )
                    {
                        // Do all of the squares have all of the numbers?
                        bool accept( true );
                        for ( size_t i( 0 ); i != combination.size(); ++i )
                        {
                            for ( size_t j( 0 ); j != combination_2.size(); ++j )
                            {
                                if ( ! slice.square( combination[i] ).contains( combination_2[j] ) )
                                    accept = false;
                            }
                        }
                        if ( accept )
                        {
                            SetOfNumbers set_of_numbers( combination_2 );
                            OneSudokuSquare isolated_values( set_of_numbers );
                            // Find the squares.
                            for ( size_t i( 0 ); i != unsolved_squares_indices.size(); ++i )
                            {
                                OneSudokuSquare square = slice.square( unsolved_squares_indices[i] );
                                if ( square.contains( combination_2[0] ) )
                                {
                                    if ( ! ( square == isolated_values ) )
                                    {
                                        slice.set_square( unsolved_squares_indices[i], isolated_values );
                                        something_changed_on_first_pass = true;
                        //                something_changed = true;
            //                            std::cout << "2. We did something for N = " << N << ", slice ID = " << slice.id() << ", values are ";
            //                            isolated_values.show();
            //                            std::cout << std::endl;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        for ( size_t N( 2 ); N != 8; ++N )
        {
            // If N squares contain exactly N numbers, those N numbers cannot be anywhere else. Their frequencies are irrelevant.
            // e.g. | 459 | 45 | 45 | (two squares block 45) or | 12 | 19 | 29 | 14 | (three squares block 129).
            // N == 1 never passes the merged.size() == N test so it is essentially ignored.
            std::vector< size_t > unsolved_squares_indices = slice.indices_of_unsolved_squares();
            if ( static_cast<int>( N ) > ( static_cast<int>( unsolved_squares_indices.size() ) - 1 ) )
                break;
            // Add the unknowns from any combination of N squares and check if there are N unknowns.
            GenerateCombinations generate_combinations( unsolved_squares_indices, N );
            std::vector< size_t > combination;
            while ( generate_combinations.next_combination( combination ) )
            {
                OneSudokuSquare merged( slice.square( combination[0] ) );
                for ( size_t i( 1 ); i != combination.size(); ++i )
                    merged = merge( merged, slice.square( combination[i] ) );
                if ( merged.size() == N )
                {
                    // Unset these values in the other squares.
                    for ( size_t i( 0 ); i != slice.size(); ++i )
                    {
                        if ( contains( combination, i ) )
                            continue;
                        OneSudokuSquare square = slice.square( i );
                        if ( square.solved() )
                            continue;
                        if ( square.unset( merged.values() ) )
                        {
                            slice.set_square( i, square );
                            something_changed_on_first_pass = true;
                  //          something_changed = true;
        //                    std::cout << "3. We did something for N = " << N << ", slice ID = " << slice.id() << ", values are ";
        //                    merged.show();
        //                    std::cout << std::endl;
                       //     std::cout << "Changed square = " << i << std::endl;
                        }
                    }
                }
            }
        }
    }
    return something_changed_on_first_pass;
}

// ********************************************************************************

/*

| D | D | D |   |   |   |   |   |   |
| D | D | D |   |   |   |   |   |   |
|   |   |   | C | C | C | C | C | C |

Values not possible in the squares marked D, are not allowed in the squares marked C.

So we take all the possible values in the squares marked D, then take the set 1 through 9 minus those values
and eliminate those values from the squares marked C.

*/

bool holistic( Sudoku & sudoku )
{
    bool something_changed_on_first_pass( false );
    bool a_change_was_made( true );
    while ( a_change_was_made )
    {
        a_change_was_made = false;
        for ( size_t i( 0 ); i != sudoku.number_of_trans_square_dependencies(); ++i )
        {
            TransSquareDependency tsd = sudoku.next_trans_square_dependency();
            OneSudokuSlice determining_values = tsd.determining_values();
            SetOfNumbers all_possible_numbers = determining_values.collect_all_possible_numbers();
            if ( all_possible_numbers.size() != 9 )
            {
                // Invert the selection.
				SetOfNumbers square( 1, 9, SetOfNumbers::THROW ); // Initialises to all 9 possibilities.
				square.set_empty_is_allowed( false );
				square.remove( all_possible_numbers );
                OneSudokuSlice values_to_be_changed = tsd.values_to_be_changed();
                bool there_was_a_change( false );
                for ( size_t j( 0 ); j != values_to_be_changed.size(); ++j )
                {
                    OneSudokuSquare square_2 = values_to_be_changed.square( j );
                    if ( square_2.unset( square ) )
                    {
                        values_to_be_changed.set_square( j, square_2 );
                        something_changed_on_first_pass = true;
                        a_change_was_made = true;
                        there_was_a_change = true;
                    }
                }
                if ( there_was_a_change )
                {
                    tsd.update_values_to_be_changed( values_to_be_changed );
                    sudoku.update_trans_square_dependency( tsd );
                }
            }
        }
    }
    return something_changed_on_first_pass;
}

// ********************************************************************************

bool apply_X_Wings_on_rows( Sudoku & sudoku, const size_t iRow_1, const size_t iRow_2 )
{
    bool something_changed_at_all( false );
    for ( size_t row_1( iRow_1 ); row_1 != iRow_1+3; ++row_1 )
    {
        OneSudokuSlice slice_row_1 = sudoku.row( row_1 );
        std::vector< size_t > unsolved_squares_indices = slice_row_1.indices_of_unsolved_squares();
        if ( unsolved_squares_indices.size() < 2 )
            continue;
        SetOfNumbers frequency_is_2_in_row_1( SetOfNumbers::THROW );
        { // Scoping brackets
            // For each number present in an unsolved square, count how many occurrences there are.
            SetOfNumbers values;
            for ( size_t i( 0 ); i != unsolved_squares_indices.size(); ++i )
            {
                OneSudokuSquare square = slice_row_1.square( unsolved_squares_indices[i] );
                values.add( square.values() );
            }
            for ( size_t i( 1 ); i != 10; ++i )
            {
                if ( values.frequency( i ) == 2 )
                    frequency_is_2_in_row_1.add( i );
            }
        } // Scoping brackets
        if ( frequency_is_2_in_row_1.empty()  )
            continue;
        for ( size_t row_2( iRow_2 ); row_2 != iRow_2+3; ++row_2 )
        {
            OneSudokuSlice slice_row_2 = sudoku.row( row_2 );
            unsolved_squares_indices = slice_row_2.indices_of_unsolved_squares();
            if ( unsolved_squares_indices.size() < 2 )
                continue;
            SetOfNumbers frequency_is_2_in_row_2( SetOfNumbers::THROW );
            { // Scoping brackets
                // For each number present in an unsolved square, count how many occurrences there are.
                SetOfNumbers values;
                for ( size_t i( 0 ); i != unsolved_squares_indices.size(); ++i )
                {
                    OneSudokuSquare square = slice_row_2.square( unsolved_squares_indices[i] );
                    values.add( square.values() );
                }
                for ( size_t i( 1 ); i != 10; ++i )
                {
                    if ( values.frequency( i ) == 2 )
                        frequency_is_2_in_row_2.add( i );
                }
            } // Scoping brackets
            if ( frequency_is_2_in_row_2.empty()  )
                continue;
            // When we are here, both row 1 and row 2 have at least one number that occurs in exactly two unsolved squares.
            // First, check if they have any numbers in common.
            SetOfNumbers intersection = frequency_is_2_in_row_1.in_common( frequency_is_2_in_row_2 );
            // Second, the numbers must occur in the same two columns.
            for ( size_t i( 0 ); i != intersection.size(); ++i )
            {
                size_t x_wing_value = intersection.value( i );
                size_t row_1_col_1( 9 );
                size_t row_1_col_2( 9 );
                size_t row_2_col_1( 9 );
                size_t row_2_col_2( 9 );
                for ( size_t j( 0 ); j != slice_row_1.size(); ++j )
                {
                    OneSudokuSquare square = slice_row_1.square( j );
                    if ( square.contains( x_wing_value ) )
                    {
                        if ( row_1_col_1 == 9 )
                            row_1_col_1 = j;
                        else
                        {
                            row_1_col_2 = j;
                            break;
                        }
                    }
                }
                for ( size_t j( 0 ); j != slice_row_2.size(); ++j )
                {
                    OneSudokuSquare square = slice_row_2.square( j );
                    if ( square.contains( x_wing_value ) )
                    {
                        if ( row_2_col_1 == 9 )
                            row_2_col_1 = j;
                        else
                        {
                            row_2_col_2 = j;
                            break;
                        }
                    }
                }
                if ( row_1_col_1 != row_2_col_1 )
                    continue;
                if ( row_1_col_2 != row_2_col_2 )
                    continue;
                // We can now erase x_wing_value in those two columns everywhere except in rows row_1 and row_2.
                OneSudokuSlice slice_col_1 = sudoku.column( row_1_col_1 );
                bool a_change_was_made( false );
                for ( size_t j( 0 ); j != slice_col_1.size(); ++j )
                {
                    OneSudokuSquare square = slice_col_1.square( j );
                    if ( ( j == row_1 ) || ( j == row_2 ) )
                        continue;
                    if ( square.solved() )
                        continue;
                    if ( square.unset( x_wing_value ) )
                    {
         //               std::cout << "We did something for X-Wings, slice ID = " << slice_col_1.id() << ", value = " << x_wing_value << std::endl;
                        slice_col_1.set_square( j, square );
                        something_changed_at_all = true;
                        a_change_was_made = true;
                    }
                }
                if ( a_change_was_made )
                    sudoku.update_slice( slice_col_1 );
                OneSudokuSlice slice_col_2 = sudoku.column( row_1_col_2 );
                a_change_was_made = false;
                for ( size_t j( 0 ); j != slice_col_2.size(); ++j )
                {
                    OneSudokuSquare square = slice_col_2.square( j );
                    if ( ( j == row_1 ) || ( j == row_2 ) )
                        continue;
                    if ( square.solved() )
                        continue;
                    if ( square.unset( x_wing_value ) )
                    {
     //                   std::cout << "We did something for X-Wings, slice ID = " << slice_col_2.id() << ", value = " << x_wing_value << std::endl;
                        slice_col_2.set_square( j, square );
                        something_changed_at_all = true;
                        a_change_was_made = true;
                    }
                }
                if ( a_change_was_made )
                    sudoku.update_slice( slice_col_2 );
            }
        }
    }
    return something_changed_at_all;
}

// ********************************************************************************

bool apply_X_Wings_on_columns( Sudoku & sudoku, const size_t iCol_1, const size_t iCol_2 )
{
    bool something_changed_at_all( false );
    for ( size_t col_1( iCol_1 ); col_1 != iCol_1+3; ++col_1 )
    {
        OneSudokuSlice slice_col_1 = sudoku.column( col_1 );
        std::vector< size_t > unsolved_squares_indices = slice_col_1.indices_of_unsolved_squares();
        if ( unsolved_squares_indices.size() < 2 )
            continue;
        SetOfNumbers frequency_is_2_in_col_1( SetOfNumbers::THROW );
        { // Scoping brackets
            // For each number present in an unsolved square, count how many occurrences there are.
            SetOfNumbers values;
            for ( size_t i( 0 ); i != unsolved_squares_indices.size(); ++i )
            {
                OneSudokuSquare square = slice_col_1.square( unsolved_squares_indices[i] );
                values.add( square.values() );
            }
            for ( size_t i( 1 ); i != 10; ++i )
            {
                if ( values.frequency( i ) == 2 )
                    frequency_is_2_in_col_1.add( i );
            }
        } // Scoping brackets
        if ( frequency_is_2_in_col_1.empty()  )
            continue;
        for ( size_t col_2( iCol_2 ); col_2 != iCol_2+3; ++col_2 )
        {
            OneSudokuSlice slice_col_2 = sudoku.column( col_2 );
            unsolved_squares_indices = slice_col_2.indices_of_unsolved_squares();
            if ( unsolved_squares_indices.size() < 2 )
                continue;
            SetOfNumbers frequency_is_2_in_col_2( SetOfNumbers::THROW );
            { // Scoping brackets
                // For each number present in an unsolved square, count how many occurrences there are.
                SetOfNumbers values;
                for ( size_t i( 0 ); i != unsolved_squares_indices.size(); ++i )
                {
                    OneSudokuSquare square = slice_col_2.square( unsolved_squares_indices[i] );
                    values.add( square.values() );
                }
                for ( size_t i( 1 ); i != 10; ++i )
                {
                    if ( values.frequency( i ) == 2 )
                        frequency_is_2_in_col_2.add( i );
                }
            } // Scoping brackets
            if ( frequency_is_2_in_col_2.empty()  )
                continue;
            // When we are here, both column 1 and column 2 have at least one number that occurs in exactly two unsolved squares.
            // First, check if they have any numbers in common.
            SetOfNumbers intersection = frequency_is_2_in_col_1.in_common( frequency_is_2_in_col_2 );
            // Second, the numbers must occur in the same two rows.
            for ( size_t i( 0 ); i != intersection.size(); ++i )
            {
                size_t x_wing_value = intersection.value( i );
                size_t col_1_row_1( 9 );
                size_t col_1_row_2( 9 );
                size_t col_2_row_1( 9 );
                size_t col_2_row_2( 9 );
                for ( size_t j( 0 ); j != slice_col_1.size(); ++j )
                {
                    OneSudokuSquare square = slice_col_1.square( j );
                    if ( square.contains( x_wing_value ) )
                    {
                        if ( col_1_row_1 == 9 )
                            col_1_row_1 = j;
                        else
                        {
                            col_1_row_2 = j;
                            break;
                        }
                    }
                }
                for ( size_t j( 0 ); j != slice_col_2.size(); ++j )
                {
                    OneSudokuSquare square = slice_col_2.square( j );
                    if ( square.contains( x_wing_value ) )
                    {
                        if ( col_2_row_1 == 9 )
                            col_2_row_1 = j;
                        else
                        {
                            col_2_row_2 = j;
                            break;
                        }
                    }
                }
                if ( col_1_row_1 != col_2_row_1 )
                    continue;
                if ( col_1_row_2 != col_2_row_2 )
                    continue;
                // We can now erase x_wing_value in those two rows everywhere except in columns col_1 and col_2.
                OneSudokuSlice slice_row_1 = sudoku.row( col_1_row_1 );
                bool a_change_was_made( false );
                for ( size_t j( 0 ); j != slice_row_1.size(); ++j )
                {
                    OneSudokuSquare square = slice_row_1.square( j );
                    if ( ( j == col_1 ) || ( j == col_2 ) )
                        continue;
                    if ( square.solved() )
                        continue;
                    if ( square.unset( x_wing_value ) )
                    {
          //              std::cout << "We did something for X-Wings, slice ID = " << slice_row_1.id() << ", value = " << x_wing_value << std::endl;
                        slice_row_1.set_square( j, square );
                        something_changed_at_all = true;
                        a_change_was_made = true;
                    }
                }
                if ( a_change_was_made )
                    sudoku.update_slice( slice_row_1 );
                OneSudokuSlice slice_row_2 = sudoku.row( col_1_row_2 );
                a_change_was_made = false;
                for ( size_t j( 0 ); j != slice_row_2.size(); ++j )
                {
                    OneSudokuSquare square = slice_row_2.square( j );
                    if ( ( j == col_1 ) || ( j == col_2 ) )
                        continue;
                    if ( square.solved() )
                        continue;
                    if ( square.unset( x_wing_value ) )
                    {
        //                std::cout << "We did something for X-Wings, slice ID = " << slice_row_2.id() << ", value = " << x_wing_value << std::endl;
                        slice_row_2.set_square( j, square );
                        something_changed_at_all = true;
                        a_change_was_made = true;
                    }
                }
                if ( a_change_was_made )
                    sudoku.update_slice( slice_row_2 );
            }
        }
    }
    return something_changed_at_all;    
}

// ********************************************************************************

bool apply_X_Wings( Sudoku & sudoku )
{
    bool something_changed_on_first_pass( false );

    // Try each of rows 0, 1 & 2 against each of rows 3, 4 & 5
    if ( apply_X_Wings_on_rows( sudoku, 0, 3 ) )
        something_changed_on_first_pass = true;

    // Try each of rows 0, 1 & 2 against each of rows 6, 7 & 8
    if ( apply_X_Wings_on_rows( sudoku, 0, 6 ) )
        something_changed_on_first_pass = true;

    // Try each of rows 3, 4 & 5 against each of rows 6, 7 & 8
    if ( apply_X_Wings_on_rows( sudoku, 3, 6 ) )
        something_changed_on_first_pass = true;

    // Try each of columns 0, 1 & 2 against each of columns 3, 4 & 5
    if ( apply_X_Wings_on_columns( sudoku, 0, 3 ) )
        something_changed_on_first_pass = true;

    // Try each of columns 0, 1 & 2 against each of columns 6, 7 & 8
    if ( apply_X_Wings_on_columns( sudoku, 0, 6 ) )
        something_changed_on_first_pass = true;

    // Try each of columns 3, 4 & 5 against each of columns 6, 7 & 8
    if ( apply_X_Wings_on_columns( sudoku, 3, 6 ) )
        something_changed_on_first_pass = true;

    return something_changed_on_first_pass;
}

// ********************************************************************************

bool empty_rectangle( Sudoku & sudoku )
{
    bool something_changed_on_first_pass( false );
    // Find an empty rectangle block.
    for ( size_t i( 0 ); i != 9; ++i )
    {
        OneSudokuSlice block = sudoku.block( i );
        // There must be at least four solved squares.
        SetOfNumbers indices_of_solved_squares = SetOfNumbers( block.indices_of_solved_squares() );
        if ( indices_of_solved_squares.size() < 4 )
            continue;
        // There must be at least two unsolved squares.
        if ( indices_of_solved_squares.size() > 7 )
            continue;
        // I should be able to draw exactly one horizontal line and one vertical line and that should cut all of the unsolved ones
        // and it should not be possible to cut all of the unsolved ones with a single line
        // This means that the block lay-outs must be defined in the same way for each block, which is the case.
        bool found( false );
        size_t row;
        size_t col;
        size_t rc2index[3][3] = { {0,1,2},
                                  {3,4,5},
                                  {6,7,8} };
        for ( int iRow( 0 ); iRow != 3; ++iRow )
        {
            for ( int iCol( 0 ); iCol != 3; ++iCol )
            {
                CyclicInteger row_m1( 0, 2, iRow-1 );
                CyclicInteger row_p1( 0, 2, iRow+1 );
                CyclicInteger col_m1( 0, 2, iCol-1 );
                CyclicInteger col_p1( 0, 2, iCol+1 );
                if ( ( block.square( rc2index[row_m1.current_value()][col_m1.current_value()] ).solved() ) &&
                     ( block.square( rc2index[row_m1.current_value()][col_p1.current_value()] ).solved() ) &&
                     ( block.square( rc2index[row_p1.current_value()][col_m1.current_value()] ).solved() ) &&
                     ( block.square( rc2index[row_p1.current_value()][col_p1.current_value()] ).solved() ) )
                {
                    // Check unsolved squares are not all in one line.
                    if ( ( ( ! block.square( rc2index[row_m1.current_value()][iCol] ).solved() ) || ( ! block.square( rc2index[row_p1.current_value()][iCol] ).solved() ) ) && 
                         ( ( ! block.square( rc2index[iRow][col_m1.current_value()] ).solved() ) || ( ! block.square( rc2index[iRow][col_p1.current_value()] ).solved() ) ) )
                    {
                        row = iRow;
                        col = iCol;
                        found = true;
                        break;
                    }
                }
            }
            if ( found )
                break;
        }
        if ( ! found )
            continue;

    }
    return something_changed_on_first_pass;
}

// ********************************************************************************

bool update_trivial_cases( Sudoku & sudoku )
{
	bool result( false );
    for ( size_t i( 0 ); i != Sudoku::number_of_slices(); ++i )
    {
        if ( sudoku.solved() )
       	    return false;
        OneSudokuSlice slice = sudoku.next_slice();
        if ( check_if_we_are_the_only_possibility( slice ) )
        {
            sudoku.update_slice( slice );
            result = true;
        }
    }
    return result;
}

// ********************************************************************************

bool apply_checkerboard( Sudoku & sudoku )
{
    // @@ We are constanly redetermining all of this. This could be a property
    // of the Sudoku object.

    // It is weird, but all "white" squares in columns must be identical to all white squares in rows.
    bool result( false );
    std::vector< OneSudokuSquare > col_white_squares;
    std::vector< OneSudokuSquare > row_white_squares;
    std::vector< size_t > col_status_values;
    std::vector< size_t > row_status_values;
    for ( size_t i( 0 ); i != 20; ++i )
    {
        col_white_squares.push_back( sudoku.square( col_white_squares_indices[ i ] ) );
        col_status_values.push_back( col_white_squares[i].size() );
    }
    for ( size_t i( 0 ); i != 20; ++i )
    {
        row_white_squares.push_back( sudoku.square( row_white_squares_indices[ i ] ) );
        row_status_values.push_back( row_white_squares[i].size() );
    }
    // I am not sure yet how to actually use this information?
    // The most straightforward way is if one of the two sets more singletons than the other.
    // We could sort first, or remove all pairs of matching singletons.
    // We do not really want to remove any of the members of col_white_squares or row_white_squares
    // because removing is difficult and because we would lose the information which square in
    // the original sudoku it mapped to.
    // "0" means it has been used. "1" means it can only contain one digit (i.e. has been solved)
    // but we have not found its counterpart yet. "2" or higher is the number of remaining digits in this square.
    for ( size_t i( 0 ); i != 20; ++i )
    {
        if ( col_status_values[i] != 1 )
            continue;
        for ( size_t j( 0 ); j != 20; ++j )
        {
            if ( row_status_values[j] != 1 )
               continue;
            if ( col_white_squares[i].value() == row_white_squares[j].value() )
            {
                col_status_values[i] = 0;
                row_status_values[j] = 0;
                break;
            }
        }
    }
    // How many solved squares that have not been matched yet are left?
    size_t nunmatched_cols( 0 );
    for ( size_t i( 0 ); i != 20; ++i )
    {
        if ( col_status_values[i] == 1 )
           ++nunmatched_cols;
    }
    size_t nunmatched_rows( 0 );
    for ( size_t i( 0 ); i != 20; ++i )
    {
        if ( row_status_values[i] == 1 )
           ++nunmatched_rows;
    }
    if ( nunmatched_cols > 0 )
    {
        for ( size_t i( 0 ); i != 20; ++i )
        {
            if ( col_status_values[i] == 1 )
            {
                size_t value_to_find = col_white_squares[i].value();
                size_t npossible_squares( 0 );
                size_t square_index( 21 );
                for ( size_t j( 0 ); j != 20; ++j )
                {
                    if ( row_status_values[j] < 2 )
                       continue;
                    if ( row_white_squares[j].contains( value_to_find ) )
                    {
                       ++npossible_squares;
                       square_index = j;
                    }
                }
                if ( npossible_squares == 1 )
                {
                    result = true; // Something changed.
                    sudoku.update_square( row_white_squares_indices[ square_index ], OneSudokuSquare( value_to_find ) );
                    col_status_values[i] = 0;
                    row_status_values[ square_index ] = 0;
                    --nunmatched_cols;
                }
            }
        }
    }
    if ( nunmatched_rows > 0 )
    {
        for ( size_t i( 0 ); i != 20; ++i )
        {
            if ( row_status_values[i] == 1 )
            {
                size_t value_to_find = row_white_squares[i].value();
                size_t npossible_squares( 0 );
                size_t square_index( 21 );
                for ( size_t j( 0 ); j != 20; ++j )
                {
                    if ( col_status_values[j] < 2 )
                       continue;
                    if ( col_white_squares[j].contains( value_to_find ) )
                    {
                       ++npossible_squares;
                       square_index = j;
                    }
                }
                if ( npossible_squares == 1 )
                {
                    result = true; // Something changed.
                    sudoku.update_square( col_white_squares_indices[ square_index ], OneSudokuSquare( value_to_find ) );
                    row_status_values[i] = 0;
                    col_status_values[ square_index ] = 0;
                    --nunmatched_rows;
                }
            }
        }
    }
    // Whatever is now left in the columns must also be present in the rows and vice versa.
    // So if either the columns or the rows contain a number that is not present in the other,
    // then that digit can be deleted.

    SetOfNumbers possible_values_col( SetOfNumbers::AUTO_REMOVE );
    for ( size_t i( 0 ); i != 20; ++i )
    {
        if ( col_status_values[i] == 0 )
           continue;
        possible_values_col.add( col_white_squares[i].values() );
    }
    SetOfNumbers possible_values_row( SetOfNumbers::AUTO_REMOVE );
    for ( size_t i( 0 ); i != 20; ++i )
    {
        if ( row_status_values[i] == 0 )
           continue;
        possible_values_row.add( row_white_squares[i].values() );
    }

    SetOfNumbers in_common = possible_values_col.in_common( possible_values_row );
  //  in_common.show();
    for ( size_t i( 0 ); i != possible_values_col.size(); ++i )
    {
        if ( in_common.contains( possible_values_col.value( i ) ) )
           continue;
        // When we are here, we can remove the value possible_values_col.value( i ) from all row_white_squares that do not have a row_status_values of 0.
        for ( size_t j( 0 ); j != 20; ++j )
        {
            if ( row_status_values[j] == 0 )
               continue;
            if ( row_white_squares[j].contains( possible_values_col.value( i ) ) )
            {
                row_white_squares[j].unset( possible_values_col.value( i ) );
                sudoku.update_square( row_white_squares_indices[j], row_white_squares[j] );
                result = true; // Something changed.
                std::cout << "apply_checkerboard() final did something." << std::endl;
            }
        }
    }

    for ( size_t i( 0 ); i != possible_values_row.size(); ++i )
    {
        if ( in_common.contains( possible_values_row.value( i ) ) )
           continue;
        // When we are here, we can remove the value possible_values_row.value( i ) from all col_white_squares that do not have a col_status_values of 0.
        for ( size_t j( 0 ); j != 20; ++j )
        {
            if ( col_status_values[j] == 0 )
               continue;
            if ( col_white_squares[j].contains( possible_values_row.value( i ) ) )
            {
                col_white_squares[j].unset( possible_values_row.value( i ) );
                sudoku.update_square( col_white_squares_indices[j], col_white_squares[j] );
                result = true; // Something changed.
                std::cout << "apply_checkerboard() final did something." << std::endl;
            }
        }
    }

    // Are in dark blue or in lite blue all of a certain digit solved? Then we know how often it occurs in the other one.

    // We can subtract the solved white squares (both columns and rows) from the maximum
    // number of times a digit can occur (5 in dark blue, 4 in lite blue).
    std::vector< size_t > nsolved_per_digit_white_squares( 9, 0 );
    for ( size_t i( 0 ); i != 20; ++i )
    {
        if ( col_white_squares[i].size() == 1 )
           ++nsolved_per_digit_white_squares[ col_white_squares[i].value() - 1 ];
        if ( row_white_squares[i].size() == 1 )
           ++nsolved_per_digit_white_squares[ row_white_squares[i].value() - 1 ];
    }
    std::vector< size_t > max_per_digit_dark_blue_squares( 9, 5 );
    std::vector< size_t > max_per_digit_lite_blue_squares( 9, 4 );
    for ( size_t j( 0 ); j != 9; ++j )
    {
        max_per_digit_dark_blue_squares[j] -= nsolved_per_digit_white_squares[j];
        max_per_digit_lite_blue_squares[j] -= nsolved_per_digit_white_squares[j];
    }

    std::vector< OneSudokuSquare > dark_blue_squares;
    std::vector< OneSudokuSquare > lite_blue_squares;
    std::vector< size_t > dark_blue_status_values;
    std::vector< size_t > lite_blue_status_values;
    std::vector< size_t > nsolved_per_digit_dark_blue_squares( 9, 0 );
    std::vector< size_t > nsolved_per_digit_lite_blue_squares( 9, 0 );
    for ( size_t i( 0 ); i != 25; ++i )
    {
        dark_blue_squares.push_back( sudoku.square( dark_blue_squares_indices[ i ] ) );
        dark_blue_status_values.push_back( dark_blue_squares[i].size() );
        if ( dark_blue_squares[i].size() == 1 )
        {
           ++nsolved_per_digit_dark_blue_squares[ dark_blue_squares[i].value() - 1 ];
        }
    }
    for ( size_t i( 0 ); i != 16; ++i )
    {
        lite_blue_squares.push_back( sudoku.square( lite_blue_squares_indices[ i ] ) );
        lite_blue_status_values.push_back( lite_blue_squares[i].size() );
        if ( lite_blue_squares[i].size() == 1 )
        {
           ++nsolved_per_digit_lite_blue_squares[ lite_blue_squares[i].value() - 1 ];
        }
    }
    // We are going to test two equalities:
    // 1. nsolved_per_digit_dark_blue_squares[i] + nsolved_per_digit_lite_blue_squares[i] == odd.
    // 2. nsolved_per_digit_dark_blue_squares[i] == nsolved_per_digit_lite_blue_squares[i] + 1.

    // If one digit has been solved four times in lite, then it must occur five times in dark.
    for ( size_t j( 0 ); j != 9; ++j )
    {
        if ( ( nsolved_per_digit_dark_blue_squares[j] == max_per_digit_dark_blue_squares[j] ) &&
             ( nsolved_per_digit_lite_blue_squares[j] != max_per_digit_lite_blue_squares[j] ) )
        {
            // How many times does it occur in the unsolved squares?
            size_t noccurrences( 0 );
            for ( size_t i( 0 ); i != 16; ++i )
            {
                if ( lite_blue_status_values[i] == 1 )
                   continue;
                if ( lite_blue_squares[i].contains( j+1 ) )
                   ++noccurrences;
            }
            if ( nsolved_per_digit_dark_blue_squares[j] == ( nsolved_per_digit_lite_blue_squares[j] + noccurrences + 1 ) )
            {
                std::cout << "apply_checkerboard() yes 1 will do something." << std::endl;
                for ( size_t i( 0 ); i != 16; ++i )
                {
                    if ( lite_blue_status_values[i] == 1 )
                       continue;
                    if ( lite_blue_squares[i].contains( j+1 ) )
                    {
                        lite_blue_squares[i] = OneSudokuSquare( j+1 );
                        lite_blue_status_values[i] = 1;
                        sudoku.update_square( lite_blue_squares_indices[i], OneSudokuSquare( j+1 ) );
                        ++nsolved_per_digit_lite_blue_squares[j];
                        result = true; // Something changed.
                        std::cout << "apply_checkerboard() yes 1 did something." << std::endl;
                    }
                }
            }
        }
        if ( ( nsolved_per_digit_dark_blue_squares[j] != max_per_digit_dark_blue_squares[j] ) &&
             ( nsolved_per_digit_lite_blue_squares[j] == max_per_digit_lite_blue_squares[j] ) )
        {
            // How many times does it occur in the unsolved squares?
            size_t noccurrences( 0 );
            for ( size_t i( 0 ); i != 25; ++i )
            {
                if ( dark_blue_status_values[i] == 1 )
                   continue;
                if ( dark_blue_squares[i].contains( j+1 ) )
                   ++noccurrences;
            }
            if ( ( nsolved_per_digit_lite_blue_squares[j] + 1 ) == ( nsolved_per_digit_dark_blue_squares[j] + noccurrences ) )
            {
                std::cout << "apply_checkerboard() yes 2 will do something." << std::endl;
                for ( size_t i( 0 ); i != 25; ++i )
                {
                    if ( dark_blue_status_values[i] == 1 )
                       continue;
                    if ( dark_blue_squares[i].contains( j+1 ) )
                    {
                        dark_blue_squares[i] = OneSudokuSquare( j+1 );
                        dark_blue_status_values[i] = 1;
                        sudoku.update_square( dark_blue_squares_indices[i], OneSudokuSquare( j+1 ) );
                        ++nsolved_per_digit_dark_blue_squares[j];
                        result = true; // Something changed.
                        std::cout << "apply_checkerboard() yes 2 did something." << std::endl;
                    }
                }
            }
        }
    }

    for ( size_t j( 0 ); j != 9; ++j )
    {
        if ( is_even( nsolved_per_digit_dark_blue_squares[j] + nsolved_per_digit_lite_blue_squares[j] ) )
        {
            // If currently an even number has been solved, and there is only one possibility left among the unsolved squares, then we must set it.
    //        nunsolved
   //         std::cout << "Yes.3..." << std::endl;
        }
    }

return result;


    // Both equalities remain true if I remove a solved digit simultaneously from dark and from lite.
    // But I *cannot* remove two of the same solved digit from either dark or lite, because then 
    // condition 2. can no longer be tested.
    // Are there any solved digits that occur once in each? Then set their status to 0.
    for ( size_t j( 0 ); j != 9; ++j )
    {
        if ( ( nsolved_per_digit_dark_blue_squares[j] > 0 ) &&
             ( nsolved_per_digit_lite_blue_squares[j] > 0 ) )
        {
     //       std::cout << "So far, so good." << std::endl;
            for ( size_t i( 0 ); i != 25; ++i )
            {
                if ( dark_blue_status_values[i] == 0 )
                   continue;
                if ( ( dark_blue_status_values[i] == 1 ) && ( dark_blue_squares[i].value() == j+1 ) )
                {
                    dark_blue_status_values[i] = 0;
     //               --nsolved_dark_blue_squares;
                    --nsolved_per_digit_dark_blue_squares[ j ];
                    break;
                }
            }
            for ( size_t i( 0 ); i != 16; ++i )
            {
                if ( lite_blue_status_values[i] == 0 )
                   continue;
                if ( ( lite_blue_status_values[i] == 1 ) && ( lite_blue_squares[i].value() == j+1 ) )
                {
                    lite_blue_status_values[i] = 0;
     //               --nsolved_lite_blue_squares;
                    --nsolved_per_digit_lite_blue_squares[ j ];
                    break;
                }
            }
        }
    }

// If currently the sum of solved digits is even and in the unsolved squares it only occurs once, then that must be set

    // I am not sure yet how to actually use this information?
    // The most straightforward way is if one of the two sets more singletons than the other.
    // We could sort first, or remove all pairs of matching singletons.
    // We do not really want to remove any of the members of col_white_squares or row_white_squares
    // because removing is difficult and because we would lose the information which square in
    // the original sudoku it mapped to.
    // "0" means it has been used. "1" means it can only contain one digit (i.e. has been solved)
    // but we have not found its counterpart yet. "2" or higher is the number of remaining digits in this square.
//    for ( size_t i( 0 ); i != 20; ++i )
//    {
//        if ( col_status_values[i] != 1 )
//            continue;
//        for ( size_t j( 0 ); j != 20; ++j )
//        {
//            if ( row_status_values[j] != 1 )
//               continue;
//            if ( col_white_squares[i].value() == row_white_squares[j].value() )
//            {
//                col_status_values[i] = 0;
//                row_status_values[j] = 0;
//                break;
//            }
//        }
//    }
//    // How many solved squares that have not been matched yet are left?
//    size_t nunmatched_cols( 0 );
//    for ( size_t i( 0 ); i != 20; ++i )
//    {
//        if ( col_status_values[i] == 1 )
//           ++nunmatched_cols;
//    }
//    size_t nunmatched_rows( 0 );
//    for ( size_t i( 0 ); i != 20; ++i )
//    {
//        if ( row_status_values[i] == 1 )
//           ++nunmatched_rows;
//    }

    return result;
}

} // namespace

// ********************************************************************************

void solve_without_guessing( Sudoku & result, bool & error_caught, size_t & counter )
{
    ++counter;
//    std::cout << "Now entering solve_without_guessing()." << std::endl;
    error_caught = false;
    try
    {
        bool there_were_changes( true );
        while ( there_were_changes && ( ! result.solved() ) && ( ! result.there_are_contradictions() ) )
        {
            there_were_changes = false;
            while ( update_trivial_cases( result ) )
            {
                there_were_changes = true;
            }
            if ( holistic( result ) )
            {
                there_were_changes = true;
                update_trivial_cases( result );
            }
            if ( apply_X_Wings( result ) )
            {
                there_were_changes = true;
                update_trivial_cases( result );
            }
            if ( empty_rectangle( result ) )
            {
                there_were_changes = true;
                update_trivial_cases( result );
            }
            if ( apply_checkerboard( result ) )
            {
                there_were_changes = true;
                update_trivial_cases( result );
                std::cout << "apply_checkerboard() did something." << std::endl;
            }
        }
    }
    catch ( std::exception & e )
    {
        error_caught = true;
    }
}

// ********************************************************************************

Sudoku solve( const Sudoku & sudoku )
{
    Sudoku result( sudoku );
    size_t niterations( 0 );
    Stack< Sudoku > sudoku_guesses;
    Stack< size_t > square_indices;
    Stack< size_t > guessed_number_indices;
    size_t nguesses( 0 );
    bool error_caught( false );
    size_t max_sp( 0 );
    size_t solve_without_guessing_counter( 0 );
    do
    {
        if ( sudoku_guesses.stack_pointer() > max_sp )
            max_sp = sudoku_guesses.stack_pointer();
//        std::cout << "stack pointer = " << sudoku_guesses.stack_pointer() << ", max. stack pointer = " << max_sp << std::endl;
        ++niterations;
        solve_without_guessing( result, error_caught, solve_without_guessing_counter );
        if ( result.solved() )
            std::cout << "The sudoku has been solved without the need for guessing." << std::endl;
        else if ( ( ! error_caught ) && ( ! result.there_are_contradictions() ) )
        {
            // There were no more changes with our normal methods.
            // Take each square with only two possibilities and see if one value leads to a contradiction, in which case it is solved by setting it to the other value.
            bool error_caught_2( false );
            for ( size_t square_index( 0 ); square_index != 81; ++square_index )
            {
                if ( result.square( square_index ).solved() )
                    continue;
                if ( result.square( square_index ).values().size() > 2 )
                    continue;
                // Try the first value.
                sudoku_guesses.push( result );
                OneSudokuSquare square = result.square( square_index );
                square = OneSudokuSquare( square.value( 0 ) );
                result.update_square( square_index, square );
                solve_without_guessing( result, error_caught_2, solve_without_guessing_counter );
                if ( result.solved() )
                    break;
                if ( error_caught_2 || result.there_are_contradictions() )
                {
                    // Undo last guess.
                    result = sudoku_guesses.pop();
                    OneSudokuSquare square = result.square( square_index );
                    square.unset( square.value( 0 ) );
                    result.update_square( square_index, square );
//                    std::cout << "We solved a square by contradiction." << std::endl;
                    solve_without_guessing( result, error_caught_2, solve_without_guessing_counter );
                }
                else // Try the second value.
                {
                    // Undo last guess.
                    result = sudoku_guesses.pop();
                    sudoku_guesses.push( result );
                    OneSudokuSquare square = result.square( square_index );
                    square = OneSudokuSquare( square.value( 1 ) );
                    result.update_square( square_index, square );
                    solve_without_guessing( result, error_caught_2, solve_without_guessing_counter );
                    if ( error_caught_2 || result.there_are_contradictions() )
                    {
                        // Undo last guess.
                        result = sudoku_guesses.pop();
                        OneSudokuSquare square = result.square( square_index );
                        square.unset( square.value( 1 ) );
                        result.update_square( square_index, square );
//                        std::cout << "We solved a square by contradiction." << std::endl;
                        solve_without_guessing( result, error_caught_2, solve_without_guessing_counter );
                    }
                    else // We have not learned anything new, restore the old state.
                    {
                        // Undo last guess.
                        result = sudoku_guesses.pop();
                    }
                }
            }
        }
        
        // When we are here, there are three options:
        // 1. The sudoku is solved
        // 2. The sudoku is not solved, but there are no errors or contradictions
        // 3. The sudoku has errors / contradictions
        if ( error_caught || ( ! result.solved() ) || result.there_are_contradictions() )
        {
            ++nguesses;
//            std::cout << "nguesses = " << nguesses << std::endl;
            if ( error_caught || result.there_are_contradictions() )
            {
                bool found( false );
                do
                {
                    if ( sudoku_guesses.empty() )
                    {
                        result.show();
                        throw std::runtime_error( "Programming error." );
                    }
                    // Undo last guess.
                    result = sudoku_guesses.pop();
                    size_t square_index = square_indices.pop();
                    size_t guessed_number_index = guessed_number_indices.pop();
                    ++guessed_number_index;
                    if ( guessed_number_index == result.square( square_index ).size() )
                    {
                        guessed_number_index = 0;
                        // Find next square that has not been solved.
                        for ( size_t i( square_index+1 ); i != result.nsquares(); ++i )
                        {
                            if ( ! result.square( i ).solved() )
                            {
                                square_index = i;
                                found = true;
                                break;
                            }
                        }
                    }
                    else
                        found = true;
                    if ( found )
                    {
                        sudoku_guesses.push( result );
                        square_indices.push( square_index );
                        OneSudokuSquare square = result.square( square_index );
                        guessed_number_indices.push( guessed_number_index );
                        square = OneSudokuSquare( square.value( guessed_number_index ) );
                        result.update_square( square_index, square );
                    }
                }
                while ( ! found );
            }
            else
            {
                // Find first square that has not been solved.
                size_t square_index( result.nsquares() );
                for ( size_t i( 0 ); i != result.nsquares(); ++i )
                {
                    if ( ! result.square( i ).solved() )
                    {
                        square_index = i;
                        break;
                    }
                }
                sudoku_guesses.push( result );
                square_indices.push( square_index );
                OneSudokuSquare square = result.square( square_index );
                size_t guessed_number_index = 0;
                guessed_number_indices.push( guessed_number_index );
                square = OneSudokuSquare( square.value( guessed_number_index ) );
                result.update_square( square_index, square );
            }
        }
    } while ( ! result.solved() );
    std::cout << "solve_without_guessing_counter: " << solve_without_guessing_counter << std::endl;
    std::cout << "Number of iterations: " << niterations % 27 << std::endl;
    std::cout << "stack pointer = " << sudoku_guesses.stack_pointer() << ", max. stack pointer = " << max_sp << std::endl;
    return result;
}

// ********************************************************************************

