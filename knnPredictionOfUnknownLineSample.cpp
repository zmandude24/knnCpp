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
#include "knnPredictionOfUnknownLineSample.h"


/// <summary>
/// Sorts the array of distances when a new distance is added to the tail end of the list
/// </summary>
/// <param name="knownSampleIndex">The index of the knowns the constructor is currently at</param>
void knnPredictionOfUnknownLineSample::SortDistances(int knownSampleIndex) {
    // Place the new entry on the proper place on the array
    int lastNearestNeighborIndex = numberOfNearestNeighbors - 1;
    if (knownSampleIndex < numberOfNearestNeighbors - 1) lastNearestNeighborIndex = knownSampleIndex;

    for (int nearestNeighborsIndex = lastNearestNeighborIndex; nearestNeighborsIndex > 0; nearestNeighborsIndex--) {
        // If the distance is less than it's predescesor, swap their positions
        if (distances[nearestNeighborsIndex]->Distance < distances[nearestNeighborsIndex - 1]->Distance) {
            distanceSample* dummy = new distanceSample(SamplesWithKnownStatuses[knownSampleIndex], SampleWithUnknownStatus);
            *dummy = *distances[nearestNeighborsIndex - 1];
            *distances[nearestNeighborsIndex - 1] = *distances[nearestNeighborsIndex];
            *distances[nearestNeighborsIndex] = *dummy;
            delete dummy; dummy = NULL;
        }
    }
}

/// <summary>
/// Predicts the line status of the unknown line sample. It will return false in the case of a tie, but k is usually odd and
/// the statuses of the nearest neighbors are unanimous in virtually every case.
/// </summary>
/// <returns>The predicted status of the unknown line sample</returns>
bool knnPredictionOfUnknownLineSample::PredictStatus() {
    int numOfWorkingLines = 0;
    int numOfNotWorkingLines = 0;

    for (int nearestNeighborIndex = 0; nearestNeighborIndex < numberOfNearestNeighbors; nearestNeighborIndex++) {
        if (distances[nearestNeighborIndex]->IsWorking == true) numOfWorkingLines += 1;
        else numOfNotWorkingLines += 1;
    }

    if (numOfWorkingLines > numOfNotWorkingLines) return true;
    else return false;
}

void knnPredictionOfUnknownLineSample::SetDistances()
{
    if (numberOfNearestNeighbors > NumberOfKnownStatuses) {
        cout << "Error: The number of nearest neighbors is larger than the number of known statuses.\n";
        return;
    }

    if (distances != NULL) {
        for (int freeingIndex = 0; freeingIndex < numberOfNearestNeighbors; freeingIndex++) {
            if (distances[freeingIndex] != NULL) {
                delete distances[freeingIndex];
                distances[freeingIndex] = NULL;
            }
        }
        delete[] distances;
        distances = NULL;
    }

    distances = new distanceSample*[numberOfNearestNeighbors];
    if (distances == NULL) {
        MemoryAllocationFailure("distances");
        return;
    }
    for (int initializingIndex = 0; initializingIndex < numberOfNearestNeighbors; initializingIndex++) {
        distances[initializingIndex] = NULL;
    }

    for (int knownSampleIndex = 0; knownSampleIndex < NumberOfKnownStatuses; knownSampleIndex++) {
        // Special case where not all elements of distances are filled
        if (knownSampleIndex < numberOfNearestNeighbors) {
            distances[knownSampleIndex] = new distanceSample(SamplesWithKnownStatuses[knownSampleIndex], SampleWithUnknownStatus);
            if (distances[knownSampleIndex] == NULL) {
                MemoryAllocationFailure("distances[knownSampleIndex]");
                return;
            }
            if (knownSampleIndex > 0) {
                SortDistances(knownSampleIndex);
            }
        }

        else {
            distanceSample* distanceAtKnownSampleIndex =
                new distanceSample(SamplesWithKnownStatuses[knownSampleIndex], SampleWithUnknownStatus);
            if (distanceAtKnownSampleIndex == NULL) {
                MemoryAllocationFailure("distanceAtIndexI");
                return;
            }

            if (distanceAtKnownSampleIndex->Distance < distances[numberOfNearestNeighbors - 1]->Distance) {
                delete distances[numberOfNearestNeighbors - 1];
                distances[numberOfNearestNeighbors - 1] = distanceAtKnownSampleIndex;
                distanceAtKnownSampleIndex = NULL;
                SortDistances(knownSampleIndex);
            }
            else {
                delete distanceAtKnownSampleIndex;
                distanceAtKnownSampleIndex = NULL;
            }
        }
    }
}


/// <summary>
/// Free the dynamically allocated memory and set their pointers to NULL. Notice that only distanceSample** distances is freed.
/// </summary>
void knnPredictionOfUnknownLineSample::FreeMemory()
{
    if (distances != NULL) {
        for (int freeingIndex = 0; freeingIndex < numberOfNearestNeighbors; freeingIndex++) {
            if (distances[freeingIndex] != NULL) {
                delete distances[freeingIndex];
                distances[freeingIndex] = NULL;
            }
        }
        delete[] distances;
        distances = NULL;
    }
}

/// <summary>
/// Display an error message and call FreeMemory().
/// </summary>
/// <param=variableName>The name of the variable that failed to get memory allocation</param>
void knnPredictionOfUnknownLineSample::MemoryAllocationFailure(string variableName)
{
    cout << "Error: node() failed to allocate memory for " << variableName << "\n";
    FreeMemory();
}


/// <summary>
/// The constructor (default nearest neighbors is 5 and you have to set a different number with ChangeNumberOfNearestNeighbors())
/// </summary>
/// <param name="samplesWithKnownStatuses">The array of line samples with known line statuses</param>
/// <param name="numberOfKnownStatuses">The number of elements in the samplesWithKnownStatuses array</param>
/// <param name="sampleWithUnknownStatus">A line sample with an unknown line status</param>
knnPredictionOfUnknownLineSample::knnPredictionOfUnknownLineSample(lineSample** samplesWithKnownStatuses, int numberOfKnownStatuses,
    lineSample* sampleWithUnknownStatus)
{
    SamplesWithKnownStatuses = samplesWithKnownStatuses;
    NumberOfKnownStatuses = numberOfKnownStatuses;
    SampleWithUnknownStatus = sampleWithUnknownStatus;
    SetDistances();
    this->PredictedStatus = PredictStatus();
}

/// <summary>
/// The deconstructor
/// </summary>
knnPredictionOfUnknownLineSample::~knnPredictionOfUnknownLineSample()
{
    FreeMemory();
}


/// <summary>
/// Change the number of nearest neighbors and recalculate the nearest neighbors.
/// </summary>
/// <param name="numberOfNearestNeighbors">The new number of nearest neighbors</param>
void knnPredictionOfUnknownLineSample::ChangeNumberOfNearestNeighbors(int numberOfNearestNeighbors)
{
    // Don't bother recalculating if the number of nearest neighbors won't change.
    if (this->numberOfNearestNeighbors == numberOfNearestNeighbors) return;

    this->numberOfNearestNeighbors = numberOfNearestNeighbors;
    SetDistances();
}

/// <summary>
/// Print the distances of the nearest neighbors and the line status prediction.
/// </summary>
void knnPredictionOfUnknownLineSample::Print()
{
    cout << "\nKNN Algorithm:\n";
    for (int i = 0; i < numberOfNearestNeighbors; i++) cout << "distances[" << to_string(i) << "] distance: " <<
        to_string(distances[i]->Distance) << "\n";
    cout << "Line Status Prediction: " << to_string(PredictedStatus) << "\n";
}