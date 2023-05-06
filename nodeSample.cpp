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

/// <summary>
/// Free the dynamically allocated memory and set their pointers to NULL.
/// </summary>
void nodeSample::FreeMemory()
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

/// <summary>
/// Display an error message and call FreeMemory().
/// </summary>
/// <param=variableName>The name of the variable that failed to get memory allocation</param>
void nodeSample::MemoryAllocationFailure(string variableName)
{
    cout << "Error: node() failed to allocate memory for " << variableName << "\n";
    FreeMemory();
}


/// <summary>
/// This constructor uses previously allocated parameters that will be freed on the deconstructor.
/// </summary>
/// <param name="nodeNumber">The unique identifying number for the node</param>
/// <param name="voltage">The pointer to the node voltage (startNodeNum = nodeNum and destNodeNum = 0)</param>
/// <param name="currents">The pointer to the currents (startNodeNum = nodeNum)</param>
/// <param name="numberOfCurrents">The number of currents the dynamically allocated array 'currents' has</param>
nodeSample::nodeSample(int nodeNumber, parameter* voltage, parameter** currents, int numberOfCurrents)
{
    NodeNumber = nodeNumber;
    Voltage = voltage;
    Currents = currents;
    NumberOfCurrents = numberOfCurrents;
}
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
/// The rated current (typically the magnitude for a two line node at peak normal usage of the grid)
/// </param>
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

/// <summary>
/// This will free everything in its pointers
/// </summary>
nodeSample::~nodeSample()
{
    FreeMemory();
}

/// <summary>
/// Prints the node number, voltage phasor, and current phasors
/// </summary>
void nodeSample::PrintNode()
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