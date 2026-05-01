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
#include <cstring>

using namespace std;

const uint32_t INF = 0x7F800000;
const regex BIN_STR(R"(^0b[01]+$)");
const regex DEC_STR(R"(^[0-9]*\.?[0-9]+$)");

struct NormalizedMantissas {
    uint32_t mantA; // 24-bit mantissa with implicit bit
    uint32_t mantB;
};

/*
    Convert values
*/
uint32_t textToFloatingPointBinary(string decimalNumber) {
    if (decimalNumber.length() < 1 ) return INF;

    uint32_t floatingBin = INF;

    if (regex_match(decimalNumber, DEC_STR)) {
        float f = stof(decimalNumber);
        // Copy the float bit pattern into an integer without reinterpreting the type.
        memcpy(&floatingBin, &f, sizeof(float));
    }

    else if (regex_match(decimalNumber, BIN_STR)) 
        floatingBin = std::stoul(decimalNumber.substr(2), nullptr, 2);

    else
        return INF;

    return floatingBin;
}

/*
    Align exponents for two IEEE-754 numbers
*/
NormalizedMantissas normalizeExponents(uint32_t numA, uint32_t numB) {
    uint32_t expA = (numA >> 23) & 0xFF;
    uint32_t mantA = numA & 0x7FFFFF;

    uint32_t expB = (numB >> 23) & 0xFF;
    uint32_t mantB = numB & 0x7FFFFF;

    uint32_t fullMantA = (expA == 0) ? mantA : (mantA | (1 << 23));
    uint32_t fullMantB = (expB == 0) ? mantB : (mantB | (1 << 23));

    int d = static_cast<int>(expA) - static_cast<int>(expB);

    if (d > 0) {
        fullMantB >>= d;
    } else if (d < 0) {
        fullMantA >>= (-d);
    }
    NormalizedMantissas result;
    result.mantA = fullMantA;
    result.mantB = fullMantB;
    return result;
}

/*
    Reconstruct the 24-bit mantissa for a 32-bit IEEE-754 number
*/
uint32_t reconstructMantissa(uint32_t ieeeValue) {
    uint32_t exp = (ieeeValue >> 23) & 0xFF;
    uint32_t mant = ieeeValue & 0x7FFFFF;

    if (exp == 0) {
        return mant;
    }

    return mant | (1u << 23);
}

/*
    Normalize mantissa and exponent in-place after operations
*/
void normalizeMantissa(uint32_t& mantissa, int32_t& exponent) {
    if (mantissa == 0) {
        exponent = 0;
        return;
    }

    if (mantissa & (1 << 24)) {
        mantissa >>= 1;
        exponent++;
    } else {
        while (mantissa > 0 && (mantissa & (1 << 23)) == 0) {
            mantissa <<= 1;
            exponent--;
        }
    }
}

// Multiply two IEEE-754 32-bit values using explicit mantissa and exponent handling.
uint32_t multiplyIEEE754(uint32_t numA, uint32_t numB) {
    uint32_t signA = numA >> 31;
    uint32_t signB = numB >> 31;
    uint32_t sign = signA ^ signB;

    uint32_t expA = (numA >> 23) & 0xFF;
    uint32_t expB = (numB >> 23) & 0xFF;
    uint32_t mantA = numA & 0x7FFFFF;
    uint32_t mantB = numB & 0x7FFFFF;

    if ((expA == 0 && mantA == 0) || (expB == 0 && mantB == 0)) {
        return 0u;
    }

    if (expA == 255 || expB == 255) {
        return INF;
    }

    uint32_t fullMantA = reconstructMantissa(numA);
    uint32_t fullMantB = reconstructMantissa(numB);

    int32_t biasedExpA = expA == 0 ? 1 : static_cast<int32_t>(expA);
    int32_t biasedExpB = expB == 0 ? 1 : static_cast<int32_t>(expB);
    int32_t resultExp = biasedExpA + biasedExpB - 127;

    uint64_t product = static_cast<uint64_t>(fullMantA) * static_cast<uint64_t>(fullMantB);
    uint32_t resultMant;

    if (product & (1ull << 47)) {
        resultMant = static_cast<uint32_t>(product >> 24);
        resultExp++;
    } else {
        resultMant = static_cast<uint32_t>(product >> 23);
    }

    while (resultMant > 0 && (resultMant & (1u << 23)) == 0 && resultExp > 0) {
        resultMant <<= 1;
        resultExp--;
    }

    if (resultExp <= 0 || resultMant == 0) {
        return 0u;
    }
    if (resultExp >= 255) {
        return INF;
    }

    resultMant &= 0x7FFFFF;
    return (sign << 31) | (static_cast<uint32_t>(resultExp) << 23) | resultMant;
}

#endif