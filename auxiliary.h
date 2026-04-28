/*
    Auxiliary functions related to string/decimal numbers and their
    IEEE-754 floating point representation.
    Cast, normalize, format.
*/

#ifndef AUX_H
#define AUX_H

#include <cstdint>
#include <bitset>
#include <bit>
#include <regex>
#include <string>

using namespace std;

// Constant values for convenience
const uint32_t INF = 0x7F800000;
const regex BIN_STR(R"(^0b[01]+$)");
const regex DEC_STR(R"(^[0-9]*\.?[0-9]+$)");

/*
    Convert values
*/
uint32_t textToFloatingPointBinary(string decimalNumber) {
    if (decimalNumber.length() < 1 ) return INF;

    uint32_t floatingBin;

    if (regex_match(decimalNumber, DEC_STR)) 
        floatingBin = bit_cast<uint32_t>(stof(decimalNumber));

    else if (regex_match(decimalNumber, BIN_STR)) 
        floatingBin = std::stoul(decimalNumber.substr(2), nullptr, 2);

    else
        return INF;

    return floatingBin;
}

#endif