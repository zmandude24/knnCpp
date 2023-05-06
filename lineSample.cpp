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
/// Free the dynamically allocated memory and set their pointers to NULL. Notice that nodeSample* node1 and nodeSample* node2
/// are not freed here.
/// </summary>
void lineSample::FreeMemory()
{
    if (Node1LineCurrentNorm != NULL) {
        delete Node1LineCurrentNorm;
        Node1LineCurrentNorm = NULL;
    }
    if (Node2LineCurrentNorm != NULL) {
        delete Node2LineCurrentNorm;
        Node2LineCurrentNorm = NULL;
    }

    if (Node1VoltageNorm != NULL) {
        delete Node1VoltageNorm;
        Node1VoltageNorm = NULL;
    }
    if (Node2VoltageNorm != NULL) {
        delete Node2VoltageNorm;
        Node2VoltageNorm = NULL;
    }

    if (Node1OtherCurrentsNorm != NULL) {
        for (int deletingIndex = 0; deletingIndex < NumberOfNode1OtherCurrents; deletingIndex++) {
            if (Node1OtherCurrentsNorm[deletingIndex] != NULL) {
                delete Node1OtherCurrentsNorm[deletingIndex];
                Node1OtherCurrentsNorm[deletingIndex] = NULL;
            }
        }
        delete[] Node1OtherCurrentsNorm;
        Node1OtherCurrentsNorm = NULL;
        NumberOfNode1OtherCurrents = 0;
    }
    if (Node2OtherCurrentsNorm != NULL) {
        for (int deletingIndex = 0; deletingIndex < NumberOfNode2OtherCurrents; deletingIndex++) {
            if (Node2OtherCurrentsNorm[deletingIndex] != NULL) {
                delete Node2OtherCurrentsNorm[deletingIndex];
                Node2OtherCurrentsNorm[deletingIndex] = NULL;
            }
        }
        delete[] Node2OtherCurrentsNorm;
        Node2OtherCurrentsNorm = NULL;
        NumberOfNode2OtherCurrents = 0;
    }
}

/// <summary>
/// Display an error message and call FreeMemory().
/// </summary>
/// <param=variableName>The name of the variable that failed to get memory allocation</param>
void lineSample::MemoryAllocationFailure(string variableName)
{
    cout << "Error: node() failed to allocate memory for " << variableName << "\n";
    FreeMemory();
}


/// <summary>
/// The constructor
/// </summary>
/// <param name="node1">The first node of the line</param>
/// <param name="node2">The second node of the line</param>
/// <param name="isWorking">The status of the line</param>
lineSample::lineSample(nodeSample* node1, nodeSample* node2, bool isWorking)
{
    this->node1 = node1;
    this->node2 = node2;
    IsWorking = isWorking;

    // Node 1 line current
    Node1LineCurrentNorm = new parameter();
    if (Node1LineCurrentNorm == NULL) {
        MemoryAllocationFailure("Node1LineCurrentNorm");
        return;
    }
    bool foundCurrent = false;
    for (int currentIndex = 0; currentIndex < node1->NumberOfCurrents; currentIndex++) {    // Find the current going to node 2
        if (node1->Currents[currentIndex]->DestinationNodeNumber == node2->NodeNumber) {    // Found the current going to node 2
            *Node1LineCurrentNorm = *node1->Currents[currentIndex];
            Node1LineCurrentNorm->Phasor = node1->Currents[currentIndex]->Phasor / phasor(node1->RatedCurrent, 0);
            foundCurrent = true;
            break;
        }
    }
    if (foundCurrent == false) {    // Failed to find the current going to node 2
        cout << "Error: Unable to find a current in node1 going to node2!\n";
        FreeMemory();
        return;
    }

    // Node 2 line current
    Node2LineCurrentNorm = new parameter();
    if (Node2LineCurrentNorm == NULL) {
        MemoryAllocationFailure("Node2LineCurrentNorm");
        return;
    }
    foundCurrent = false;
    for (int currentIndex = 0; currentIndex < node2->NumberOfCurrents; currentIndex++) {    // Find the current going to node 1
        if (node2->Currents[currentIndex]->DestinationNodeNumber == node1->NodeNumber) {    // Found the current going to node 1
            *Node2LineCurrentNorm = *node2->Currents[currentIndex];
            Node2LineCurrentNorm->Phasor = node2->Currents[currentIndex]->Phasor / phasor(node2->RatedCurrent, 0);
            foundCurrent = true;
            break;
        }
    }
    if (foundCurrent == false) {    // Failed to find the current going to node 1
        cout << "Error: Unable to find a current in node2 going to node1!\n";
        FreeMemory();
        return;
    }

    // Node 1 voltage
    Node1VoltageNorm = new parameter();
    if (Node1VoltageNorm == NULL) {
        MemoryAllocationFailure("Node1VoltageNorm");
        return;
    }
    *Node1VoltageNorm = *node1->Voltage;
    Node1VoltageNorm->Phasor = node1->Voltage->Phasor / phasor(node1->RatedVoltage, 0);

    // Node 2 voltage
    Node2VoltageNorm = new parameter();
    if (Node2VoltageNorm == NULL) {
        MemoryAllocationFailure("Node2VoltageNorm");
        return;
    }
    *Node2VoltageNorm = *node2->Voltage;
    Node2VoltageNorm->Phasor = node2->Voltage->Phasor / phasor(node2->RatedVoltage, 0);

    // Node 1 other currents
    if (node1->NumberOfCurrents <= 1) NumberOfNode1OtherCurrents = 0;
    else {
        NumberOfNode1OtherCurrents = node1->NumberOfCurrents - 1;
        Node1OtherCurrentsNorm = new parameter * [NumberOfNode1OtherCurrents];
        if (Node1OtherCurrentsNorm == NULL) {
            MemoryAllocationFailure("Node1OtherCurrentsNorm");
            return;
        }
        for (int initializingIndex = 0; initializingIndex < NumberOfNode1OtherCurrents; initializingIndex++) {
            Node1OtherCurrentsNorm[initializingIndex] = NULL;
        }

        foundCurrent = false;
        for (int currentIndex = 0; currentIndex < node1->NumberOfCurrents; currentIndex++) {
            if (foundCurrent == false) {
                // Skip the line current
                if (node1->Currents[currentIndex]->DestinationNodeNumber == node2->NodeNumber) foundCurrent = true;

                else {
                    Node1OtherCurrentsNorm[currentIndex] = new parameter();
                    if (Node1OtherCurrentsNorm[currentIndex] == NULL) {
                        MemoryAllocationFailure("Node1OtherCurrentsNorm[currentIndex]");
                        return;
                    }
                    *Node1OtherCurrentsNorm[currentIndex] = *node1->Currents[currentIndex];
                    Node1OtherCurrentsNorm[currentIndex]->Phasor =
                        node1->Currents[currentIndex]->Phasor / phasor(node1->RatedCurrent, 0);
                }
            }

            else {
                Node1OtherCurrentsNorm[currentIndex - 1] = new parameter();
                if (Node1OtherCurrentsNorm[currentIndex - 1] == NULL) {
                    MemoryAllocationFailure("Node1OtherCurrentsNorm[currentIndex - 1]");
                    return;
                }
                *Node1OtherCurrentsNorm[currentIndex - 1] = *node1->Currents[currentIndex];
                Node1OtherCurrentsNorm[currentIndex - 1]->Phasor =
                    node1->Currents[currentIndex]->Phasor / phasor(node1->RatedCurrent, 0);
            }
        }
    }

    // Node 2 other currents
    if (node2->NumberOfCurrents <= 1) NumberOfNode2OtherCurrents = 0;
    else {
        NumberOfNode2OtherCurrents = node2->NumberOfCurrents - 1;
        Node2OtherCurrentsNorm = new parameter * [NumberOfNode2OtherCurrents];
        if (Node2OtherCurrentsNorm == NULL) {
            MemoryAllocationFailure("Node2OtherCurrentsNorm");
            return;
        }
        for (int initializingIndex = 0; initializingIndex < NumberOfNode2OtherCurrents; initializingIndex++) {
            Node2OtherCurrentsNorm[initializingIndex] = NULL;
        }

        foundCurrent = false;
        for (int currentIndex = 0; currentIndex < node2->NumberOfCurrents; currentIndex++) {
            if (foundCurrent == false) {
                if (node2->Currents[currentIndex]->DestinationNodeNumber == node1->NodeNumber) foundCurrent = true;

                else {
                    Node2OtherCurrentsNorm[currentIndex] = new parameter();
                    if (Node2OtherCurrentsNorm[currentIndex] == NULL) {
                        MemoryAllocationFailure("Node2OtherCurrentsNorm[currentIndex]");
                        return;
                    }
                    *Node2OtherCurrentsNorm[currentIndex] = *node2->Currents[currentIndex];
                    Node2OtherCurrentsNorm[currentIndex]->Phasor =
                        node2->Currents[currentIndex]->Phasor / phasor(node2->RatedCurrent, 0);
                }
            }

            else {
                Node2OtherCurrentsNorm[currentIndex - 1] = new parameter();
                if (Node2OtherCurrentsNorm[currentIndex - 1] == NULL) {
                    MemoryAllocationFailure("Node2OtherCurrentsNorm[currentIndex - 1]");
                    return;
                }
                *Node2OtherCurrentsNorm[currentIndex - 1] = *node2->Currents[currentIndex];
                Node2OtherCurrentsNorm[currentIndex - 1]->Phasor =
                    node2->Currents[currentIndex]->Phasor / phasor(node2->RatedCurrent, 0);
            }
        }
    }
}

/// <summary>
/// The deconstructor (notice that the node pointers don't have their memory freed here)
/// </summary>
lineSample::~lineSample()
{
    FreeMemory();
}


/// <summary>
/// Print the nodes, line status, and normalized parameters.
/// </summary>
void lineSample::PrintLine()
{
    cout << "\nNode 1:\n";
    node1->PrintNode();
    cout << "\nNode 2:\n";
    node2->PrintNode();

    cout << "\nLine status: " << to_string(IsWorking) << "\n";
    cout << "Node 1 Normalized Line Current:\n";
    Node1LineCurrentNorm->PrintParameter();
    cout << "Node 2 Normalized Line Current:\n";
    Node2LineCurrentNorm->PrintParameter();
    cout << "Node 1 Normalized Node Voltage:\n";
    Node1VoltageNorm->PrintParameter();
    cout << "Node 2 Normalized Node Voltage:\n";
    Node2VoltageNorm->PrintParameter();
    cout << "Node 1 Normalized Other Currents:\n";
    for (int currentIndex = 0; currentIndex < NumberOfNode1OtherCurrents; currentIndex++) {
        Node1OtherCurrentsNorm[currentIndex]->PrintParameter();
    }
    cout << "Node 2 Normalized Other Currents:\n";
    for (int currentIndex = 0; currentIndex < NumberOfNode2OtherCurrents; currentIndex++) {
        Node2OtherCurrentsNorm[currentIndex]->PrintParameter();
    }
}