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


    /// <summary>
    /// Create the pointer array of distances.
    /// </summary>
    const void SetDistances();
    /// <summary>
    /// Sorts the array of distances when a new distance is added to the tail end of the list
    /// </summary>
    /// <param name="knownSampleIndex">The index of the knowns the constructor is currently at</param>
    const void SortDistances(int knownSampleIndex);
    /// <summary>
    /// Predicts the line status of the unknown line sample. It will return false in the case of a tie, but k is usually odd and
    /// the statuses of the nearest neighbors are unanimous in virtually every case.
    /// </summary>
    /// <returns>The predicted status of the unknown line sample</returns>
    const bool PredictStatus();

    /// <summary>
    /// Free the dynamically allocated memory and set their pointers to NULL. Notice that only distanceSample** distances is freed.
    /// </summary>
    const void FreeMemory();

    /// <summary>
    /// Display an error message and call FreeMemory().
    /// </summary>
    /// <param=variableName>The name of the variable that failed to get memory allocation</param>
    const void MemoryAllocationFailure(string variableName);


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


    /// <summary>
    /// The constructor (default nearest neighbors is 5 and you have to set a different number with ChangeNumberOfNearestNeighbors())
    /// </summary>
    /// <param name="samplesWithKnownStatuses">The array of line samples with known line statuses</param>
    /// <param name="numberOfKnownStatuses">The number of elements in the samplesWithKnownStatuses array</param>
    /// <param name="sampleWithUnknownStatus">A line sample with an unknown line status</param>
    /// <param name="numberOfNearestNeighbors">
    /// The number of nearest neighbors the unknown sample is compared to for the prediction</param>
    explicit knnPredictionOfUnknownLineSample(lineSample** samplesWithKnownStatuses, int numberOfKnownStatuses,
        lineSample* sampleWithUnknownStatus, int numberOfNearestNeighbors = 5);

    /// <summary>
    /// The deconstructor
    /// </summary>
    ~knnPredictionOfUnknownLineSample();


    /// <summary>
    /// Change the number of nearest neighbors and recalculate the nearest neighbors.
    /// </summary>
    /// <param name="numberOfNearestNeighbors">The new number of nearest neighbors</param>
    const void ChangeNumberOfNearestNeighbors(int numberOfNearestNeighbors);
    
    
    /// <summary>
    /// Print the distances of the nearest neighbors and the line status prediction.
    /// </summary>
    const void Print();
};