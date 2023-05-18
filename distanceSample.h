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


    /// <summary>
    /// Verify if the line samples are samples of the same line.
    /// </summary>
    /// <param name="known">The line sample with the status known</param>
    /// <param name="unknown">The line sample with the status unknown</param>
    /// <returns>True if the samples are of the same line</returns>
    const bool AreSamplesOfTheSameLine(lineSample* sampleWithKnownStatus, lineSample* sampleWithUnknownStatus);

    /// <summary>
    /// Calculate the weighted euclidean distance between the parameters. This method assumes the two line samples were checked to be
    /// of the same line beforehand.
    /// </summary>
    /// <param name="sampleWithKnownStatus">The line sample with the line status known</param>
    /// <param name="sampleWithUnknownStatus">The line sample with the line status unknown</param>
    /// <returns>The weighted euclidean distance</returns>
    const double CalculateDistance(lineSample* sampleWithKnownStatus, lineSample* sampleWithUnknownStatus);


public:
    /// <summary>
    /// The distance between the line with a known output and the line with the unknown output
    /// </summary>
    double Distance = 1000000000;
    /// <summary>
    /// The status of the known line
    /// </summary>
    bool IsWorking = true;


    /// <summary>
    /// The constructor
    /// </summary>
    /// <param name="sampleWithKnownStatus">the line sample with a known line status</param>
    /// <param name="sampleWithUnknownStatus">the line sample with an unknown line status</param>
    distanceSample(lineSample* sampleWithKnownStatus, lineSample* sampleWithUnknownStatus);

    /// <summary>
    /// The deconstructor (frees nothing)
    /// </summary>
    ~distanceSample();

    /// <summary>
    /// Print the attached known line, weights, distance, and the status of the known line.
    /// </summary>
    const void Print();
};