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
#include "lineSample.h"


/// <summary>
/// Calculates and stores the distance of the known line from the unknown line
/// </summary>
class distanceSample {
private:
    /// <summary>
    /// A pointer to the line sample of interest with the known output
    /// </summary>
    lineSample* line = NULL;
    /// <summary>
    /// The weight of the line currents
    /// </summary>
    double wLine = 20;
    /// <summary>
    /// The weight of the node voltages
    /// </summary>
    double wNode = 4;
    /// <summary>
    /// The weight of the other currents flowing from the nodes not counting the line currents
    /// </summary>
    double wOther = 1;

    bool AreSamplesOfTheSameLine(lineSample* sampleWithKnownStatus, lineSample* sampleWithUnknownStatus);
    double CalculateDistance(lineSample* sampleWithKnownStatus, lineSample* sampleWithUnknownStatus);
public:
    /// <summary>
    /// The distance between the line with a known output and the line with the unknown output
    /// </summary>
    double Distance = 10000000000;
    /// <summary>
    /// The status of the known line
    /// </summary>
    bool IsWorking = true;

    distanceSample(lineSample* sampleWithKnownStatus, lineSample* sampleWithUnknownStatus);
    ~distanceSample();

    void Print();
};

