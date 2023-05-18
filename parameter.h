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
    /// <summary>
    /// Find the phasor representation of the data in 'samples' that is assumed to be perfectly sinusoidal.
    /// </summary>
    const phasor CalculatePhasor();
    /// <summary>
    /// The Root Mean Square is calculated with the formula RMS = sqrt(1/n * SUM(xi^2)) where n is the number of samples and xi is
    /// the ith sample.
    /// </summary>
    /// <returns> The RMS value of the values in 'samples'</returns>
    const double RMS();
    /// <summary>
    /// The phase angle is calculated from the first data point or time = 0 by finding if the sine wave is increasing or decreasing and 
    /// then calculating the arcsine while adjusting if the phase angle is in the second or third quadrant.
    /// </summary>
    /// <param name="rms">The RMS value of the values found by RMS()</param>
    /// <returns>The phase angle of the phasor representation of the values in 'samples' in degrees</returns>
    const double PhaseAngleDegrees(double rms);
    
    
    /// <summary>
    /// Free the dynamically allocated memory and set their pointers to NULL.
    /// </summary>
    const void FreeMemory();

    /// <summary>
    /// Display an error message and call FreeMemory().
    /// </summary>
    /// <param=variableName>The name of the variable that failed to get memory allocation</param>
    const void MemoryAllocationFailure(string variableName);


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


    /// <summary>
    /// The default constructor setting all values to their defaults
    /// </summary>
    parameter();
    /// <summary>
    /// This constructor calculates the phasor from the array of instantaneous measurements.
    /// </summary>
    /// <param name="samples">The array of instantaneous measurements</param>
    /// <param name="numberOfSamples">The number of elements in the array or instantaneous measurements</param>
    /// <param name="name">The name to be used for the parameter</param>
    /// <param name="units">The base unit suffix</param>
    /// <param name="startNodeNumber">The starting node number (0 is ground)</param>
    /// <param name="destinationNodeNumber">The destination node number (0 is ground)</param>
    explicit parameter(instantaneousMeasurement* samples, int numberOfSamples, string name, string units,
        int startNodeNumber, int destinationNodeNumber);
    /// <summary>
    /// This constructor doesn't involve a set of instantaneous measurements.
    /// </summary>
    /// <param name="phasorr">The phasor representation of the parameter</param>
    /// <param name="name">The name to be used for the parameter</param>
    /// <param name="units">The base unit suffix</param>
    /// <param name="startNodeNumber">The starting node number (0 is ground)</param>
    /// <param name="destinationNodeNumber">The destination node number (0 is ground)</param>
    explicit parameter(phasor phasorr, string name, string units, int startNodeNumber, int destinationNodeNumber);

    /// <summary>
    /// This will free everything in its pointers
    /// </summary>
    ~parameter();


    /// <summary>
    /// Print the name, number of samples, phasor, and the starting and destination node numbers.
    /// </summary>
    const void PrintParameter();
};