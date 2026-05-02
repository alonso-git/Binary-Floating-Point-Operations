#ifndef ADD_H
#define ADD_H

#include "auxiliary.h"

const uint32_t MANTISSA_MASK = 0x7FFFFF;
const uint32_t EXPONENT_MASK = 0xFF;
const uint32_t SIGN_MASK = 0x1;

struct IEEE754{
    uint8_t sign;
    int32_t exponent;
    uint32_t mantissa;

    IEEE754(uint32_t ieee754_num) {
        sign = (ieee754_num >> 31) & SIGN_MASK; 
        exponent = (ieee754_num >> 23) & EXPONENT_MASK;
        mantissa = ieee754_num & MANTISSA_MASK;
        mantissa = (exponent == 0) ? mantissa : (mantissa | (1 << 23));
    }
};

void bitwiseSubtraction(uint32_t& a, uint32_t& b) {
    while (b != 0) {
        uint32_t borrow = (~a) & b;
        
        a = a ^ b;
        
        b = borrow << 1;
    }
}

void bitwiseAddition(uint32_t& a, uint32_t& b) {
    while (b != 0) {
        uint32_t carry = a & b; 
        a = a ^ b;
        b = carry << 1;
    }
}

uint8_t determineSharedExponent (IEEE754 numA, IEEE754 numB) {
    return (numA.exponent > numB.exponent) ? numA.exponent : numB.exponent;
}

uint32_t reconstructNumberFromStruct(IEEE754 num) {
    num.mantissa &= MANTISSA_MASK;

    return (num.sign << 31 | num.exponent << 23 | num.mantissa );
}

uint32_t add(uint32_t numA, uint32_t numB) {
    IEEE754 a = IEEE754(numA);
    IEEE754 b = IEEE754(numB);

    NormalizedMantissas normMantissas = normalizeExponents(numA,numB);

    a.mantissa = normMantissas.mantA;
    b.mantissa = normMantissas.mantB;

    a.exponent = b.exponent = determineSharedExponent(a,b);

    if (a.sign == b.sign) {
        bitwiseAddition(a.mantissa, b.mantissa);
    } else {
        if (a.mantissa >= b.mantissa) {
            bitwiseSubtraction(a.mantissa, b.mantissa);
            if (a.mantissa == 0) {
                a.sign = 0;
                a.exponent = 0;
            }
        } else {
            bitwiseSubtraction(b.mantissa, a.mantissa);
            a = b;
        }
    }

    normalizeMantissa(a.mantissa, a.exponent);
    
    return reconstructNumberFromStruct(a);
}

#endif