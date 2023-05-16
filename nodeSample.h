#pragma once

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
/// The data representation of a point on the grid in a specified time range with the voltage and the currents coming from it.
/// </summary>
class nodeSample {
private:
    /// <summary>
    /// The default rated voltage
    /// </summary>
    const double defaultRatedVoltage = 250000;
    /// <summary>
    /// The default rated current
    /// </summary>
    const double defaultRatedCurrent = 25;

    void FreeMemory();
    void MemoryAllocationFailure(string variableName);
public:
    /// <summary>
    /// The unique identifying number for the node
    /// </summary>
    int NodeNumber = 0;
    /// <summary>
    /// The pointer to the node voltage (startNodeNum = nodeNum and destNodeNum = 0)
    /// </summary>
    parameter* Voltage = NULL;
    /// <summary>
    /// The pointer to the currents (startNodeNum = nodeNum)
    /// </summary>
    parameter** Currents = NULL;
    /// <summary>
    /// The number of currents the dynamically allocated array 'currents' has
    /// </summary>
    int NumberOfCurrents = 0;
    /// <summary>
    /// The rated voltage (typically the voltage at peak normal usage of the grid)
    /// </summary>
    double RatedVoltage = defaultRatedVoltage;
    /// <summary>
    /// The rated current (typically the magnitude for a two line node at peak normal usage of the grid)
    /// </summary>
    double RatedCurrent = defaultRatedCurrent;

    explicit nodeSample(int nodeNumber, parameter* voltage, parameter** currents, int numberOfCurrents);
    explicit nodeSample(int nodeNumber, phasor voltage, phasor* currents, int* currentDestinationNodes, int numberOfCurrents);
    explicit nodeSample(int nodeNumber, phasor voltage, phasor current1, int current1DestinationNode,
        phasor current2, int current2DestinationNode);
    explicit nodeSample(int nodeNumber, phasor voltage, phasor current1, int current1DestinationNode,
        phasor current2, int current2DestinationNode, phasor current3, int current3DestinationNode);
    ~nodeSample();

    void PrintNode();
};