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

uint32_t subtractIEEE754(uint32_t numA, uint32_t numB) {
    uint32_t signA = numA >> 31;
    uint32_t signB = numB >> 31;
    uint32_t expA = (numA >> 23) & 0xFF;
    uint32_t expB = (numB >> 23) & 0xFF;
    uint32_t mantA = numA & 0x7FFFFF;
    uint32_t mantB = numB & 0x7FFFFF;

    // Handle special cases
    if (expA == 255 || expB == 255) return INF;
    if (expA == 0 && mantA == 0) return numB ^ (1U << 31); // -B
    if (expB == 0 && mantB == 0) return numA;

    uint32_t fullMantA = reconstructMantissa(numA);
    uint32_t fullMantB = reconstructMantissa(numB);

    int32_t biasedExpA = expA == 0 ? 1 : static_cast<int32_t>(expA);
    int32_t biasedExpB = expB == 0 ? 1 : static_cast<int32_t>(expB);

    // Normalize exponents
    if (biasedExpA > biasedExpB) {
        int shift = biasedExpA - biasedExpB;
        fullMantB >>= shift;
        biasedExpB = biasedExpA;
    } else if (biasedExpB > biasedExpA) {
        int shift = biasedExpB - biasedExpA;
        fullMantA >>= shift;
        biasedExpA = biasedExpB;
    }

    uint32_t resultMant;
    uint32_t resultSign = signA;

    if (signA == signB) {
        // Same sign: subtract mantissas
        if (fullMantA >= fullMantB) {
            resultMant = fullMantA - fullMantB;
        } else {
            resultMant = fullMantB - fullMantA;
            resultSign = 1 - signA; // Flip sign
        }
    } else {
        // Different signs: add mantissas
        resultMant = fullMantA + fullMantB;
    }

    int32_t resultExp = biasedExpA;

    // Normalize result
    if (resultMant & (1 << 24)) {
        resultMant >>= 1;
        resultExp++;
    } else {
        int shift = 0;
        while (resultMant > 0 && (resultMant & (1 << 23)) == 0) {
            resultMant <<= 1;
            shift++;
        }
        resultExp -= shift;
    }

    if (resultExp <= 0 || resultMant == 0) return 0;
    if (resultExp >= 255) return INF;

    resultMant &= 0x7FFFFF;
    return (resultSign << 31) | (static_cast<uint32_t>(resultExp) << 23) | resultMant;
}

string floatingPointBinaryToText(uint32_t binary) {
    if (binary == 0) return "0.0";
    if (binary == INF) return "inf";
    if (binary == (INF | (1 << 31))) return "-inf";
    if ((binary & 0x7F800000) == 0x7F800000 && (binary & 0x7FFFFF) != 0) return "nan";

    float f;
    memcpy(&f, &binary, sizeof(float));
    return to_string(f);
}

#endif