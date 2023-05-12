#pragma once

#define _USE_MATH_DEFINES

using namespace std;

#include <cmath>
#include <exception>
#include <iostream>
#include <string>
#include "phasor.h"
#include "instantaneousMeasurement.h"


/// <summary>
/// A parameter of interest in the power grid like a node voltage or line current.
/// </summary>
class parameter {
private:
    phasor CalculatePhasor();
    double RMS();
    double PhaseAngleDegrees(double rms);
    void FreeMemory();
    void MemoryAllocationFailure(string variableName);

public:
    /// <summary>
    /// A dynamically allocated array of the samples.
    /// </summary>
    instantaneousMeasurement* Samples = NULL;
    /// <summary>
    /// The number of samples allocated in 'samples'
    /// </summary>
    int NumberOfSamples = 0;
    /// <summary>
    /// The phasor representation of the data in 'samples'
    /// </summary>
    phasor Phasor = phasor(0, 0);
    /// <summary>
    /// The name of the parameter
    /// </summary>
    string Name = "";
    /// <summary>
    /// The base unit name
    /// </summary>
    string Units = "";
    /// <summary>
    /// The number of the starting node (0 is ground).
    /// </summary>
    int StartNodeNumber = 0;
    /// <summary>
    /// The number of the destination node (0 is ground).
    /// </summary>
    int DestinationNodeNumber = 0;

    parameter();
    parameter(instantaneousMeasurement* samples, int numberOfSamples, string name, string units,
        int startNodeNumber, int destinationNodeNumber);
    parameter(phasor phasorr, string name, string units, int startNodeNumber, int destinationNodeNumber);
    ~parameter();

    void PrintParameter();
};