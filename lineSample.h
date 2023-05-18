#pragma once

#define _USE_MATH_DEFINES

using namespace std;

#include <cmath>
#include <exception>
#include <iostream>
#include <string>
#include "phasor.h"
#include "parameter.h"
#include "nodeSample.h"


/// <summary>
/// This contains a pointer to each node of interest as well as if the line is working or not.
/// </summary>
class lineSample {
private:
    /// <summary>
    /// The first node the line is connected to
    /// </summary>
    shared_ptr<nodeSample> node1 = NULL;
    /// <summary>
    /// The second node the line is connected to
    /// </summary>
    shared_ptr<nodeSample> node2 = NULL;


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
    /// The normalized (magnitude divided by the current rating) current flowing through the line from node 1
    /// </summary>
    parameter* Node1LineCurrentNorm = NULL;
    /// <summary>
    /// The normalized (magnitude divided by the current rating) current flowing through the line from node 2
    /// </summary>
    parameter* Node2LineCurrentNorm = NULL;

    /// <summary>
    /// The normalized (magnitude divided by the voltage rating) voltage at node 1
    /// </summary>
    parameter* Node1VoltageNorm = NULL;
    /// <summary>
    /// The normalized (magnitude divided by the voltage rating) voltage at node 2
    /// </summary>
    parameter* Node2VoltageNorm = NULL;

    /// <summary>
    /// The normalized (magnitude divided by the current rating) currents flowing from node 1 not counting the line current
    /// </summary>
    parameter** Node1OtherCurrentsNorm = NULL;
    /// <summary>
    /// The number of currents flowing from node 1 not counting the line current
    /// </summary>
    int NumberOfNode1OtherCurrents = 0;
    /// <summary>
    /// The normalized (magnitude divided by the current rating) currents flowing from node 2 not counting the line current
    /// </summary>
    parameter** Node2OtherCurrentsNorm = NULL;
    /// <summary>
    /// The number of currents flowing from node 2 not counting the line current
    /// </summary>
    int NumberOfNode2OtherCurrents = 0;

    /// <summary>
    /// The status of the line
    /// </summary>
    bool IsWorking = true;


    /// <summary>
    /// The constructor
    /// </summary>
    /// <param name="node1">The first node of the line</param>
    /// <param name="node2">The second node of the line</param>
    /// <param name="isWorking">The status of the line</param>
    explicit lineSample(shared_ptr<nodeSample> node1, shared_ptr<nodeSample> node2, bool isWorking);

    /// <summary>
    /// The deconstructor
    /// </summary>
    ~lineSample();
    

    /// <summary>
    /// Print the nodes, line status, and normalized parameters.
    /// </summary>
    const void PrintLine();
};