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

    void UpdateRealAndImag();
    void UpdateRMSandPhase();

public:
    /// <summary>
    /// The RMS (Root Mean Square) of the sinusoidal waveform
    /// </summary>
    double RMSvalue = 0;
    /// <summary>
    /// The offset angle of the sinusoidal waveform in degrees
    /// </summary>
    double PhaseAngleDegrees = 0;

    phasor();
    phasor(double rmsValue, double phaseAngleDegrees);

    void Print();
    string PhasorToString();

    phasor operator+(const phasor& p2);
    phasor operator-(const phasor& p2);
    phasor operator*(const phasor& p2);
    phasor operator/(const phasor& p2);
    phasor Pow(phasor base, double power);
};