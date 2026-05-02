#ifndef DIVIDE_H
#define DIVIDE_H

#include "auxiliary.h"
#include "addition.h"

uint32_t divide(uint32_t numA, uint32_t numB) {
    IEEE754 a(numA);
    IEEE754 b(numB);

    if (numB == 0) return INF; 
    if (numA == 0) return 0;

    uint8_t resSign = a.sign ^ b.sign;

    int32_t resExp = a.exponent - b.exponent + 127;

    uint64_t tempMantA = static_cast<uint64_t>(a.mantissa) << 23;
    uint32_t resMant = static_cast<uint32_t>(tempMantA / b.mantissa);

    a.sign = resSign;
    a.exponent = resExp;
    a.mantissa = resMant;

    normalizeMantissa(a.mantissa, a.exponent);

    return reconstructNumberFromStruct(a);
}

#endif