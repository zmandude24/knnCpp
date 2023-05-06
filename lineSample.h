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
    nodeSample* node1 = NULL;
    /// <summary>
    /// The second node the line is connected to
    /// </summary>
    nodeSample* node2 = NULL;

    void FreeMemory();
    void MemoryAllocationFailure(string variableName);
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

    lineSample(nodeSample* node1, nodeSample* node2, bool isWorking);
    ~lineSample();
    
    void PrintLine();
};

