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


    /// <summary>
    /// This constructor uses previously allocated parameters that will be freed on the deconstructor.
    /// </summary>
    /// <param name="nodeNumber">The unique identifying number for the node</param>
    /// <param name="voltage">The pointer to the node voltage (startNodeNum = nodeNum and destNodeNum = 0)</param>
    /// <param name="currents">The pointer to the currents (startNodeNum = nodeNum)</param>
    /// <param name="numberOfCurrents">The number of currents the dynamically allocated array 'currents' has</param>
    explicit nodeSample(int nodeNumber, parameter* voltage, parameter** currents, int numberOfCurrents);
    /// <summary>
    /// This constructor uses phasors and an array of current destination node numbers in place of the parameter pointers.
    /// </summary>
    /// <param name="nodeNumber">The unique identifying number for the node</param>
    /// <param name="voltage">The node voltage phasor</param>
    /// <param name="currents">The array of current phasors</param>
    /// <param name="currentDestinationNodes">The array of current destination node numbers</param>
    /// <param name="numberOfCurrents">The number of currents the dynamically allocated array 'currents' has</param>
    /// <param name="ratedVoltage">The rated voltage (typically the voltage at peak normal usage of the grid)</param>
    /// <param name="ratedCurrent">
    /// The rated current (typically the magnitude for a two line node at peak normal usage of the grid) </param>
    explicit nodeSample(int nodeNumber, phasor voltage, phasor* currents, int* currentDestinationNodes, int numberOfCurrents);
    /// <summary>
    /// A specific constructor for a node with two currents
    /// </summary>
    /// <param name="nodeNumber">The unique identifying number for the node</param>
    /// <param name="voltage">The voltage phasor for the node</param>
    /// <param name="current1">The first current phasor</param>
    /// <param name="current1DestinationNode">The first current's destination node number</param>
    /// <param name="current2">The second current phasor</param>
    /// <param name="current2DestinationNode">The second current's destination node number</param>
    explicit nodeSample(int nodeNumber, phasor voltage, phasor current1, int current1DestinationNode,
        phasor current2, int current2DestinationNode);
    /// <summary>
    /// A specific constructor for a node with three currents
    /// </summary>
    /// <param name="nodeNumber">The unique identifying number for the node</param>
    /// <param name="voltage">The voltage phasor for the node</param>
    /// <param name="current1">The first current phasor</param>
    /// <param name="current1DestinationNode">The first current's destination node number</param>
    /// <param name="current2">The second current phasor</param>
    /// <param name="current2DestinationNode">The second current's destination node number</param>
    /// <param name="current3">The third current phasor</param>
    /// <param name="current3DestinationNode">The third current's destination node number</param>
    explicit nodeSample(int nodeNumber, phasor voltage, phasor current1, int current1DestinationNode,
        phasor current2, int current2DestinationNode, phasor current3, int current3DestinationNode);

    /// <summary>
    /// This will free everything in its pointers
    /// </summary>
    ~nodeSample();


    /// <summary>
    /// Prints the node number, voltage phasor, and current phasors
    /// </summary>
    const void PrintNode();
};