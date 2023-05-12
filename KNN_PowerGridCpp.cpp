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
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <time.h>
#include "phasor.h"
#include "instantaneousMeasurement.h"
#include "parameter.h"
#include "nodeSample.h"
#include "lineSample.h"
#include "distanceSample.h"
#include "knnPredictionOfUnknownLineSample.h"


/// <summary>
/// This method returns a random double between the minimum and maximum parameters.
/// </summary>
/// <param name="minimum">The lowest possible value of the random double</param>
/// <param name="maximum">The highest possible value of the random double</param>
/// <returns>A random double between the minimum and maximum parameters</returns>
double RandomDouble(double minimum, double maximum)
{
    if (minimum == maximum) {
        cout << "Warning: The two parameters in RandomDouble() are equal, and their value will be returned.\n";
        return minimum;
    }
    if (minimum > maximum) {
        cout << "Warning: The minimum parameter is greater than the maximum parameter, but the method will still work as intended.\n";
    }

    double randomMultiple = (double)(rand() % RAND_MAX) / (double)(RAND_MAX - 1);
    double randomDouble = minimum + randomMultiple * (maximum - minimum);
    return randomDouble;
}


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
    double frequency = 60;  // sine wave frequency in Hz
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
    for (int sampleIndex = 0; sampleIndex < numOfSamples; sampleIndex++) {
        samples[sampleIndex].timeStamp = (double)sampleIndex / (double)samplesPerSecond;

        // samples[sampleIndex].value = A * sin(wt + theta)
        double timeDependentAngle = 2 * M_PI * frequency * samples[sampleIndex].timeStamp;
        double Amplitude = sqrt(2) * referencePhasor->RMSvalue;
        double angleOffset = M_PI / 180 * referencePhasor->PhaseAngleDegrees;
        samples[sampleIndex].value = Amplitude * sin(timeDependentAngle + angleOffset);
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

    
void TestNodeClassMemoryAllocationFailure(string, nodeSample*);
void TestNodeClassFreeMemory(nodeSample*);
/// <summary>
/// Create a sample node and print it.
/// </summary>
void TestNodeClass()
{
    nodeSample* testNode = new nodeSample(1, phasor(250000, 15), phasor(25, -165), 0, phasor(25, 15), 2);
    if (testNode == NULL) {
        TestNodeClassMemoryAllocationFailure("testNode", testNode);
        return;
    }
    testNode->PrintNode();

    TestNodeClassFreeMemory(testNode);
}
/// <summary>
/// Display an error message when the method TestNodeClass() fails to allocate memory for a variable.
/// </summary>
/// <param name="variableName">The variable the method failed to allocate memory for</param>
void TestNodeClassMemoryAllocationFailure(string variableName, nodeSample* testNode)
{
    cout << "Error: TestNodeClass() failed to allocate memory for " << variableName << "\n";
    TestNodeClassFreeMemory(testNode);
}
/// <summary>
/// Free allocated memory for the method TestNodeClass().
/// </summary>
void TestNodeClassFreeMemory(nodeSample* testNode)
{
    if (testNode != NULL) {
        delete testNode;
        testNode = NULL;
    }
}


void TestLineClassMemoryAllocationFailure(string, lineSample*);
void TestLineClassFreeMemory(lineSample*);
/// <summary>
/// Create a sample line and print it.
/// </summary>
void TestLineClass()
{
    shared_ptr<nodeSample> node1 = NULL;
    shared_ptr<nodeSample> node2 = NULL;
    lineSample* testLine = NULL;

    node1 = shared_ptr<nodeSample>(new nodeSample(1, phasor(250000, 15), phasor(25, -165), 0, phasor(25, 15), 2));
    if (node1 == NULL) {
        TestLineClassMemoryAllocationFailure("node1", testLine);
        return;
    }
    node2 = shared_ptr<nodeSample>(new nodeSample(2, phasor(245000, 13), phasor(25, -165), 1, phasor(15, 15), 0,
        phasor(10, 15), 3));
    if (node2 == NULL) {
        TestLineClassMemoryAllocationFailure("node2", testLine);
        return;
    }

    testLine = new lineSample(node1, node2, true);
    if (testLine == NULL) {
        TestLineClassMemoryAllocationFailure("testLine", testLine);
        return;
    }
    testLine->PrintLine();

    TestLineClassFreeMemory(testLine);
}
/// <summary>
/// Display an error message when the method TestLineClass() fails to allocate memory for a variable.
/// </summary>
/// <param name="variableName">The variable the method failed to allocate memory for</param>
void TestLineClassMemoryAllocationFailure(string variableName, lineSample* testLine)
{
    cout << "Error: TestLineClass() failed to allocate memory for " << variableName << "\n";
    TestLineClassFreeMemory(testLine);
}
/// <summary>
/// Free allocated memory for the method TestLineClass().
/// </summary>
void TestLineClassFreeMemory(lineSample* testLine)
{
    if (testLine != NULL) {
        delete testLine;
        testLine = NULL;
    }
}


void TestDistanceClassMemoryAllocationFailure(string, lineSample*, lineSample*, distanceSample*);
void TestDistanceClassFreeMemory(lineSample*, lineSample*, distanceSample*);
/// <summary>
/// Create two line samples and a distance sample from it
/// </summary>
void TestDistanceClass()
{
    shared_ptr<nodeSample> sample1Node1 = NULL;
    shared_ptr<nodeSample> sample1Node2 = NULL;
    shared_ptr<nodeSample> sample2Node1 = NULL;
    shared_ptr<nodeSample> sample2Node2 = NULL;
    lineSample* sample1 = NULL;
    lineSample* sample2 = NULL;
    distanceSample* testDistance = NULL;

    sample1Node1 = shared_ptr<nodeSample>(new nodeSample(1, phasor(250000, 15), phasor(25, -165), 0, phasor(25, 15), 2));
    if (sample1Node1 == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample1Node1", sample1, sample2, testDistance);
        return;
    }
    sample1Node2 = shared_ptr<nodeSample>(new nodeSample(2, phasor(245000, 13), phasor(25, -165), 1, phasor(15, 15), 0,
        phasor(10, 15), 3));
    if (sample1Node2 == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample1Node2", sample1, sample2, testDistance);
        return;
    }
    sample1 = new lineSample(sample1Node1, sample1Node2, true);
    if (sample1 == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample1", sample1, sample2, testDistance);
        return;
    }

    sample2Node1 = shared_ptr<nodeSample>(new nodeSample(1, phasor(252500, 15), phasor(25, -165), 0, phasor(24.75, 15), 2));
    if (sample2Node1 == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample2Node1", sample1, sample2, testDistance);
        return;
    }
    sample2Node2 = shared_ptr<nodeSample>(new nodeSample(2, phasor(250000, 13), phasor(25.25, -165), 1, phasor(15.15, 15), 0,
        phasor(10.10, 15), 3));
    if (sample2Node2 == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample2Node2", sample1, sample2, testDistance);
        return;
    }
    sample2 = new lineSample(sample2Node1, sample2Node2, true);
    if (sample2 == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample2", sample1, sample2, testDistance);
        return;
    }

    testDistance = new distanceSample(sample1, sample2);
    if (testDistance == NULL) {
        TestDistanceClassMemoryAllocationFailure("testDistance", sample1, sample2, testDistance);
        return;
    }
    testDistance->Print();

    // Free Memory
    TestDistanceClassFreeMemory(sample1, sample2, testDistance);
}
/// <summary>
/// Display an error message when the method TestDistanceClass() fails to allocate memory for a variable.
/// </summary>
/// <param name="variableName">The variable the method failed to allocate memory for</param>
void TestDistanceClassMemoryAllocationFailure(string variableName,
    lineSample* sample1, lineSample* sample2, distanceSample* testDistance)
{
    cout << "Error: TestDistanceClass() failed to allocate memory for " << variableName << "\n";
    TestDistanceClassFreeMemory(sample1, sample2, testDistance);
}
/// <summary>
/// Free allocated memory for the method TestDistanceClass().
/// </summary>
void TestDistanceClassFreeMemory(lineSample* sample1, lineSample* sample2, distanceSample* testDistance)
{
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


shared_ptr<nodeSample> TestKnnClassRandomNodeSample(int, phasor, phasor*, int*, int);
void TestKnnClassMemoryAllocationFailure(string, lineSample**, lineSample*, knnPredictionOfUnknownLineSample*,
    phasor*, phasor*, phasor*, phasor*, int*, int*, int);
void TestKnnClassFreeMemory(lineSample**, lineSample*, knnPredictionOfUnknownLineSample*, phasor*, phasor*, phasor*, phasor*,
    int*, int*, int);
/// <summary>
/// Create a set of 'numberOfSamplesWithKnownStatuses' line samples where 'percentOfFailureCases' percent have the line failing.
/// The samples with the line up has average phasors for each parameter as well as the samples with the line down. A line sample
/// that can be either up or down is then created and the KNN algorithm for the class predicts if the line is up or down.
/// </summary>
/// <param name="numberOfSamplesWithKnownStatuses">The number of samples with known line statuses</param>
/// <param name="numberOfNearestNeighbors">The number of nearest neighbors the unknown line status is compared to</param>
/// <param name="percentOfFailureCases">The percentage of the samples with known line statuses that have the line failing</param>
void TestKnnClass(int numberOfSamplesWithKnownStatuses = 100, int numberOfNearestNeighbors = 5, double percentOfFailureCases = 20)
{
    shared_ptr<nodeSample> node1 = NULL;
    shared_ptr<nodeSample> node2 = NULL;
    lineSample** samplesWithKnownStatuses = NULL;
    lineSample* sampleWithUnknownStatus = NULL;
    knnPredictionOfUnknownLineSample* testKNN = NULL;

    int numberOfNode1Currents = 2;
    int numberOfNode2Currents = 2;
    phasor* node1WorkingAverageCurrentPhasors = NULL;
    phasor* node2WorkingAverageCurrentPhasors = NULL;
    phasor* node1NotWorkingAverageCurrentPhasors = NULL;
    phasor* node2NotWorkingAverageCurrentPhasors = NULL;
    int* node1CurrentDestinationNodes = NULL;
    int* node2CurrentDestinationNodes = NULL;

    int numberOfNotWorkingSamples = (int)((double)numberOfSamplesWithKnownStatuses * percentOfFailureCases) / 100;
    int numberOfWorkingSamples = numberOfSamplesWithKnownStatuses - numberOfNotWorkingSamples;


    // Node 1 Averages
    phasor node1WorkingAverageVoltagePhasor = phasor(250000, 15);
    phasor node1NotWorkingAverageVoltagePhasor = phasor(50000, 90);
    
    node1WorkingAverageCurrentPhasors = new phasor[numberOfNode1Currents];
    if (node1WorkingAverageCurrentPhasors == NULL) {
        TestKnnClassMemoryAllocationFailure("node1WorkingAverageCurrentPhasors",
            samplesWithKnownStatuses, sampleWithUnknownStatus, testKNN,
            node1WorkingAverageCurrentPhasors, node2WorkingAverageCurrentPhasors,
            node1NotWorkingAverageCurrentPhasors, node2NotWorkingAverageCurrentPhasors,
            node1CurrentDestinationNodes, node2CurrentDestinationNodes, numberOfSamplesWithKnownStatuses);
        return;
    }
    node1WorkingAverageCurrentPhasors[0] = phasor(25, -165);
    node1WorkingAverageCurrentPhasors[1] = phasor(25, 15);

    node1NotWorkingAverageCurrentPhasors = new phasor[numberOfNode1Currents];
    if (node1NotWorkingAverageCurrentPhasors == NULL) {
        TestKnnClassMemoryAllocationFailure("node1NotWorkingAverageCurrentPhasors",
            samplesWithKnownStatuses, sampleWithUnknownStatus, testKNN,
            node1WorkingAverageCurrentPhasors, node2WorkingAverageCurrentPhasors,
            node1NotWorkingAverageCurrentPhasors, node2NotWorkingAverageCurrentPhasors,
            node1CurrentDestinationNodes, node2CurrentDestinationNodes, numberOfSamplesWithKnownStatuses);
        return;
    }
    node1NotWorkingAverageCurrentPhasors[0] = phasor(250, -135);
    node1NotWorkingAverageCurrentPhasors[1] = phasor(250, 45);

    node1CurrentDestinationNodes = new int[numberOfNode1Currents];
    if (node1CurrentDestinationNodes == NULL) {
        TestKnnClassMemoryAllocationFailure("node1CurrentDestinationNodes", samplesWithKnownStatuses, sampleWithUnknownStatus, testKNN,
            node1WorkingAverageCurrentPhasors, node2WorkingAverageCurrentPhasors,
            node1NotWorkingAverageCurrentPhasors, node2NotWorkingAverageCurrentPhasors,
            node1CurrentDestinationNodes, node2CurrentDestinationNodes, numberOfSamplesWithKnownStatuses);
        return;
    }
    node1CurrentDestinationNodes[0] = 0;
    node1CurrentDestinationNodes[1] = 2;


    // Node 2 Averages
    phasor node2WorkingAverageVoltagePhasor = phasor(250000, 15);
    phasor node2NotWorkingAverageVoltagePhasor = phasor(50000, 90);

    node2WorkingAverageCurrentPhasors = new phasor[numberOfNode2Currents];
    if (node2WorkingAverageCurrentPhasors == NULL) {
        TestKnnClassMemoryAllocationFailure("node2WorkingAverageCurrentPhasors",
            samplesWithKnownStatuses, sampleWithUnknownStatus, testKNN,
            node1WorkingAverageCurrentPhasors, node2WorkingAverageCurrentPhasors,
            node1NotWorkingAverageCurrentPhasors, node2NotWorkingAverageCurrentPhasors,
            node1CurrentDestinationNodes, node2CurrentDestinationNodes, numberOfSamplesWithKnownStatuses);
        return;
    }
    node2WorkingAverageCurrentPhasors[0] = phasor(25, 15);
    node2WorkingAverageCurrentPhasors[1] = phasor(25, -165);

    node2NotWorkingAverageCurrentPhasors = new phasor[numberOfNode2Currents];
    if (node2NotWorkingAverageCurrentPhasors == NULL) {
        TestKnnClassMemoryAllocationFailure("node2NotWorkingAverageCurrentPhasors",
            samplesWithKnownStatuses, sampleWithUnknownStatus, testKNN,
            node1WorkingAverageCurrentPhasors, node2WorkingAverageCurrentPhasors,
            node1NotWorkingAverageCurrentPhasors, node2NotWorkingAverageCurrentPhasors,
            node1CurrentDestinationNodes, node2CurrentDestinationNodes, numberOfSamplesWithKnownStatuses);
        return;
    }
    node2NotWorkingAverageCurrentPhasors[0] = phasor(250, 45);
    node2NotWorkingAverageCurrentPhasors[1] = phasor(250, -135);

    node2CurrentDestinationNodes = new int[numberOfNode2Currents];
    if (node2CurrentDestinationNodes == NULL) {
        TestKnnClassMemoryAllocationFailure("node2CurrentDestinationNodes", samplesWithKnownStatuses, sampleWithUnknownStatus, testKNN,
            node1WorkingAverageCurrentPhasors, node2WorkingAverageCurrentPhasors,
            node1NotWorkingAverageCurrentPhasors, node2NotWorkingAverageCurrentPhasors,
            node1CurrentDestinationNodes, node2CurrentDestinationNodes, numberOfSamplesWithKnownStatuses);
        return;
    }
    node2CurrentDestinationNodes[0] = 0;
    node2CurrentDestinationNodes[1] = 1;


    // Samples With Known Statuses
    samplesWithKnownStatuses = new lineSample*[numberOfSamplesWithKnownStatuses];
    for (int sampleIndex = 0; sampleIndex < numberOfSamplesWithKnownStatuses; sampleIndex++) {
        samplesWithKnownStatuses[sampleIndex] = NULL;
    }

    for (int sampleIndex = 0; sampleIndex < numberOfSamplesWithKnownStatuses; sampleIndex++) {
        if (sampleIndex < numberOfWorkingSamples) {
            node1 = TestKnnClassRandomNodeSample(1, node1WorkingAverageVoltagePhasor, node1WorkingAverageCurrentPhasors,
                node1CurrentDestinationNodes, numberOfNode1Currents);
            if (node1 == NULL) {
                TestKnnClassMemoryAllocationFailure("node1", samplesWithKnownStatuses, sampleWithUnknownStatus, testKNN,
                    node1WorkingAverageCurrentPhasors, node2WorkingAverageCurrentPhasors,
                    node1NotWorkingAverageCurrentPhasors, node2NotWorkingAverageCurrentPhasors,
                    node1CurrentDestinationNodes, node2CurrentDestinationNodes, numberOfSamplesWithKnownStatuses);
                return;
            }

            node2 = TestKnnClassRandomNodeSample(2, node2WorkingAverageVoltagePhasor, node2WorkingAverageCurrentPhasors,
                node2CurrentDestinationNodes, numberOfNode2Currents);
            if (node2 == NULL) {
                TestKnnClassMemoryAllocationFailure("node2", samplesWithKnownStatuses, sampleWithUnknownStatus, testKNN,
                    node1WorkingAverageCurrentPhasors, node2WorkingAverageCurrentPhasors,
                    node1NotWorkingAverageCurrentPhasors, node2NotWorkingAverageCurrentPhasors,
                    node1CurrentDestinationNodes, node2CurrentDestinationNodes, numberOfSamplesWithKnownStatuses);
                return;
            }

            samplesWithKnownStatuses[sampleIndex] = new lineSample(node1, node2, true);
            if (samplesWithKnownStatuses[sampleIndex] == NULL) {
                TestKnnClassMemoryAllocationFailure("samplesWithKnownStatuses[" + to_string(sampleIndex) + "]",
                    samplesWithKnownStatuses, sampleWithUnknownStatus, testKNN,
                    node1WorkingAverageCurrentPhasors, node2WorkingAverageCurrentPhasors,
                    node1NotWorkingAverageCurrentPhasors, node2NotWorkingAverageCurrentPhasors,
                    node1CurrentDestinationNodes, node2CurrentDestinationNodes, numberOfSamplesWithKnownStatuses);
                return;
            }
        }

        else {
            node1 = TestKnnClassRandomNodeSample(1, node1NotWorkingAverageVoltagePhasor, node1NotWorkingAverageCurrentPhasors,
                node1CurrentDestinationNodes, numberOfNode1Currents);
            if (node1 == NULL) {
                TestKnnClassMemoryAllocationFailure("node1", samplesWithKnownStatuses, sampleWithUnknownStatus, testKNN,
                    node1WorkingAverageCurrentPhasors, node2WorkingAverageCurrentPhasors,
                    node1NotWorkingAverageCurrentPhasors, node2NotWorkingAverageCurrentPhasors,
                    node1CurrentDestinationNodes, node2CurrentDestinationNodes, numberOfSamplesWithKnownStatuses);
                return;
            }

            node2 = TestKnnClassRandomNodeSample(2, node2NotWorkingAverageVoltagePhasor, node2NotWorkingAverageCurrentPhasors,
                node2CurrentDestinationNodes, numberOfNode2Currents);
            if (node2 == NULL) {
                TestKnnClassMemoryAllocationFailure("node2", samplesWithKnownStatuses, sampleWithUnknownStatus, testKNN,
                    node1WorkingAverageCurrentPhasors, node2WorkingAverageCurrentPhasors,
                    node1NotWorkingAverageCurrentPhasors, node2NotWorkingAverageCurrentPhasors,
                    node1CurrentDestinationNodes, node2CurrentDestinationNodes, numberOfSamplesWithKnownStatuses);
                return;
            }

            samplesWithKnownStatuses[sampleIndex] = new lineSample(node1, node2, false);
            if (samplesWithKnownStatuses[sampleIndex] == NULL) {
                TestKnnClassMemoryAllocationFailure("samplesWithKnownStatuses[" + to_string(sampleIndex) + "]",
                    samplesWithKnownStatuses, sampleWithUnknownStatus, testKNN,
                    node1WorkingAverageCurrentPhasors, node2WorkingAverageCurrentPhasors,
                    node1NotWorkingAverageCurrentPhasors, node2NotWorkingAverageCurrentPhasors,
                    node1CurrentDestinationNodes, node2CurrentDestinationNodes, numberOfSamplesWithKnownStatuses);
                return;
            }
        }

        node1 = NULL;
        node2 = NULL;
    }


    // Sample With Unknown Status
    bool isWorking = true;
    if (RandomDouble(0, 100) < percentOfFailureCases) isWorking = false;

    if (isWorking == true) {
        node1 = TestKnnClassRandomNodeSample(1, node1WorkingAverageVoltagePhasor, node1WorkingAverageCurrentPhasors,
            node1CurrentDestinationNodes, numberOfNode1Currents);
        if (node1 == NULL) {
            TestKnnClassMemoryAllocationFailure("node1", samplesWithKnownStatuses, sampleWithUnknownStatus, testKNN,
                node1WorkingAverageCurrentPhasors, node2WorkingAverageCurrentPhasors,
                node1NotWorkingAverageCurrentPhasors, node2NotWorkingAverageCurrentPhasors,
                node1CurrentDestinationNodes, node2CurrentDestinationNodes, numberOfSamplesWithKnownStatuses);
            return;
        }

        node2 = TestKnnClassRandomNodeSample(2, node2WorkingAverageVoltagePhasor, node2WorkingAverageCurrentPhasors,
            node2CurrentDestinationNodes, numberOfNode2Currents);
        if (node2 == NULL) {
            TestKnnClassMemoryAllocationFailure("node2", samplesWithKnownStatuses, sampleWithUnknownStatus, testKNN,
                node1WorkingAverageCurrentPhasors, node2WorkingAverageCurrentPhasors,
                node1NotWorkingAverageCurrentPhasors, node2NotWorkingAverageCurrentPhasors,
                node1CurrentDestinationNodes, node2CurrentDestinationNodes, numberOfSamplesWithKnownStatuses);
            return;
        }
    }
    else {
        node1 = TestKnnClassRandomNodeSample(1, node1NotWorkingAverageVoltagePhasor, node1NotWorkingAverageCurrentPhasors,
            node1CurrentDestinationNodes, numberOfNode1Currents);
        if (node1 == NULL) {
            TestKnnClassMemoryAllocationFailure("node1", samplesWithKnownStatuses, sampleWithUnknownStatus, testKNN,
                node1WorkingAverageCurrentPhasors, node2WorkingAverageCurrentPhasors,
                node1NotWorkingAverageCurrentPhasors, node2NotWorkingAverageCurrentPhasors,
                node1CurrentDestinationNodes, node2CurrentDestinationNodes, numberOfSamplesWithKnownStatuses);
            return;
        }

        node2 = TestKnnClassRandomNodeSample(2, node2NotWorkingAverageVoltagePhasor, node2NotWorkingAverageCurrentPhasors,
            node2CurrentDestinationNodes, numberOfNode2Currents);
        if (node2 == NULL) {
            TestKnnClassMemoryAllocationFailure("node2", samplesWithKnownStatuses, sampleWithUnknownStatus, testKNN,
                node1WorkingAverageCurrentPhasors, node2WorkingAverageCurrentPhasors,
                node1NotWorkingAverageCurrentPhasors, node2NotWorkingAverageCurrentPhasors,
                node1CurrentDestinationNodes, node2CurrentDestinationNodes, numberOfSamplesWithKnownStatuses);
            return;
        }
    }

    sampleWithUnknownStatus = new lineSample(node1, node2, isWorking);
    if (sampleWithUnknownStatus == NULL) {
        TestKnnClassMemoryAllocationFailure("sampleWithUnknownStatus", samplesWithKnownStatuses, sampleWithUnknownStatus, testKNN,
            node1WorkingAverageCurrentPhasors, node2WorkingAverageCurrentPhasors,
            node1NotWorkingAverageCurrentPhasors, node2NotWorkingAverageCurrentPhasors,
            node1CurrentDestinationNodes, node2CurrentDestinationNodes, numberOfSamplesWithKnownStatuses);
        return;
    }


    testKNN = new knnPredictionOfUnknownLineSample(samplesWithKnownStatuses, numberOfSamplesWithKnownStatuses,
        sampleWithUnknownStatus);
    if (testKNN == NULL) {
        TestKnnClassMemoryAllocationFailure("testKNN", samplesWithKnownStatuses, sampleWithUnknownStatus, testKNN,
            node1WorkingAverageCurrentPhasors, node2WorkingAverageCurrentPhasors,
            node1NotWorkingAverageCurrentPhasors, node2NotWorkingAverageCurrentPhasors,
            node1CurrentDestinationNodes, node2CurrentDestinationNodes, numberOfSamplesWithKnownStatuses);
        return;
    }
    testKNN->Print();


    TestKnnClassFreeMemory(samplesWithKnownStatuses, sampleWithUnknownStatus, testKNN,
        node1WorkingAverageCurrentPhasors, node2WorkingAverageCurrentPhasors,
        node1NotWorkingAverageCurrentPhasors, node2NotWorkingAverageCurrentPhasors,
        node1CurrentDestinationNodes, node2CurrentDestinationNodes, numberOfSamplesWithKnownStatuses);
}
/// <summary>
/// Generate a node sample with parameter magnitudes between 90% and 110% of the average phasor magnitudes.
/// </summary>
/// <param name="nodeNumber">The node's unique identifying number</param>
/// <param name="averageVoltage">The average voltage phasor</param>
/// <param name="averageCurrents">The array of average current phasors</param>
/// <param name="currentDestinationNodes">The destination node numbers of the currents</param>
/// <param name="numberOfCurrents">The number of currents in the node</param>
/// <returns></returns>
shared_ptr<nodeSample> TestKnnClassRandomNodeSample(int nodeNumber, phasor averageVoltage, phasor* averageCurrents,
    int* currentDestinationNodes, int numberOfCurrents)
{
    phasor* currents = NULL;
    shared_ptr<nodeSample> node = NULL;

    phasor voltage = phasor(RandomDouble(0.9 * averageVoltage.RMSvalue, 1.1 * averageVoltage.RMSvalue),
        averageVoltage.PhaseAngleDegrees);

    currents = new phasor[numberOfCurrents];
    if (currents == NULL) return NULL;
    for (int currentIndex = 0; currentIndex < numberOfCurrents; currentIndex++) {
        currents[currentIndex] =
            phasor(RandomDouble(0.9 * averageCurrents[currentIndex].RMSvalue, 1.1 * averageCurrents[currentIndex].RMSvalue),
                averageCurrents[currentIndex].PhaseAngleDegrees);
    }

    node = shared_ptr<nodeSample>(new nodeSample(nodeNumber, voltage, currents, currentDestinationNodes, numberOfCurrents));
    return node;
}
/// <summary>
/// Display an error message when the method TestKnnClass() fails to allocate memory for a variable.
/// </summary>
/// <param name="variableName">The variable the method failed to allocate memory for</param>
void TestKnnClassMemoryAllocationFailure(string variableName,
    lineSample** samplesWithKnownStatuses, lineSample* sampleWithUnknownStatus, knnPredictionOfUnknownLineSample* testKNN,
    phasor* node1WorkingAverageCurrentPhasors, phasor* node2WorkingAverageCurrentPhasors,
    phasor* node1NotWorkingAverageCurrentPhasors, phasor* node2NotWorkingAverageCurrentPhasors,
    int* node1CurrentDestinationNodes, int* node2CurrentDestinationNodes, int numberOfSamplesWithKnownStatuses)
{
    cout << "Error: TestKnnClass() failed to allocate memory for " << variableName << ".\n";
    TestKnnClassFreeMemory(samplesWithKnownStatuses, sampleWithUnknownStatus, testKNN,
        node1WorkingAverageCurrentPhasors, node2WorkingAverageCurrentPhasors,
        node1NotWorkingAverageCurrentPhasors, node2NotWorkingAverageCurrentPhasors,
        node1CurrentDestinationNodes, node2CurrentDestinationNodes,
        numberOfSamplesWithKnownStatuses);
}
/// <summary>
/// Free allocated memory for the method TestKnnClass().
/// </summary>
void TestKnnClassFreeMemory(
    lineSample** samplesWithKnownStatuses, lineSample* sampleWithUnknownStatus, knnPredictionOfUnknownLineSample* testKNN,
    phasor* node1WorkingAverageCurrentPhasors, phasor* node2WorkingAverageCurrentPhasors,
    phasor* node1NotWorkingAverageCurrentPhasors, phasor* node2NotWorkingAverageCurrentPhasors,
    int* node1CurrentDestinationNodes, int* node2CurrentDestinationNodes, int numberOfSamplesWithKnownStatuses)
{
    if (samplesWithKnownStatuses != NULL) {
        for (int deletingIndex = 0; deletingIndex < numberOfSamplesWithKnownStatuses; deletingIndex++) {
            if (samplesWithKnownStatuses[deletingIndex] != NULL) {
                delete samplesWithKnownStatuses[deletingIndex];
                samplesWithKnownStatuses[deletingIndex] = NULL;
            }
        }
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

    if (node1WorkingAverageCurrentPhasors != NULL) {
        delete[] node1WorkingAverageCurrentPhasors;
        node1WorkingAverageCurrentPhasors = NULL;
    }
    if (node2WorkingAverageCurrentPhasors != NULL) {
        delete[] node2WorkingAverageCurrentPhasors;
        node2WorkingAverageCurrentPhasors = NULL;
    }
    if (node1NotWorkingAverageCurrentPhasors != NULL) {
        delete[] node1NotWorkingAverageCurrentPhasors;
        node1NotWorkingAverageCurrentPhasors = NULL;
    }
    if (node2NotWorkingAverageCurrentPhasors != NULL) {
        delete[] node2NotWorkingAverageCurrentPhasors;
        node2NotWorkingAverageCurrentPhasors = NULL;
    }

    if (node1CurrentDestinationNodes != NULL) {
        delete[] node1CurrentDestinationNodes;
        node1CurrentDestinationNodes = NULL;
    }
    if (node2CurrentDestinationNodes != NULL) {
        delete[] node2CurrentDestinationNodes;
        node2CurrentDestinationNodes = NULL;
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
    TestKnnClass();
}