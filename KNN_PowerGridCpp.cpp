/// <summary>
/// The KNN algorithm project predicts faults and failures for a power grid at the large scale or high voltage distribution level
/// by comparing voltage and current measurements of each point of interest to the most similar cases of those points and giving the 
/// most common status of those points as the prediction. Because the voltages being measured can be as high as 500kV, a power meter 
/// for each point of interest in the circuit is too expensive. This means there won’t be a meter for every point of interest, so 
/// being perfectly accurate is virtually impossible. However, the KNN algorithm has an error rate of 0.01% or lower assuming adequate 
/// metering.
/// 
/// This particular implementaion creates a list of samples where each line is operational and samples with one line down as a set
/// of cases where the line status is known. Then a sample where the line status is unknown is created and the line status is predicted
/// by the most common status of the k closest known samples.
/// 
/// This implementation:
/// 1. Stores samples of each parameter
/// 2. Groups them into nodes with the voltage at that point as well as the currents flowing out of the point
/// 3. Those nodes are then used to form line samples which store the normalized line currents, node voltages, and other currents
///     flowing out of those nodes not counting the line currents. Normalization means dividing the magnitudes by the voltage or
///     current rating.
/// </summary>

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
#include "lineSample.h"
#include "distanceSample.h"
#include "knnPredictionOfUnknownLineSample.h"


void TestDivideByZeroPhasorExceptionMemoryAllocationFailure(string, phasor*, phasor*, phasor*);
void TestDivideByZeroPhasorExceptionFreeMemory(phasor*, phasor*, phasor*);
/// <summary>
/// Attempt to divide a phasor by a zero phasor where the special case catch should output "Error: Divisor phasor is 0." 
/// </summary>
void TestDivideByZeroPhasorException()
{
    phasor* p1 = NULL;
    phasor* p2 = NULL;
    phasor* pdiv = NULL;

    p1 = new phasor(5, 30);
    if (p1 == NULL) {
        TestDivideByZeroPhasorExceptionMemoryAllocationFailure("p1", p1, p2, pdiv);
        return;
    }

    p2 = new phasor(0, 0);
    if (p2 == NULL) {
        TestDivideByZeroPhasorExceptionMemoryAllocationFailure("p2", p1, p2, pdiv);
        return;
    }

    pdiv = new phasor();
    if (pdiv == NULL) {
        TestDivideByZeroPhasorExceptionMemoryAllocationFailure("pdiv", p1, p2, pdiv);
        return;
    }
    *pdiv = *p1 / *p2;

    TestDivideByZeroPhasorExceptionFreeMemory(p1, p2, pdiv);
}
/// <summary>
/// Display an error message when the method TestDivideByZeroPhasorException() fails to allocate memory for a variable.
/// </summary>
/// <param name="variableName">The variable name the main method failed to allocate memory for</param>
void TestDivideByZeroPhasorExceptionMemoryAllocationFailure(string variableName, phasor* p1, phasor* p2, phasor* pdiv)
{
    cout << "Error: TestDivideByZeroPhasorException() failed to allocate memory for " << variableName << "\n";
    TestDivideByZeroPhasorExceptionFreeMemory(p1, p2, pdiv);
}
/// <summary>
/// Free memory for the method TestDivideByZeroPhasorException()
/// </summary>
void TestDivideByZeroPhasorExceptionFreeMemory(phasor* p1, phasor* p2, phasor* pdiv)
{
    if (p1 != NULL) {
        delete p1;
        p1 = NULL;
    }
    if (p2 != NULL) {
        delete p2;
        p2 = NULL;
    }
    if (pdiv != NULL) {
        delete pdiv;
        pdiv = NULL;
    }
}


void TestZeroToNonPositivePowerExceptionMemoryAllocationFailure(string, phasor*, phasor*);
void TestZeroToNonPositivePowerExceptionFreeMemory(phasor*, phasor*);
/// <summary>
/// Attempt to raise a zero phasor to a non-positive power where the special case catch should output "Error: Base is 0 and power
/// is non-positive."
/// </summary>
void TestZeroToNonPositivePowerException()
{
    phasor* ph = NULL;
    phasor* pexp = NULL;

    ph = new phasor(0, 0);
    if (ph == NULL) {
        TestZeroToNonPositivePowerExceptionMemoryAllocationFailure("ph", ph, pexp);
        return;
    }

    pexp = new phasor();
    if (pexp == NULL) {
        TestZeroToNonPositivePowerExceptionMemoryAllocationFailure("pexp", ph, pexp);
        return;
    }
    *pexp = ph->Pow(*ph, 0);

    TestZeroToNonPositivePowerExceptionFreeMemory(ph, pexp);
}
/// <summary>
/// Display an error message when the method TestZeroToNonPositivePowerException() fails to allocate memory for a variable.
/// </summary>
/// <param name="variableName">The name of the variable the method failed to allocate memory for</param>
void TestZeroToNonPositivePowerExceptionMemoryAllocationFailure(string variableName, phasor* ph, phasor* pexp)
{
    cout << "Error: TestZeroToNonPositivePowerException() failed to allocate memory for " << variableName << "\n";
    TestZeroToNonPositivePowerExceptionFreeMemory(ph, pexp);
}
/// <summary>
/// Free allocated memory for the method TestZeroToNonPositivePowerException()
/// </summary>
void TestZeroToNonPositivePowerExceptionFreeMemory(phasor* ph, phasor* pexp)
{
    if (ph != NULL) {
        delete ph;
        ph = NULL;
    }
    if (pexp != NULL) {
        delete pexp;
        pexp = NULL;
    }
}


void TestParameterPhasorCalcAccuracyMemoryAllocationFailure(string, phasor*, instantaneousMeasurement*, parameter*, phasor*);
void TestParameterPhasorCalcAccuracyFreeMemory(phasor*, instantaneousMeasurement*, parameter*, phasor*);
/// <summary>
/// This will test the accuracy of the parameter class's phasor calculation by creating sample points from a reference phasor and
/// the phasor calculated will be compared to the reference phasor. 
/// </summary>
void TestParameterPhasorCalcAccuracy()
{
    int samplesPerSecond = 32000;
    double totalSamplingTime = 1;
    double f = 60;  // sine wave frequency in Hz
    int numOfSamples = (int)((double)samplesPerSecond * totalSamplingTime);

    phasor* referencePhasor = NULL;
    instantaneousMeasurement* samples = NULL;
    parameter* testParameter = NULL;
    phasor* difference = NULL;

    referencePhasor = new phasor(120, 30);
    if (referencePhasor == NULL) {
        TestParameterPhasorCalcAccuracyMemoryAllocationFailure("referencePhasor", referencePhasor, samples, testParameter, difference);
        return;
    }

    // Create the array of samples
    samples = new instantaneousMeasurement[numOfSamples];
    if (samples == NULL) {
        TestParameterPhasorCalcAccuracyMemoryAllocationFailure("samples", referencePhasor, samples, testParameter, difference);
        return;
    }
    for (int i = 0; i < numOfSamples; i++) {
        samples[i].timeStamp = (double)i / (double)samplesPerSecond;

        // samples[i].value = A * sin(wt + theta)
        double wt = 2 * M_PI * f * samples[i].timeStamp;                            // Time dependent angle
        double A = sqrt(2) * referencePhasor->RMSvalue;                     // Amplitude
        double theta = M_PI / 180 * referencePhasor->PhaseAngleDegrees;     // Angle offset
        samples[i].value = A * sin(wt + theta);
    }

    // Allocate the test parameter
    testParameter = new parameter(samples, numOfSamples, "V1", "V", 1, 0);
    if (testParameter == NULL) {
        TestParameterPhasorCalcAccuracyMemoryAllocationFailure("testParameter", referencePhasor, samples, testParameter, difference);
        return;
    }
    samples = NULL;

    // Print the test parameter, the calculated phasor, and the reference phasor
    testParameter->PrintParameter();
    cout << "\nCalculated Phasor: " << testParameter->Phasor.PhasorToString() << "\n";
    cout << "Reference Phasor: " << referencePhasor->PhasorToString() << "\n";

    // Calculate and print the percent error
    difference = new phasor();
    if (difference == NULL) {
        TestParameterPhasorCalcAccuracyMemoryAllocationFailure("difference", referencePhasor, samples, testParameter, difference);
        return;
    }
    *difference = *referencePhasor - testParameter->Phasor;
    double percentError = 100 * difference->RMSvalue / referencePhasor->RMSvalue;
    cout << "The percent error is " << percentError << "\n";

    TestParameterPhasorCalcAccuracyFreeMemory(referencePhasor, samples, testParameter, difference);
}
/// <summary>
/// Display an error message when the method TestParameterPhasorCalcAccuracy() fails to allocate memory for a variable.
/// </summary>
void TestParameterPhasorCalcAccuracyMemoryAllocationFailure(string variableName, phasor* referencePhasor,
    instantaneousMeasurement* samples, parameter* testParameter, phasor* difference)
{
    cout << "Error: TestParameterPhasorCalcAccuracyFreeMemory() failed to allocate memory for " << variableName << "\n";
    TestParameterPhasorCalcAccuracyFreeMemory(referencePhasor, samples, testParameter, difference);
}
/// <summary>
/// Free allocated memory for the method TestParameterPhasorCalcAccuracy()
/// </summary>
void TestParameterPhasorCalcAccuracyFreeMemory(phasor* referencePhasor, instantaneousMeasurement* samples, parameter* testParameter,
    phasor* difference)
{
    if (referencePhasor != NULL) {
        delete referencePhasor;
        referencePhasor = NULL;
    }
    if (testParameter != NULL) {
        delete testParameter;
        testParameter = NULL;
    }
    if (samples != NULL) {
        delete samples;
        samples = NULL;
    }
    if (difference != NULL) {
        delete difference;
        difference = NULL;
    }
}


void TestNodeClassMemoryAllocationFailure(string, phasor*, int*, nodeSample*);
void TestNodeClassFreeMemory(phasor*, int*, nodeSample*);
/// <summary>
/// Create a sample node and print it.
/// </summary>
void TestNodeClass()
{
    phasor* currents = NULL;
    int* currentDestNodes = NULL;
    nodeSample* testNode = NULL;

    phasor voltage = phasor(250000, 15);

    currents = new phasor[2];
    if (currents == NULL) {
        TestNodeClassMemoryAllocationFailure("currents", currents, currentDestNodes, testNode);
        return;
    }
    currents[0] = phasor(25, -165); currents[1] = phasor(25, 15);

    currentDestNodes = new int[2];
    if (currentDestNodes == NULL) {
        TestNodeClassMemoryAllocationFailure("currentDestNodes", currents, currentDestNodes, testNode);
        return;
    }
    currentDestNodes[0] = 0; currentDestNodes[1] = 2;

    testNode = new nodeSample(1, voltage, currents, currentDestNodes, 2);
    if (testNode == NULL) {
        TestNodeClassMemoryAllocationFailure("testNode", currents, currentDestNodes, testNode);
        return;
    }
    testNode->PrintNode();

    TestNodeClassFreeMemory(currents, currentDestNodes, testNode);
}
/// <summary>
/// Display an error message when the method TestNodeClass() fails to allocate memory for a variable.
/// </summary>
/// <param name="variableName">The variable the method failed to allocate memory for</param>
void TestNodeClassMemoryAllocationFailure(string variableName, phasor* currents, int* currentDestNodes, nodeSample* testNode)
{
    cout << "Error: TestNodeClass() failed to allocate memory for " << variableName << "\n";
    TestNodeClassFreeMemory(currents, currentDestNodes, testNode);
}
/// <summary>
/// Free allocated memory for the method TestNodeClass().
/// </summary>
void TestNodeClassFreeMemory(phasor* currents, int* currentDestNodes, nodeSample* testNode)
{
    if (testNode != NULL) {
        delete testNode;
        testNode = NULL;
    }
    if (currentDestNodes != NULL) {
        delete[] currentDestNodes;
        currentDestNodes = NULL;
    }
    if (currents != NULL) {
        delete[] currents;
        currents = NULL;
    }
}


void TestLineClassMemoryAllocationFailure(string, phasor*, phasor*, int*, int*, lineSample*);
void TestLineClassFreeMemory(phasor*, phasor*, int*, int*, lineSample*);
/// <summary>
/// Create a sample line and print it.
/// </summary>
void TestLineClass()
{
    phasor* node1Currents = NULL;
    phasor* node2Currents = NULL;
    int* node1CurrentDestNodes = NULL;
    int* node2CurrentDestNodes = NULL;
    shared_ptr<nodeSample> node1 = NULL;
    shared_ptr<nodeSample> node2 = NULL;
    lineSample* testLine = NULL;

    // Node 1
    phasor node1Voltage = phasor(250000, 15);
    node1Currents = new phasor[2];
    if (node1Currents == NULL) {
        TestLineClassMemoryAllocationFailure("node1Currents", node1Currents, node2Currents, node1CurrentDestNodes, node2CurrentDestNodes,
            testLine);
        return;
    }
    node1Currents[0] = phasor(25, -165); node1Currents[1] = phasor(25, 15);
    node1CurrentDestNodes = new int[2];
    if (node1CurrentDestNodes == NULL) {
        TestLineClassMemoryAllocationFailure("node1CurrentDestNodes", node1Currents, node2Currents,
            node1CurrentDestNodes, node2CurrentDestNodes, testLine);
        return;
    }
    node1CurrentDestNodes[0] = 0; node1CurrentDestNodes[1] = 2;
    node1 = shared_ptr<nodeSample>(new nodeSample(1, node1Voltage, node1Currents, node1CurrentDestNodes, 2));
    if (node1 == NULL) {
        TestLineClassMemoryAllocationFailure("node1", node1Currents, node2Currents, node1CurrentDestNodes, node2CurrentDestNodes,
            testLine);
        return;
    }

    // Node 2
    phasor node2Voltage = phasor(245000, 13);
    node2Currents = new phasor[3];
    if (node2Currents == NULL) {
        TestLineClassMemoryAllocationFailure("node2Currents", node1Currents, node2Currents, node1CurrentDestNodes, node2CurrentDestNodes,
            testLine);
        return;
    }
    node2Currents[0] = phasor(25, -165); node2Currents[1] = phasor(15, 15); node2Currents[2] = phasor(10, 15);
    node2CurrentDestNodes = new int[3];
    if (node2CurrentDestNodes == NULL) {
        TestLineClassMemoryAllocationFailure("node2CurrentDestNodes", node1Currents, node2Currents,
            node1CurrentDestNodes, node2CurrentDestNodes, testLine);
        return;
    }
    node2CurrentDestNodes[0] = 1; node2CurrentDestNodes[1] = 0; node2CurrentDestNodes[2] = 3;
    node2 = shared_ptr<nodeSample>(new nodeSample(2, node2Voltage, node2Currents, node2CurrentDestNodes, 3));
    if (node2 == NULL) {
        TestLineClassMemoryAllocationFailure("node2", node1Currents, node2Currents, node1CurrentDestNodes, node2CurrentDestNodes,
            testLine);
        return;
    }

    testLine = new lineSample(node1, node2, true);
    if (testLine == NULL) {
        TestLineClassMemoryAllocationFailure("testLine", node1Currents, node2Currents, node1CurrentDestNodes, node2CurrentDestNodes,
            testLine);
        return;
    }
    testLine->PrintLine();

    TestLineClassFreeMemory(node1Currents, node2Currents, node1CurrentDestNodes, node2CurrentDestNodes, testLine);
}
/// <summary>
/// Display an error message when the method TestLineClass() fails to allocate memory for a variable.
/// </summary>
/// <param name="variableName">The variable the method failed to allocate memory for</param>
void TestLineClassMemoryAllocationFailure(string variableName, phasor* node1Currents, phasor* node2Currents,
    int* node1CurrentDestNodes, int* node2CurrentDestNodes, lineSample* testLine)
{
    cout << "Error: TestLineClass() failed to allocate memory for " << variableName << "\n";
    TestLineClassFreeMemory(node1Currents, node2Currents, node1CurrentDestNodes, node2CurrentDestNodes, testLine);
}
/// <summary>
/// Free allocated memory for the method TestLineClass().
/// </summary>
void TestLineClassFreeMemory(phasor* node1Currents, phasor* node2Currents, int* node1CurrentDestNodes, int* node2CurrentDestNodes,
    lineSample* testLine)
{
    if (node1Currents != NULL) {
        delete[] node1Currents;
        node1Currents = NULL;
    }
    if (node1CurrentDestNodes != NULL) {
        delete[] node1CurrentDestNodes;
        node1CurrentDestNodes = NULL;
    }

    if (node2Currents != NULL) {
        delete[] node2Currents;
        node2Currents = NULL;
    }
    if (node2CurrentDestNodes != NULL) {
        delete[] node2CurrentDestNodes;
        node2CurrentDestNodes = NULL;
    }

    if (testLine != NULL) {
        delete testLine;
        testLine = NULL;
    }
}


void TestDistanceClassMemoryAllocationFailure(string, phasor*, phasor*, phasor*, phasor*, int*, int*, int*, int*,
    lineSample*, lineSample*, distanceSample*);
void TestDistanceClassFreeMemory(phasor*, phasor*, phasor*, phasor*, int*, int*, int*, int*,
    lineSample*, lineSample*, distanceSample*);
/// <summary>
/// Create two line samples and a distance sample from it
/// </summary>
void TestDistanceClass()
{
    phasor* sample1Node1Currents = NULL;
    phasor* sample1Node2Currents = NULL;
    phasor* sample2Node1Currents = NULL;
    phasor* sample2Node2Currents = NULL;
    int* sample1Node1CurrentDestNodes = NULL;
    int* sample1Node2CurrentDestNodes = NULL;
    int* sample2Node1CurrentDestNodes = NULL;
    int* sample2Node2CurrentDestNodes = NULL;
    shared_ptr<nodeSample> sample1Node1 = NULL;
    shared_ptr<nodeSample> sample1Node2 = NULL;
    shared_ptr<nodeSample> sample2Node1 = NULL;
    shared_ptr<nodeSample> sample2Node2 = NULL;
    lineSample* sample1 = NULL;
    lineSample* sample2 = NULL;
    distanceSample* testDistance = NULL;

    // Sample 1, Node 1
    phasor sample1Node1Voltage = phasor(250000, 15);
    sample1Node1Currents = new phasor[2];
    if (sample1Node1Currents == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample1Node1Currents",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1, sample2, testDistance);
        return;
    }
    sample1Node1Currents[0] = phasor(25, -165); sample1Node1Currents[1] = phasor(25, 15);
    sample1Node1CurrentDestNodes = new int[2];
    if (sample1Node1CurrentDestNodes == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample1Node1CurrentDestNodes",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1, sample2, testDistance);
        return;
    }
    sample1Node1CurrentDestNodes[0] = 0; sample1Node1CurrentDestNodes[1] = 2;
    sample1Node1 = shared_ptr<nodeSample>(new nodeSample(1, sample1Node1Voltage, sample1Node1Currents, sample1Node1CurrentDestNodes, 2));
    if (sample1Node1 == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample1Node1",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1, sample2, testDistance);
        return;
    }

    // Sample 1, Node 2
    phasor sample1Node2Voltage = phasor(245000, 13);
    sample1Node2Currents = new phasor[3];
    if (sample1Node2Currents == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample1Node2Currents",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1, sample2, testDistance);
        return;
    }
    sample1Node2Currents[0] = phasor(25, -165); sample1Node2Currents[1] = phasor(15, 15); sample1Node2Currents[2] = phasor(10, 15);
    sample1Node2CurrentDestNodes = new int[3];
    if (sample1Node2CurrentDestNodes == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample1Node2CurrentDestNodes",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1, sample2, testDistance);
        return;
    }
    sample1Node2CurrentDestNodes[0] = 1; sample1Node2CurrentDestNodes[1] = 0; sample1Node2CurrentDestNodes[2] = 3;
    sample1Node2 = shared_ptr<nodeSample>(new nodeSample(2, sample1Node2Voltage, sample1Node2Currents, sample1Node2CurrentDestNodes, 3));
    if (sample1Node2 == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample1Node2",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1, sample2, testDistance);
        return;
    }

    // Sample 1
    sample1 = new lineSample(sample1Node1, sample1Node2, true);
    if (sample1 == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample1",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1, sample2, testDistance);
        return;
    }

    // Sample 2, Node 1
    phasor sample2Node1Voltage = phasor(252500, 15);
    sample2Node1Currents = new phasor[2];
    if (sample2Node1Currents == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample2Node1Currents",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1, sample2, testDistance);
        return;
    }
    sample2Node1Currents[0] = phasor(25, -165); sample2Node1Currents[1] = phasor(24.75, 15);
    sample2Node1CurrentDestNodes = new int[2];
    if (sample2Node1CurrentDestNodes == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample2Node1CurrentDestNodes",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1, sample2, testDistance);
        return;
    }
    sample2Node1CurrentDestNodes[0] = 0; sample2Node1CurrentDestNodes[1] = 2;
    sample2Node1 = shared_ptr<nodeSample>(new nodeSample(1, sample2Node1Voltage, sample2Node1Currents, sample2Node1CurrentDestNodes, 2));
    if (sample2Node1 == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample2Node1",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1, sample2, testDistance);
        return;
    }

    // Sample 2, Node 2
    phasor sample2Node2Voltage = phasor(250000, 13);
    sample2Node2Currents = new phasor[3];
    if (sample2Node2Currents == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample2Node2Currents",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1, sample2, testDistance);
        return;
    }
    sample2Node2Currents[0] = phasor(25.25, -165); sample2Node2Currents[1] = phasor(15.15, 15); sample2Node2Currents[2] = phasor(10.10, 15);
    sample2Node2CurrentDestNodes = new int[3];
    if (sample2Node2CurrentDestNodes == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample2Node2CurrentDestNodes",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1, sample2, testDistance);
        return;
    }
    sample2Node2CurrentDestNodes[0] = 1; sample2Node2CurrentDestNodes[1] = 0; sample2Node2CurrentDestNodes[2] = 3;
    sample2Node2 = shared_ptr<nodeSample>(new nodeSample(2, sample2Node2Voltage, sample2Node2Currents, sample2Node2CurrentDestNodes, 3));
    if (sample2Node2 == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample2Node2",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1, sample2, testDistance);
        return;
    }

    // Sample 2
    sample2 = new lineSample(sample2Node1, sample2Node2, true);
    if (sample2 == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample2",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1, sample2, testDistance);
        return;
    }

    testDistance = new distanceSample(sample1, sample2);
    if (testDistance == NULL) {
        TestDistanceClassMemoryAllocationFailure("testDistance",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1, sample2, testDistance);
        return;
    }
    testDistance->Print();

    // Free Memory
    TestDistanceClassFreeMemory(sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
        sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
        sample1, sample2, testDistance);
}
/// <summary>
/// Display an error message when the method TestDistanceClass() fails to allocate memory for a variable.
/// </summary>
/// <param name="variableName">The variable the method failed to allocate memory for</param>
void TestDistanceClassMemoryAllocationFailure(string variableName,
    phasor* sample1Node1Currents, phasor* sample1Node2Currents, phasor* sample2Node1Currents, phasor* sample2Node2Currents,
    int* sample1Node1CurrentDestNodes, int* sample1Node2CurrentDestNodes,
    int* sample2Node1CurrentDestNodes, int* sample2Node2CurrentDestNodes,
    lineSample* sample1, lineSample* sample2, distanceSample* testDistance)
{
    cout << "Error: TestDistanceClass() failed to allocate memory for " << variableName << "\n";
    TestDistanceClassFreeMemory(sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
        sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
        sample1, sample2, testDistance);
}
/// <summary>
/// Free allocated memory for the method TestDistanceClass().
/// </summary>
void TestDistanceClassFreeMemory(
    phasor* sample1Node1Currents, phasor* sample1Node2Currents, phasor* sample2Node1Currents, phasor* sample2Node2Currents,
    int* sample1Node1CurrentDestNodes, int* sample1Node2CurrentDestNodes,
    int* sample2Node1CurrentDestNodes, int* sample2Node2CurrentDestNodes,
    lineSample* sample1, lineSample* sample2, distanceSample* testDistance)
{
    if (sample1Node1Currents != NULL) {
        delete[] sample1Node1Currents;
        sample1Node1Currents = NULL;
    }
    if (sample1Node1CurrentDestNodes != NULL) {
        delete[] sample1Node1CurrentDestNodes;
        sample1Node1CurrentDestNodes = NULL;
    }

    if (sample1Node2Currents != NULL) {
        delete[] sample1Node2Currents;
        sample1Node2Currents = NULL;
    }
    if (sample1Node2CurrentDestNodes != NULL) {
        delete[] sample1Node2CurrentDestNodes;
        sample1Node2CurrentDestNodes = NULL;
    }

    if (sample2Node1Currents != NULL) {
        delete[] sample2Node1Currents;
        sample2Node1Currents = NULL;
    }
    if (sample2Node1CurrentDestNodes != NULL) {
        delete[] sample2Node1CurrentDestNodes;
        sample2Node1CurrentDestNodes = NULL;
    }

    if (sample2Node2Currents != NULL) {
        delete[] sample2Node2Currents;
        sample2Node2Currents = NULL;
    }
    if (sample2Node2CurrentDestNodes != NULL) {
        delete[] sample2Node2CurrentDestNodes;
        sample2Node2CurrentDestNodes = NULL;
    }

    if (sample1 != NULL) {
        delete sample1;
        sample1 = NULL;
    }
    if (sample2 != NULL) {
        delete sample2;
        sample2 = NULL;
    }
    if (testDistance != NULL) {
        delete testDistance;
        testDistance = NULL;
    }
}


void TestKnnClassMemoryAllocationFailure(string, shared_ptr<nodeSample>*, shared_ptr<nodeSample>*,
    phasor*, phasor*, phasor*, phasor*, int*, int*, lineSample**, lineSample*, knnPredictionOfUnknownLineSample*, int, int, int);
void TestKnnClassFreeMemory(shared_ptr<nodeSample>*, shared_ptr<nodeSample>*, phasor*, phasor*, phasor*, phasor*, int*, int*,
    lineSample**, lineSample*, knnPredictionOfUnknownLineSample*, int, int, int);
/// <summary>
/// Create a set of 10 line samples with known line statuses where 6 have lines working as well as a random line sample with an unknown
/// line status. The ones without the line working will be significantly different. 
/// </summary>
void TestKnnClass(int numberOfLinesWorking = 6, int numberOfLinesNotWorking = 4)
{
    shared_ptr<nodeSample>* node1SamplesWithKnownStatuses = NULL;
    shared_ptr<nodeSample>* node2SamplesWithKnownStatuses = NULL;
    phasor* node1WorkingCurrentAveragePhasors = NULL;
    phasor* node2WorkingCurrentAveragePhasors = NULL;
    phasor* node1NotWorkingCurrentAveragePhasors = NULL;
    phasor* node2NotWorkingCurrentAveragePhasors = NULL;
    int* node1CurrentDestNodes = NULL;
    int* node2CurrentDestNodes = NULL;
    lineSample** samplesWithKnownStatuses = NULL;
    shared_ptr<nodeSample> node1SampleWithUnknownStatus = NULL;
    shared_ptr<nodeSample> node2SampleWithUnknownStatus = NULL;
    lineSample* sampleWithUnknownStatus = NULL;
    knnPredictionOfUnknownLineSample* testKNN = NULL;

    int node1Number = 1;
    int node2Number = 2;
    int numberOfSamplesWithKnownStatuses = numberOfLinesWorking + numberOfLinesNotWorking;
    int numberOfNode1Currents = 2;
    int numberOfNode2Currents = 2;


    // Node 1 Knowns
    node1SamplesWithKnownStatuses = new shared_ptr<nodeSample>[numberOfSamplesWithKnownStatuses];
    if (node1SamplesWithKnownStatuses == NULL) {
        TestKnnClassMemoryAllocationFailure("node1SamplesWithKnownStatuses",
            node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
            node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
            node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
            node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
            sampleWithUnknownStatus, testKNN, numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
        return;
    }
    for (int initializingIndex = 0; initializingIndex < numberOfSamplesWithKnownStatuses; initializingIndex++) {
        node1SamplesWithKnownStatuses[initializingIndex] = NULL;
    }

    phasor node1WorkingVoltageAveragePhasor = phasor(250000, 15);
    phasor node1NotWorkingVoltageAveragePhasor = phasor(50000, -150);

    node1WorkingCurrentAveragePhasors = new phasor[2];
    if (node1WorkingCurrentAveragePhasors == NULL) {
        TestKnnClassMemoryAllocationFailure("node1WorkingCurrentAveragePhasors",
            node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
            node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
            node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
            node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
            sampleWithUnknownStatus, testKNN, numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
        return;
    }
    node1WorkingCurrentAveragePhasors[0] = phasor(25, 165);
    node1WorkingCurrentAveragePhasors[1] = phasor(25, -15);
    
    node1NotWorkingCurrentAveragePhasors = new phasor[2];
    if (node1NotWorkingCurrentAveragePhasors == NULL) {
        TestKnnClassMemoryAllocationFailure("node1NotWorkingCurrentAveragePhasors",
            node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
            node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
            node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
            node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
            sampleWithUnknownStatus, testKNN, numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
        return;
    }
    node1NotWorkingCurrentAveragePhasors[0] = phasor(250, -70);
    node1NotWorkingCurrentAveragePhasors[1] = phasor(250, 110);

    node1CurrentDestNodes = new int[2];
    if (node1CurrentDestNodes == NULL) {
        TestKnnClassMemoryAllocationFailure("node1CurrentDestNodes",
            node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
            node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
            node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
            node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
            sampleWithUnknownStatus, testKNN, numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
        return;
    }
    node1CurrentDestNodes[0] = 0;
    node1CurrentDestNodes[1] = 2;

    for (int sampleIndex = 0; sampleIndex < numberOfSamplesWithKnownStatuses; sampleIndex++) {
        // Create node sample where the line is working
        if (sampleIndex < numberOfLinesWorking) {
            phasor voltage = node1WorkingVoltageAveragePhasor *
                phasor(0.9 + 0.2 * (double)sampleIndex / (double)numberOfLinesWorking, 0);
            
            phasor* currents = new phasor[numberOfNode1Currents];
            if (currents == NULL) {
                TestKnnClassMemoryAllocationFailure("currents",
                    node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
                    node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
                    node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
                    node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
                    sampleWithUnknownStatus, testKNN, numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
                return;
            }
            currents[0] = node1WorkingCurrentAveragePhasors[0] *
                phasor(0.9 + 0.2 * (double)sampleIndex / (double)numberOfLinesWorking, 0);
            currents[1] = node1WorkingCurrentAveragePhasors[1] *
                phasor(0.9 + 0.2 * (double)sampleIndex / (double)numberOfLinesWorking, 0);
            
            node1SamplesWithKnownStatuses[sampleIndex] =
                shared_ptr<nodeSample>(new nodeSample(node1Number, voltage, currents, node1CurrentDestNodes, 2));
            if (node1SamplesWithKnownStatuses[sampleIndex] == NULL) {
                TestKnnClassMemoryAllocationFailure("node1SamplesWithKnownStatuses[sampleIndex]",
                    node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
                    node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
                    node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
                    node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
                    sampleWithUnknownStatus, testKNN, numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
                return;
            }

            if (currents != NULL) {
                delete[] currents;
                currents = NULL;
            }
        }

        // Create node sample where the line is not working
        else {
            phasor voltage = node1NotWorkingVoltageAveragePhasor *
                phasor(0.9 + 0.2 * (double)(sampleIndex - numberOfLinesWorking) / (double)numberOfLinesNotWorking, 0);

            phasor* currents = new phasor[numberOfNode1Currents];
            if (currents == NULL) {
                TestKnnClassMemoryAllocationFailure("currents",
                    node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
                    node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
                    node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
                    node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
                    sampleWithUnknownStatus, testKNN, numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
                return;
            }
            currents[0] = node1NotWorkingCurrentAveragePhasors[0] *
                phasor(0.9 + 0.2 * (double)(sampleIndex - numberOfLinesWorking) / (double)numberOfLinesNotWorking, 0);
            currents[1] = node1NotWorkingCurrentAveragePhasors[1] *
                phasor(0.9 + 0.2 * (double)(sampleIndex - numberOfLinesWorking) / (double)numberOfLinesNotWorking, 0);
            
            node1SamplesWithKnownStatuses[sampleIndex] = 
                shared_ptr<nodeSample>(new nodeSample(node1Number, voltage, currents, node1CurrentDestNodes, 2));
            if (node1SamplesWithKnownStatuses[sampleIndex] == NULL) {
                TestKnnClassMemoryAllocationFailure("node1SamplesWithKnownStatuses[sampleIndex]",
                    node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
                    node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
                    node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
                    node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
                    sampleWithUnknownStatus, testKNN, numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
                if (currents != NULL) {
                    delete[] currents;
                    currents = NULL;
                }
                return;
            }

            if (currents != NULL) {
                delete[] currents;
                currents = NULL;
            }
        }
    }


    // Node 2 Knowns
    node2SamplesWithKnownStatuses = new shared_ptr<nodeSample>[numberOfSamplesWithKnownStatuses];
    if (node2SamplesWithKnownStatuses == NULL) {
        TestKnnClassMemoryAllocationFailure("node2SamplesWithKnownStatuses",
            node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
            node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
            node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
            node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
            sampleWithUnknownStatus, testKNN, numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
        return;
    }
    for (int initializationIndex = 0; initializationIndex < numberOfSamplesWithKnownStatuses; initializationIndex++) {
        node2SamplesWithKnownStatuses[initializationIndex] = NULL;
    }

    phasor node2WorkingVoltageAveragePhasor = phasor(250000, 15);
    phasor node2NotWorkingVoltageAveragePhasor = phasor(75000, -120);

    node2WorkingCurrentAveragePhasors = new phasor[numberOfNode2Currents];
    if (node2WorkingCurrentAveragePhasors == NULL) {
        TestKnnClassMemoryAllocationFailure("node2WorkingCurrentAveragePhasors",
            node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
            node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
            node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
            node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
            sampleWithUnknownStatus, testKNN, numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
        return;
    }
    node2WorkingCurrentAveragePhasors[0] = phasor(25, -15);
    node2WorkingCurrentAveragePhasors[1] = phasor(25, 165);
    
    node2NotWorkingCurrentAveragePhasors = new phasor[numberOfNode2Currents];
    if (node2NotWorkingCurrentAveragePhasors == NULL) {
        TestKnnClassMemoryAllocationFailure("node2NotWorkingCurrentAveragePhasors",
            node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
            node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
            node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
            node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
            sampleWithUnknownStatus, testKNN, numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
        return;
    }
    node2NotWorkingCurrentAveragePhasors[0] = phasor(250, 70);
    node2NotWorkingCurrentAveragePhasors[1] = phasor(250, -110);

    node2CurrentDestNodes = new int[numberOfNode2Currents];
    if (node2CurrentDestNodes == NULL) {
        TestKnnClassMemoryAllocationFailure("node2CurrentDestNodes",
            node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
            node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
            node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
            node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
            sampleWithUnknownStatus, testKNN, numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
        return;
    }
    node2CurrentDestNodes[0] = 1; node2CurrentDestNodes[1] = 0;

    for (int sampleIndex = 0; sampleIndex < numberOfSamplesWithKnownStatuses; sampleIndex++) {
        // Create node sample where the line is working
        if (sampleIndex < numberOfLinesWorking) {
            phasor voltage = node2WorkingVoltageAveragePhasor *
                phasor(0.9 + 0.2 * (double)sampleIndex / (double)numberOfLinesWorking, 0);

            phasor* currents = new phasor[numberOfNode2Currents];
            if (currents == NULL) {
                TestKnnClassMemoryAllocationFailure("currents",
                    node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
                    node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
                    node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
                    node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
                    sampleWithUnknownStatus, testKNN, numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
                return;
            }
            currents[0] = node2WorkingCurrentAveragePhasors[0] *
                phasor(0.9 + 0.2 * (double)sampleIndex / (double)numberOfLinesWorking, 0);
            currents[1] = node2WorkingCurrentAveragePhasors[1] *
                phasor(0.9 + 0.2 * (double)sampleIndex / (double)numberOfLinesWorking, 0);

            node2SamplesWithKnownStatuses[sampleIndex] = 
                shared_ptr<nodeSample>(new nodeSample(node2Number, voltage, currents, node2CurrentDestNodes, 2));
            if (node2SamplesWithKnownStatuses[sampleIndex] == NULL) {
                TestKnnClassMemoryAllocationFailure("node2SamplesWithKnownStatuses[sampleIndex]",
                    node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
                    node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
                    node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
                    node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
                    sampleWithUnknownStatus, testKNN, numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
                if (currents != NULL) {
                    delete[] currents;
                    currents = NULL;
                }
                return;
            }

            if (currents != NULL) {
                delete[] currents;
                currents = NULL;
            }
        }

        // Create node sample where the line is not working
        else {
            phasor voltage = node2NotWorkingVoltageAveragePhasor *
                phasor(0.9 + 0.2 * (double)(sampleIndex - numberOfLinesWorking) / (double)numberOfLinesNotWorking, 0);
            
            phasor* currents = new phasor[numberOfNode2Currents];
            if (currents == NULL) {
                TestKnnClassMemoryAllocationFailure("currents",
                    node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
                    node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
                    node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
                    node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
                    sampleWithUnknownStatus, testKNN, numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
                return;
            }
            currents[0] = node2NotWorkingCurrentAveragePhasors[0] *
                phasor(0.9 + 0.2 * (double)(sampleIndex - numberOfLinesWorking) / (double)numberOfLinesNotWorking, 0);
            currents[1] = node2NotWorkingCurrentAveragePhasors[1] *
                phasor(0.9 + 0.2 * (double)(sampleIndex - numberOfLinesWorking) / (double)numberOfLinesNotWorking, 0);

            node2SamplesWithKnownStatuses[sampleIndex] = 
                shared_ptr<nodeSample>(new nodeSample(node2Number, voltage, currents, node2CurrentDestNodes, 2));
            if (node2SamplesWithKnownStatuses[sampleIndex] == NULL) {
                TestKnnClassMemoryAllocationFailure("node2SamplesWithKnownStatuses[sampleIndex]",
                    node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
                    node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
                    node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
                    node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
                    sampleWithUnknownStatus, testKNN, numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
                return;
            }

            if (currents != NULL) {
                delete[] currents;
                currents = NULL;
            }
        }
    }


    // knowns
    samplesWithKnownStatuses = new lineSample*[numberOfSamplesWithKnownStatuses];
    if (samplesWithKnownStatuses == NULL) {
        TestKnnClassMemoryAllocationFailure("samplesWithKnownStatuses",
            node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
            node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
            node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
            node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
            sampleWithUnknownStatus, testKNN, numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
        return;
    }
    for (int initializationIndex = 0; initializationIndex < numberOfSamplesWithKnownStatuses; initializationIndex++) {
        samplesWithKnownStatuses[initializationIndex] = NULL;
    }

    for (int sampleIndex = 0; sampleIndex < numberOfSamplesWithKnownStatuses; sampleIndex++) {
        // Line Working
        if (sampleIndex < numberOfLinesWorking) {
            samplesWithKnownStatuses[sampleIndex] =
                new lineSample(node1SamplesWithKnownStatuses[sampleIndex], node2SamplesWithKnownStatuses[sampleIndex], true);
            if (samplesWithKnownStatuses[sampleIndex] == NULL) {
                TestKnnClassMemoryAllocationFailure("samplesWithKnownStatuses[sampleIndex]",
                    node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
                    node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
                    node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
                    node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
                    sampleWithUnknownStatus, testKNN, numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
                return;
            }
        }

        // Line Not Working
        else {
            samplesWithKnownStatuses[sampleIndex] =
                new lineSample(node1SamplesWithKnownStatuses[sampleIndex], node2SamplesWithKnownStatuses[sampleIndex], false);
            if (samplesWithKnownStatuses[sampleIndex] == NULL) {
                TestKnnClassMemoryAllocationFailure("samplesWithKnownStatuses[sampleIndex]",
                    node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
                    node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
                    node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
                    node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
                    sampleWithUnknownStatus, testKNN, numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
            }
        }
    }


    // unknown
    node1SampleWithUnknownStatus = shared_ptr<nodeSample>(new nodeSample(node1Number, node1WorkingVoltageAveragePhasor,
        node1WorkingCurrentAveragePhasors, node1CurrentDestNodes, 2));
    node2SampleWithUnknownStatus = shared_ptr<nodeSample>(new nodeSample(node2Number, node2WorkingVoltageAveragePhasor,
        node2WorkingCurrentAveragePhasors, node2CurrentDestNodes, 2));
    sampleWithUnknownStatus = new lineSample(node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, true);


    testKNN = new knnPredictionOfUnknownLineSample(samplesWithKnownStatuses, numberOfSamplesWithKnownStatuses, sampleWithUnknownStatus);
    testKNN->ChangeNumberOfNearestNeighbors(3);
    testKNN->Print();

    TestKnnClassFreeMemory(node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
        node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
        node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
        node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
        sampleWithUnknownStatus, testKNN, numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
}
void TestKnnClassMemoryAllocationFailure(string variableName,
    shared_ptr<nodeSample>* node1SamplesWithKnownStatuses, shared_ptr<nodeSample>* node2SamplesWithKnownStatuses,
    phasor* node1WorkingCurrentAveragePhasors, phasor* node2WorkingCurrentAveragePhasors,
    phasor* node1NotWorkingCurrentAveragePhasors, phasor* node2NotWorkingCurrentAveragePhasors,
    int* node1CurrentDestNodes, int* node2CurrentDestNodes, lineSample** samplesWithKnownStatuses,
    lineSample* sampleWithUnknownStatus,
    knnPredictionOfUnknownLineSample* testKNN, int numberOfSamplesWithKnownStatuses, int numberOfNode1Currents, int numberOfNode2Currents)
{
    cout << "Error: TestKnnClass() failed to allocate memory for " << variableName << "\n";
    TestKnnClassFreeMemory(node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
        node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
        node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
        node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
        sampleWithUnknownStatus, testKNN, numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
}
/// <summary>
/// Free allocated memory for the method TestKnnClass().
/// </summary>
void TestKnnClassFreeMemory(shared_ptr<nodeSample>* node1SamplesWithKnownStatuses, shared_ptr<nodeSample>* node2SamplesWithKnownStatuses,
    phasor* node1WorkingCurrentAveragePhasors, phasor* node2WorkingCurrentAveragePhasors,
    phasor* node1NotWorkingCurrentAveragePhasors, phasor* node2NotWorkingCurrentAveragePhasors,
    int* node1CurrentDestNodes, int* node2CurrentDestNodes, lineSample** samplesWithKnownStatuses,
    lineSample* sampleWithUnknownStatus,
    knnPredictionOfUnknownLineSample* testKNN, int numberOfSamplesWithKnownStatuses, int numberOfNode1Currents, int numberOfNode2Currents)
{
    if (node1SamplesWithKnownStatuses != NULL) {
        delete[] node1SamplesWithKnownStatuses;
        node1SamplesWithKnownStatuses = NULL;
    }
    if (node1WorkingCurrentAveragePhasors != NULL) {
        delete[] node1WorkingCurrentAveragePhasors;
        node1WorkingCurrentAveragePhasors = NULL;
    }
    if (node1NotWorkingCurrentAveragePhasors != NULL) {
        delete[] node1NotWorkingCurrentAveragePhasors;
        node1NotWorkingCurrentAveragePhasors = NULL;
    }
    if (node1CurrentDestNodes != NULL) {
        delete[] node1CurrentDestNodes;
        node1CurrentDestNodes = NULL;
    }

    if (node2SamplesWithKnownStatuses != NULL) {
        delete[] node2SamplesWithKnownStatuses;
        node2SamplesWithKnownStatuses = NULL;
    }
    if (node2WorkingCurrentAveragePhasors != NULL) {
        delete[] node2WorkingCurrentAveragePhasors;
        node2WorkingCurrentAveragePhasors = NULL;
    }
    if (node2NotWorkingCurrentAveragePhasors != NULL) {
        delete[] node2NotWorkingCurrentAveragePhasors;
        node2NotWorkingCurrentAveragePhasors = NULL;
    }
    if (node2CurrentDestNodes != NULL) {
        delete[] node2CurrentDestNodes;
        node2CurrentDestNodes = NULL;
    }

    if (samplesWithKnownStatuses != NULL) {
        delete[] samplesWithKnownStatuses;
        samplesWithKnownStatuses = NULL;
    }

    if (sampleWithUnknownStatus != NULL) {
        delete sampleWithUnknownStatus;
        sampleWithUnknownStatus = NULL;
    }

    if (testKNN != NULL) {
        delete testKNN;
        testKNN = NULL;
    }
}


int main()
{
    TestDivideByZeroPhasorException();
    TestZeroToNonPositivePowerException();
    TestParameterPhasorCalcAccuracy();
    TestNodeClass();
    TestLineClass();
    TestDistanceClass();
    TestKnnClass(6, 4);
}