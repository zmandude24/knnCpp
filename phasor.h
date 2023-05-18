#pragma once

#define _USE_MATH_DEFINES

using namespace std;

#include <cmath>
#include <exception>
#include <iostream>
#include <string>
#include "phasor.h"

/// <summary>
/// This is the complex number representation of a sinusoidal signal.
/// </summary>
class phasor {
private:
    /// <summary>
    /// The real part of the complex number in cartesian form
    /// </summary>
    double real = 0;
    /// <summary>
    /// The imaginary part of the complex number in cartesian form
    /// </summary>
    double imaginary = 0;


    /// <summary>
    /// Calculate the real and imaginary parts of the complex number in cartesian form.
    /// </summary>
    const void UpdateRealAndImag();

    /// <summary>
    /// Calculate the rms value and phase angle of the complex number.
    /// </summary>
    const void UpdateRMSandPhase();


public:
    /// <summary>
    /// The RMS (Root Mean Square) of the sinusoidal waveform
    /// </summary>
    double RMSvalue = 0;
    /// <summary>
    /// The offset angle of the sinusoidal waveform in degrees
    /// </summary>
    double PhaseAngleDegrees = 0;


    /// <summary>
    /// Set variables to default values.
    /// </summary>
    phasor();
    /// <summary>
    /// Initialize the polar parameters and calculate the cartesian parameters from those.
    /// </summary>
    /// <param name="rmsValue">The rms value of the phasor (use base units)</param>
    /// <param name="phaseAngleDegrees">The phase angle of the phasor in degrees</param>
    explicit phasor(double rmsValue, double phaseAngleDegrees);


    /// <summary>
    /// Print the phasor in polar form.
    /// </summary>
    const void Print();
    /// <summary>
    /// Returns a string in the same format PrintPhasor() prints to the terminal (without the new line character at the end)
    /// </summary>
    /// <returns>The phasor in printable string form</returns>
    const string PhasorToString();

    
    /// <summary>
    /// The phasor addition operator (this class phasor is the one to the left of the operator)
    /// </summary>
    /// <param name="rhs">The phasor to the right of the operator</param>
    /// <returns>The sum of the left and right hand phasors</returns>
    const phasor operator+(const phasor& rhs) const;
    /// <summary>
    /// The phasor subtraction operator (this class phasor is the one to the left of the operator)
    /// </summary>
    /// <param name="rhs">The phasor to the right of the operator</param>
    /// <returns>The difference of the left and right hand phasors</returns>
    const phasor operator-(const phasor& rhs) const;
    /// <summary>
    /// The phasor multiplication operator (this class phasor is the one to the left of the operator)
    /// </summary>
    /// <param name="rhs">The phasor to the right of the operator</param>
    /// <returns>The product of the left and right hand phasors</returns>
    const phasor operator*(const phasor& rhs) const;
    /// <summary>
    /// The phasor division operator (this class phasor is the one to the left of the operator) that also returns the default phasor
    /// in the case of the divide by zero exception.
    /// </summary>
    /// <param name="rhs">The phasor to the right of the operator</param>
    /// <returns>The quotient of the left and right hand phasors</returns>
    const phasor operator/(const phasor& rhs) const;
    /// <summary>
    /// The phasor exponent method that also returns the default phasor in the case of a zero phasor to a non-positive power.
    /// </summary>
    /// <param name="power">The power to raise the phasor to</param>
    /// <returns>The phasor to the power of 'power'</returns>
    const phasor Pow(double power) const;
};