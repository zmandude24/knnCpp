#define _USE_MATH_DEFINES

using namespace std;

#include <cmath>
#include <exception>
#include <iostream>
#include <string>
#include "phasor.h"
#include "instantaneousMeasurement.h"
#include "parameter.h"
#include "nodeSample.h"


const void nodeSample::FreeMemory()
{
    if (Voltage != NULL) {
        delete Voltage;
        Voltage = NULL;
    }

    if (Currents != NULL) {
        for (int deletingIndex = 0; deletingIndex < NumberOfCurrents; deletingIndex++) {
            if (Currents[deletingIndex] != NULL) {
                delete Currents[deletingIndex];
                Currents[deletingIndex] = NULL;
            }
        }
        delete[] Currents;
        Currents = NULL;
    }
}

const void nodeSample::MemoryAllocationFailure(string variableName)
{
    cout << "Error: node() failed to allocate memory for " << variableName << "\n";
    FreeMemory();
}



nodeSample::nodeSample(int nodeNumber, parameter* voltage, parameter** currents, int numberOfCurrents)
{
    NodeNumber = nodeNumber;
    Voltage = voltage;
    Currents = currents;
    NumberOfCurrents = numberOfCurrents;
}
nodeSample::nodeSample(int nodeNumber, phasor voltage, phasor* currents, int* currentDestinationNodes, int numberOfCurrents)
{
    NodeNumber = nodeNumber;

    Voltage = new parameter(voltage, "V" + to_string(nodeNumber), "V", nodeNumber, 0);
    if (Voltage == NULL) {
        MemoryAllocationFailure("Voltage");
        return;
    }

    if (numberOfCurrents > 0) {
        Currents = new parameter * [numberOfCurrents];
        if (Currents == NULL) {
            MemoryAllocationFailure("Currents");
            return;
        }
        for (int currentsIndex = 0; currentsIndex < numberOfCurrents; currentsIndex++) Currents[currentsIndex] = NULL;

        for (int currentsIndex = 0; currentsIndex < numberOfCurrents; currentsIndex++) {
            Currents[currentsIndex] = new parameter(currents[currentsIndex],
                "I" + to_string(nodeNumber) + to_string(currentDestinationNodes[currentsIndex]),
                "A", nodeNumber, currentDestinationNodes[currentsIndex]);
            if (Currents[currentsIndex] == NULL) {
                MemoryAllocationFailure("Currents[currentsIndex]");
                return;
            }
        }
    }
    NumberOfCurrents = numberOfCurrents;
}
nodeSample::nodeSample(int nodeNumber, phasor voltage, phasor current1, int current1DestinationNode,
    phasor current2, int current2DestinationNode)
{
    NodeNumber = nodeNumber;

    Voltage = new parameter(voltage, "V" + to_string(nodeNumber), "V", nodeNumber, 0);
    if (Voltage == NULL) {
        MemoryAllocationFailure("Voltage");
        return;
    }

    NumberOfCurrents = 2;   // This constructor is for specifically two currents.
    Currents = new parameter*[NumberOfCurrents];
    if (Currents == NULL) {
        MemoryAllocationFailure("Currents");
        return;
    }
    Currents[0] = new parameter(current1, "I" + to_string(nodeNumber) + to_string(current1DestinationNode), "A",
        nodeNumber, current1DestinationNode);
    if (Currents[0] == NULL) {
        MemoryAllocationFailure("Currents[0]");
        return;
    }
    Currents[1] = new parameter(current2, "I" + to_string(nodeNumber) + to_string(current2DestinationNode), "A",
        nodeNumber, current2DestinationNode);
    if (Currents[1] == NULL) {
        MemoryAllocationFailure("Currents[1]");
        return;
    }
}
nodeSample::nodeSample(int nodeNumber, phasor voltage, phasor current1, int current1DestinationNode,
    phasor current2, int current2DestinationNode, phasor current3, int current3DestinationNode)
{
    NodeNumber = nodeNumber;

    Voltage = new parameter(voltage, "V" + to_string(nodeNumber), "V", nodeNumber, 0);
    if (Voltage == NULL) {
        MemoryAllocationFailure("Voltage");
        return;
    }

    NumberOfCurrents = 3;   // This constructor is for specifically three currents.
    Currents = new parameter*[NumberOfCurrents];
    if (Currents == NULL) {
        MemoryAllocationFailure("Currents");
        return;
    }
    Currents[0] = new parameter(current1, "I" + to_string(nodeNumber) + to_string(current1DestinationNode), "A",
        nodeNumber, current1DestinationNode);
    if (Currents[0] == NULL) {
        MemoryAllocationFailure("Currents[0]");
        return;
    }
    Currents[1] = new parameter(current2, "I" + to_string(nodeNumber) + to_string(current2DestinationNode), "A",
        nodeNumber, current2DestinationNode);
    if (Currents[1] == NULL) {
        MemoryAllocationFailure("Currents[1]");
        return;
    }
    Currents[2] = new parameter(current3, "I" + to_string(nodeNumber) + to_string(current3DestinationNode), "A",
        nodeNumber, current3DestinationNode);
    if (Currents[2] == NULL) {
        MemoryAllocationFailure("Currents[2]");
        return;
    }
}

nodeSample::~nodeSample()
{
    FreeMemory();
}


const void nodeSample::PrintNode()
{
    cout << "Node " << to_string(NodeNumber) << "\n";
    if (Voltage != NULL) cout << Voltage->Name << " = " << Voltage->Phasor.PhasorToString() << "V\n";
    else cout << "'voltage' not set\n";
    if (Currents != NULL) // cout << currents[i].name << " = " << currents[i].ph.PhasorToString() << "A\n"
        for (int i = 0; i < NumberOfCurrents; i++) {
            cout << Currents[i]->Name << " = " << Currents[i]->Phasor.PhasorToString() << "A\n";
        }
    else cout << "'currents' not set\n";
    cout << "Rated Voltage: " << to_string(RatedVoltage) << "V\n";
    cout << "Rated Current: " << to_string(RatedCurrent) << "A\n";
}