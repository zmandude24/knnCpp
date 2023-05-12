#define _USE_MATH_DEFINES

using namespace std;

#include <cmath>
#include <exception>
#include <iostream>
#include <string>
#include "phasor.h"
#include "instantaneousMeasurement.h"
#include "parameter.h"

/// <summary>
/// Find the phasor representation of the data in 'samples' that is assumed to be perfectly sinusoidal.
/// </summary>
phasor parameter::CalculatePhasor()
{
    if (NumberOfSamples <= 1) {
        cout << "Insufficient number of samples to calculate a phasor.\n";
        return phasor();
    }
    return phasor(RMS(), PhaseAngleDegrees(RMS()));
}

/// <summary>
/// The Root Mean Square is calculated with the formula RMS = sqrt(1/n * SUM(xi^2)) where n is the number of samples and xi is
/// the ith sample.
/// </summary>
/// <returns> The RMS value of the values in 'samples'</returns>
double parameter::RMS()
{
    double rms = 0;
    for (int sampleIndex = 0; sampleIndex < NumberOfSamples; sampleIndex++) rms += pow(Samples[sampleIndex].value, 2);
    if (NumberOfSamples > 0) rms = sqrt(rms / NumberOfSamples);
    return rms;
}

/// <summary>
/// The phase angle is calculated from the first data point or time = 0 by finding if the sine wave is increasing or decreasing and 
/// then calculating the arcsine while adjusting if the phase angle is in the second or third quadrant.
/// </summary>
/// <param name="rms">The RMS value of the values found by RMS()</param>
/// <returns>The phase angle of the phasor representation of the values in 'samples' in degrees</returns>
double parameter::PhaseAngleDegrees(double rms)
{
    double phaseAngleDegrees = 0;

    // Count the phase angle as 90 or -90 if the first point's magnitude is greater than the peak.
    if (Samples[0].value >= rms * sqrt(2)) phaseAngleDegrees = 90;
    else if (Samples[0].value <= -rms * sqrt(2)) phaseAngleDegrees = -90;

    // Ascending
    else if (Samples[1].value >= Samples[0].value) {
        phaseAngleDegrees = 180 / M_PI * asin(Samples[0].value / (rms * sqrt(2)));
    }

    // Descending
    else {
        // Belongs in Q2
        if (Samples[0].value >= 0) {
            phaseAngleDegrees = 180 - 180 / M_PI * asin(Samples[0].value / (rms * sqrt(2)));
        }

        // Belongs in Q4
        else phaseAngleDegrees = -180 - 180 / M_PI * asin(Samples[0].value / (rms * sqrt(2)));
    }

    return phaseAngleDegrees;
}


/// <summary>
/// Free the dynamically allocated memory and set their pointers to NULL.
/// </summary>
void parameter::FreeMemory()
{
    if (Samples != NULL) {
        delete[] Samples;
        Samples = NULL;
    }
}

/// <summary>
/// Display an error message and call FreeMemory().
/// </summary>
/// <param=variableName>The name of the variable that failed to get memory allocation</param>
void parameter::MemoryAllocationFailure(string variableName)
{
    cout << "Error: node() failed to allocate memory for " << variableName << "\n";
    FreeMemory();
}


/// <summary>
/// The default constructor setting all values to their defaults
/// </summary>
parameter::parameter() {}
/// <summary>
/// This constructor calculates the phasor from the array of instantaneous measurements.
/// </summary>
/// <param name="samples">The array of instantaneous measurements</param>
/// <param name="numberOfSamples">The number of elements in the array or instantaneous measurements</param>
/// <param name="name">The name to be used for the parameter</param>
/// <param name="units">The base unit suffix</param>
/// <param name="startNodeNumber">The starting node number (0 is ground)</param>
/// <param name="destinationNodeNumber">The destination node number (0 is ground)</param>
parameter::parameter(instantaneousMeasurement* samples, int numberOfSamples, string name = "", string units = "",
    int startNodeNumber = 0, int destinationNodeNumber = 0)
{
    Samples = samples;
    NumberOfSamples = numberOfSamples;
    Phasor = CalculatePhasor();
    Name = name;
    Units = units;
    StartNodeNumber = startNodeNumber;
    DestinationNodeNumber = destinationNodeNumber;
}
/// <summary>
/// This constructor doesn't involve a set of instantaneous measurements.
/// </summary>
/// <param name="phasorr">The phasor representation of the parameter</param>
/// <param name="name">The name to be used for the parameter</param>
/// <param name="units">The base unit suffix</param>
/// <param name="startNodeNumber">The starting node number (0 is ground)</param>
/// <param name="destinationNodeNumber">The destination node number (0 is ground)</param>
parameter::parameter(phasor phasorr, string name = "", string units = "", int startNodeNumber = 0, int destinationNodeNumber = 0)
{
    Samples = NULL;
    NumberOfSamples = 0;
    Phasor = phasorr;
    Name = name;
    Units = units;
    StartNodeNumber = startNodeNumber;
    DestinationNodeNumber = destinationNodeNumber;
}

/// <summary>
/// This will free everything in its pointers
/// </summary>
parameter::~parameter()
{
    FreeMemory();
}


/// <summary>
/// Print the name, number of samples, phasor, and the starting and destination node numbers.
/// </summary>
void parameter::PrintParameter()
{
    cout << "\nName: " << Name << "\n";
    cout << "Number of samples: " << NumberOfSamples << "\n";
    cout << "Phasor: " << Phasor.PhasorToString() << Units << "\n";
    cout << "Starting Node: " << to_string(StartNodeNumber) + "\n";
    cout << "Destination Node: " << to_string(DestinationNodeNumber) + "\n";
}