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
    /// <summary>
    /// This constructor doesn't involve a set of instantaneous measurements.
    /// </summary>
    /// <param name="phasorr">The phasor representation of the parameter</param>
    /// <param name="name">The name to be used for the parameter</param>
    /// <param name="units">The base unit suffix</param>
    /// <param name="startNodeNumber">The starting node number (0 is ground)</param>
    /// <param name="destinationNodeNumber">The destination node number (0 is ground)</param>
    parameter(phasor phasorr, string name = "", string units = "",
        int startNodeNumber = 0, int destinationNodeNumber = 0)
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
    ~parameter()
    {
        FreeMemory();
    }

    /// <summary>
    /// Print the name, number of samples, phasor, and the starting and destination node numbers.
    /// </summary>
    void PrintParameter()
    {
        cout << "\nName: " << Name << "\n";
        cout << "Number of samples: " << NumberOfSamples << "\n";
        cout << "Phasor: " << Phasor.PhasorToString() << Units << "\n";
        cout << "Starting Node: " << to_string(StartNodeNumber) + "\n";
        cout << "Destination Node: " << to_string(DestinationNodeNumber) + "\n";
    }
};