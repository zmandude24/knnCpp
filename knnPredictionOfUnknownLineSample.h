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
#include "distanceSample.h"


/// <summary>
/// This contains a set of known line samples, an unknown line sample, and predicts the status of the unknown line sample.
/// </summary>
class knnPredictionOfUnknownLineSample {
private:
    /// <summary>
    /// An array containing the k closest line samples and their distances with the closest having the lowest index
    /// </summary>
    distanceSample** distances = NULL;
    /// <summary>
    /// The number of nearest neighbors to consider
    /// </summary>
    int numberOfNearestNeighbors = 5;

    void SortDistances(int knownSampleIndex);
    bool PredictStatus();

    void FreeMemory();
    void MemoryAllocationFailure(string variableName);
    void SetDistances();
public:
    /// <summary>
    /// The array of line samples with a known line status
    /// </summary>
    lineSample** SamplesWithKnownStatuses = NULL;
    /// <summary>
    /// The number of known line samples
    /// </summary>
    int NumberOfKnownStatuses = 0;
    /// <summary>
    /// The line sample with an unknown line status
    /// </summary>
    lineSample* SampleWithUnknownStatus = NULL;
    /// <summary>
    /// The predicted status
    /// </summary>
    bool PredictedStatus = true;

    knnPredictionOfUnknownLineSample(lineSample** samplesWithKnownStatuses, int numberOfKnownStatuses,
        lineSample* sampleWithUnknownStatus, int numberOfNearestNeighbors = 5);
    ~knnPredictionOfUnknownLineSample();

    void ChangeNumberOfNearestNeighbors(int numberOfNearestNeighbors);
    void Print();
};

