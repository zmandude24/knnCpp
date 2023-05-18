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


const bool distanceSample::AreSamplesOfTheSameLine(lineSample* sampleWithKnownStatus, lineSample* sampleWithUnknownStatus)
{
    bool areSamplesOfTheSameLine = true;

    if ((sampleWithKnownStatus == NULL) || (sampleWithUnknownStatus == NULL)) {
        if (sampleWithKnownStatus == NULL) {
            cout << "Error in distanceSample(): lineSample* sampleWithKnownStatus = NULL!\n";
        }
        if (sampleWithUnknownStatus == NULL) {
            cout << "Error in distanceSample(): lineSample* sampleWithUnknownStatus = NULL!\n";
        }
        return false;
    }

    // Check Line Currents
    if (sampleWithKnownStatus->Node1LineCurrentNorm->StartNodeNumber !=
        sampleWithUnknownStatus->Node1LineCurrentNorm->StartNodeNumber) areSamplesOfTheSameLine = false;
    if (sampleWithKnownStatus->Node1LineCurrentNorm->DestinationNodeNumber !=
        sampleWithUnknownStatus->Node1LineCurrentNorm->DestinationNodeNumber) areSamplesOfTheSameLine = false;
    if (sampleWithKnownStatus->Node2LineCurrentNorm->StartNodeNumber !=
        sampleWithUnknownStatus->Node2LineCurrentNorm->StartNodeNumber) areSamplesOfTheSameLine = false;
    if (sampleWithKnownStatus->Node2LineCurrentNorm->DestinationNodeNumber !=
        sampleWithUnknownStatus->Node2LineCurrentNorm->DestinationNodeNumber) areSamplesOfTheSameLine = false;

    // Check Node Voltages
    if (sampleWithKnownStatus->Node1VoltageNorm->StartNodeNumber !=
        sampleWithUnknownStatus->Node1VoltageNorm->StartNodeNumber) areSamplesOfTheSameLine = false;
    if (sampleWithKnownStatus->Node2VoltageNorm->StartNodeNumber !=
        sampleWithUnknownStatus->Node2VoltageNorm->StartNodeNumber) areSamplesOfTheSameLine = false;

    // Check Other Currents
    if (sampleWithKnownStatus->NumberOfNode1OtherCurrents !=
        sampleWithUnknownStatus->NumberOfNode1OtherCurrents) areSamplesOfTheSameLine = false;
    else for (int comparingIndex = 0; comparingIndex < sampleWithKnownStatus->NumberOfNode1OtherCurrents; comparingIndex++) {
        if (sampleWithKnownStatus->Node1OtherCurrentsNorm[comparingIndex]->StartNodeNumber !=
            sampleWithUnknownStatus->Node1OtherCurrentsNorm[comparingIndex]->StartNodeNumber) {
            areSamplesOfTheSameLine = false;
            break;
        }
        if (sampleWithKnownStatus->Node1OtherCurrentsNorm[comparingIndex]->DestinationNodeNumber !=
            sampleWithUnknownStatus->Node1OtherCurrentsNorm[comparingIndex]->DestinationNodeNumber) {
            areSamplesOfTheSameLine = false;
            break;
        }
    }
    if (sampleWithKnownStatus->NumberOfNode2OtherCurrents !=
        sampleWithUnknownStatus->NumberOfNode2OtherCurrents) areSamplesOfTheSameLine = false;
    else for (int comparingIndex = 0; comparingIndex < sampleWithKnownStatus->NumberOfNode2OtherCurrents; comparingIndex++) {
        if (sampleWithKnownStatus->Node2OtherCurrentsNorm[comparingIndex]->StartNodeNumber !=
            sampleWithUnknownStatus->Node2OtherCurrentsNorm[comparingIndex]->StartNodeNumber) {
            areSamplesOfTheSameLine = false;
            break;
        }
        if (sampleWithKnownStatus->Node2OtherCurrentsNorm[comparingIndex]->DestinationNodeNumber !=
            sampleWithUnknownStatus->Node2OtherCurrentsNorm[comparingIndex]->DestinationNodeNumber) {
            areSamplesOfTheSameLine = false;
            break;
        }
    }

    return areSamplesOfTheSameLine;
}

const double distanceSample::CalculateDistance(lineSample* sampleWithKnownStatus, lineSample* sampleWithUnknownStatus)
{
    double dist = 0;

    // The line currents
    dist += wLine / 2 * pow((sampleWithKnownStatus->Node1LineCurrentNorm->Phasor -
        sampleWithUnknownStatus->Node1LineCurrentNorm->Phasor).RMSvalue, 2);
    dist += wLine / 2 * pow((sampleWithKnownStatus->Node2LineCurrentNorm->Phasor -
        sampleWithUnknownStatus->Node2LineCurrentNorm->Phasor).RMSvalue, 2);

    // The node voltages
    dist += wNode / 2 * pow((sampleWithKnownStatus->Node1VoltageNorm->Phasor -
        sampleWithUnknownStatus->Node1VoltageNorm->Phasor).RMSvalue, 2);
    dist += wNode / 2 * pow((sampleWithKnownStatus->Node2VoltageNorm->Phasor -
        sampleWithUnknownStatus->Node2VoltageNorm->Phasor).RMSvalue, 2);

    // The other currents
    double numberOfCurrents = (double)sampleWithKnownStatus->NumberOfNode1OtherCurrents;
    for (int currentIndex = 0; currentIndex < sampleWithKnownStatus->NumberOfNode1OtherCurrents; currentIndex++) {
        dist += wOther / (2 * numberOfCurrents) * pow((sampleWithKnownStatus->Node1OtherCurrentsNorm[currentIndex]->Phasor -
            sampleWithUnknownStatus->Node1OtherCurrentsNorm[currentIndex]->Phasor).RMSvalue, 2);
    }
    numberOfCurrents = (double)sampleWithKnownStatus->NumberOfNode2OtherCurrents;
    for (int currentIndex = 0; currentIndex < sampleWithKnownStatus->NumberOfNode2OtherCurrents; currentIndex++) {
        dist += wOther / (2 * numberOfCurrents) * pow((sampleWithKnownStatus->Node2OtherCurrentsNorm[currentIndex]->Phasor -
            sampleWithUnknownStatus->Node2OtherCurrentsNorm[currentIndex]->Phasor).RMSvalue, 2);
    }

    return sqrt(dist);
}



distanceSample::distanceSample(lineSample* sampleWithKnownStatus, lineSample* sampleWithUnknownStatus)
{
    if (AreSamplesOfTheSameLine(sampleWithKnownStatus, sampleWithUnknownStatus) == false) {
        cout << "distanceSample(): lineSample* known and lineSample* unknown are not samples of the same line.\n";
        return;
    }

    line = sampleWithKnownStatus;
    IsWorking = sampleWithKnownStatus->IsWorking;

    Distance = CalculateDistance(sampleWithKnownStatus, sampleWithUnknownStatus);
}

distanceSample::~distanceSample() {}


const void distanceSample::Print() {
    line->PrintLine();
    cout << "Wline = " << to_string(wLine) << "\n";
    cout << "Wnode = " << to_string(wNode) << "\n";
    cout << "Wother = " << to_string(wOther) << "\n";
    cout << "distance = " << to_string(Distance) << "\n";
    cout << "isWorking = " << to_string(IsWorking) << "\n";
}