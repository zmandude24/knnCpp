#define _USE_MATH_DEFINES

using namespace std;

#include <cmath>
#include <exception>
#include <iostream>
#include <string>
#include "phasor.h"


/// <summary>
/// Calculate the real and imaginary parts of the complex number in cartesian form.
/// </summary>
void phasor::UpdateRealAndImag() 
{
    real = RMSvalue * cos(M_PI / 180 * PhaseAngleDegrees);
    imaginary = RMSvalue * sin(M_PI / 180 * PhaseAngleDegrees);
}

/// <summary>
/// Calculate the rms value and phase angle of the complex number.
/// </summary>
void phasor::UpdateRMSandPhase()
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

/// <summary>
/// Set variables to default values.
/// </summary>
phasor::phasor() {}
/// <summary>
/// Initialize the polar parameters and calculate the cartesian parameters from those.
/// </summary>
/// <param name="rmsValue">The rms value of the phasor (use base units)</param>
/// <param name="phaseAngleDegrees">The phase angle of the phasor in degrees</param>
phasor::phasor(double rmsValue, double phaseAngleDegrees)
{
    RMSvalue = rmsValue;
    PhaseAngleDegrees = phaseAngleDegrees;
    UpdateRealAndImag();
}

/// <summary>
/// Print the phasor in polar form.
/// </summary>
void phasor::Print()
{
    cout << RMSvalue << " @ " << PhaseAngleDegrees << "deg\n";
}
/// <summary>
/// Returns a string in the same format PrintPhasor() prints to the terminal (without the new line character at the end)
/// </summary>
/// <returns>The phasor in printable string form</returns>
string phasor::PhasorToString()
{
    return to_string(RMSvalue) + " @ " + to_string(PhaseAngleDegrees) + "deg";
}

/// <summary>
/// Use '+' to add two phasors together like you would with two numbers.
/// </summary>
const phasor phasor::operator+(const phasor& p2)
{
    phasor sum;
    sum.real = this->real + p2.real;
    sum.imaginary = this->imaginary + p2.imaginary;
    sum.UpdateRMSandPhase();
    return sum;
}
/// <summary>
/// Use '-' to subtract the right phasor from the left phasor like you would with two numbers.
/// </summary>
const phasor phasor::operator-(const phasor& p2)
{
    phasor difference;
    difference.real = this->real - p2.real;
    difference.imaginary = this->imaginary - p2.imaginary;
    difference.UpdateRMSandPhase();
    return difference;
}
/// <summary>
/// Use '*' to multiply two phasors like you would with two numbers.
/// </summary>
const phasor phasor::operator*(const phasor& p2)
{
    phasor product;
    product.RMSvalue = this->RMSvalue * p2.RMSvalue;
    product.PhaseAngleDegrees = this->PhaseAngleDegrees + p2.PhaseAngleDegrees;

    // Keep -180deg < phase_angle_degrees <= 180deg
    while (product.PhaseAngleDegrees > 180) product.PhaseAngleDegrees -= 360;
    while (product.PhaseAngleDegrees <= -180) product.PhaseAngleDegrees += 360;

    product.UpdateRealAndImag();
    return product;
}
/// <summary>
/// Use '/' to divide the left phasor by the right phasor like you would with two numbers. In the division by zero case, an
/// error message will be printed and the zero phasor will be returned.
/// </summary>
const phasor phasor::operator/(const phasor& p2)
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
/// <summary>
/// Raise a phasor to a power. In the zero phasor by a non-zero power case, an error message is printed and the zero phasor is
/// returned.
/// </summary>
/// <param name="base">The phasor to be raised to a power</param>
/// <param name="power">The exponent or power to raise the phasor to</param>
/// <returns>The phasor with the power applied</returns>
const phasor phasor::Pow(phasor base, double power)
{
    phasor exponent = phasor(0, 0);

    try {   // Handle the case with the 0 phasor to a nonpositive power
        if ((base.RMSvalue == 0) && (power <= 0)) throw 360;

        exponent.RMSvalue = pow(base.RMSvalue, power);
        exponent.PhaseAngleDegrees = base.PhaseAngleDegrees * power;

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