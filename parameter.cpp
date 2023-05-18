#define _USE_MATH_DEFINES

using namespace std;

#include <cmath>
#include <exception>
#include <iostream>
#include <string>
#include "phasor.h"
#include "instantaneousMeasurement.h"
#include "parameter.h"


const phasor parameter::CalculatePhasor()
{
    if (NumberOfSamples <= 1) {
        cout << "Insufficient number of samples to calculate a phasor.\n";
        return phasor();
    }
    return phasor(RMS(), PhaseAngleDegrees(RMS()));
}
const double parameter::RMS()
{
    double rms = 0;
    for (int sampleIndex = 0; sampleIndex < NumberOfSamples; sampleIndex++) rms += pow(Samples[sampleIndex].value, 2);
    if (NumberOfSamples > 0) rms = sqrt(rms / NumberOfSamples);
    return rms;
}
const double parameter::PhaseAngleDegrees(double rms)
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


const void parameter::FreeMemory()
{
    if (Samples != NULL) {
        delete[] Samples;
        Samples = NULL;
    }
}

const void parameter::MemoryAllocationFailure(string variableName)
{
    cout << "Error: node() failed to allocate memory for " << variableName << "\n";
    FreeMemory();
}



parameter::parameter() {}
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

parameter::~parameter()
{
    FreeMemory();
}


const void parameter::PrintParameter()
{
    cout << "\nName: " << Name << "\n";
    cout << "Number of samples: " << NumberOfSamples << "\n";
    cout << "Phasor: " << Phasor.PhasorToString() << Units << "\n";
    cout << "Starting Node: " << to_string(StartNodeNumber) + "\n";
    cout << "Destination Node: " << to_string(DestinationNodeNumber) + "\n";
}