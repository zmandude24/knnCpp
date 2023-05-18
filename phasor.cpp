#define _USE_MATH_DEFINES

using namespace std;

#include <cmath>
#include <exception>
#include <iostream>
#include <string>
#include "phasor.h"


const void phasor::UpdateRealAndImag() 
{
    real = RMSvalue * cos(M_PI / 180 * PhaseAngleDegrees);
    imaginary = RMSvalue * sin(M_PI / 180 * PhaseAngleDegrees);
}

const void phasor::UpdateRMSandPhase()
{
    RMSvalue = hypot(real, imaginary);

    if (real == 0) {   // special case to avoid division by zero
        if (imaginary < 0) PhaseAngleDegrees = -90;       // phasor is on border of Q3 and Q4
        else if (imaginary > 0) PhaseAngleDegrees = 90;   // phasor is on border of Q1 and Q2
        else PhaseAngleDegrees = 0;                       // This is the zero phasor case
    }
    else {
        PhaseAngleDegrees = 180 / M_PI * atan(imaginary / real);
        if (real < 0) {
            if (imaginary >= 0) PhaseAngleDegrees += 180; // phasor should be in Q2 and atan would've put it in Q4
            else PhaseAngleDegrees -= 180;                // phasor should be in Q3 and atan would've put it in Q1
        }
    }
}



phasor::phasor() {}
phasor::phasor(double rmsValue, double phaseAngleDegrees)
{
    RMSvalue = rmsValue;
    PhaseAngleDegrees = phaseAngleDegrees;
    UpdateRealAndImag();
}


const void phasor::Print()
{
    cout << RMSvalue << " @ " << PhaseAngleDegrees << "deg\n";
}
const string phasor::PhasorToString()
{
    return to_string(RMSvalue) + " @ " + to_string(PhaseAngleDegrees) + "deg";
}


const phasor phasor::operator+(const phasor& rhs) const
{
    phasor sum;
    sum.real = this->real + rhs.real;
    sum.imaginary = this->imaginary + rhs.imaginary;
    sum.UpdateRMSandPhase();
    return sum;
}
const phasor phasor::operator-(const phasor& rhs) const
{
    phasor difference;
    difference.real = this->real - rhs.real;
    difference.imaginary = this->imaginary - rhs.imaginary;
    difference.UpdateRMSandPhase();
    return difference;
}
const phasor phasor::operator*(const phasor& rhs) const
{
    phasor product;
    product.RMSvalue = this->RMSvalue * rhs.RMSvalue;
    product.PhaseAngleDegrees = this->PhaseAngleDegrees + rhs.PhaseAngleDegrees;

    // Keep -180deg < phase_angle_degrees <= 180deg
    while (product.PhaseAngleDegrees > 180) product.PhaseAngleDegrees -= 360;
    while (product.PhaseAngleDegrees <= -180) product.PhaseAngleDegrees += 360;

    product.UpdateRealAndImag();
    return product;
}
const phasor phasor::operator/(const phasor& p2) const
{
    phasor quotient(0, 0);

    try {   // Handle the divide by zero exception
        if (p2.RMSvalue == 0) throw 360;

        quotient.RMSvalue = this->RMSvalue / p2.RMSvalue;
        quotient.PhaseAngleDegrees = this->PhaseAngleDegrees - p2.PhaseAngleDegrees;

        // Keep -180deg < phase_angle_degrees <= 180deg
        while (quotient.PhaseAngleDegrees > 180) quotient.PhaseAngleDegrees -= 360;
        while (quotient.PhaseAngleDegrees <= -180) quotient.PhaseAngleDegrees += 360;
    }
    catch (int e) {
        if (e == 360) cout << "Error: Divisor phasor is 0.\n";
        else cout << "Exception number " << e << " has occured.\n";
        quotient.RMSvalue = 0;
        quotient.PhaseAngleDegrees = 0;
    }

    quotient.UpdateRealAndImag();
    return quotient;
}
const phasor phasor::Pow(double power) const
{
    phasor exponent = phasor(0, 0);

    try {   // Handle the case with the 0 phasor to a nonpositive power
        if ((RMSvalue == 0) && (power <= 0)) throw 360;

        exponent.RMSvalue = pow(RMSvalue, power);
        exponent.PhaseAngleDegrees = PhaseAngleDegrees * power;

        // Keep -180deg < phase_angle_degrees <= 180deg
        while (exponent.PhaseAngleDegrees > 180) exponent.PhaseAngleDegrees -= 360;
        while (exponent.PhaseAngleDegrees <= -180) exponent.PhaseAngleDegrees += 360;
    }
    catch (int e) {
        if (e == 360) cout << "Error: Base is 0 and power is non-positive.\n";
        else cout << "Exception number " << e << " has occured.\n";
        exponent.RMSvalue = 0;
        exponent.PhaseAngleDegrees = 0;
    }

    exponent.UpdateRealAndImag();
    return exponent;
}