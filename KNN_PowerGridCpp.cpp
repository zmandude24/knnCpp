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

#include <cmath>
#include <exception>
#include <iostream>
#include <string>


/// <summary>
/// A phasor representation of a sinusoidal AC parameter with the rms value (base units) and the phase angle (degrees). A phasor is a 
/// complex number with real and imaginary parts.
/// </summary>
class phasor {
private:
    /// <summary>
    /// Real part of the complex number
    /// </summary>
    double real = 0;
    /// <summary>
    /// Imaginary part of the complex number
    /// </summary>
    double imaginary = 0;
    /// <summary>
    /// Recalculate 'real' and 'imaginary' from new values of 'rms_value' and 'phase_angle_degrees'
    /// </summary>
    void UpdateRealAndImag()
    {
        real = RMSvalue * cos(M_PI / 180 * PhaseAngleDegrees);
        imaginary = RMSvalue * sin(M_PI / 180 * PhaseAngleDegrees);
    }
    /// <summary>
    /// Recalculate 'rms_value' and 'phase_angle_degrees' from new values of 'real' and 'imaginary'
    /// </summary>
    void UpdateRMSandPhase()
    {
        RMSvalue = hypot(real, imaginary);

        if (real == 0) {   // special case to avoid division by zero
            if (imaginary < 0) PhaseAngleDegrees = -90;       // phasor is on border of Q3 and Q4
            else if (imaginary > 0) PhaseAngleDegrees = 90;   // phasor is on border of Q1 and Q2
            else PhaseAngleDegrees = 0;                       // This is the zero phasor case
        }
        else {
            PhaseAngleDegrees = 180 / M_PI * atan(imaginary / real);
            if (real < 0) {
                if (imaginary >= 0) PhaseAngleDegrees += 180; // phasor should be in Q2 and atan would've put it in Q4
                else PhaseAngleDegrees -= 180;                // phasor should be in Q3 and atan would've put it in Q1
            }
        }
    }

public:
    /// <summary>
    /// magnitude of the complex number
    /// </summary>
    double RMSvalue = 0;
    /// <summary>
    /// phase angle of the complex number
    /// </summary>
    double PhaseAngleDegrees = 0;

    /// <summary>
    /// The default constructor setting all class variable to their defaults.
    /// </summary>
    phasor() {}
    /// <summary>
    /// This is the polar form constructor.
    /// </summary>
    /// <param name="rmsValue">The magnitude of the complex number</param>
    /// <param name="phaseAngleDegrees">The phase angle of the complex number</param>
    phasor(double rmsValue, double phaseAngleDegrees)
    {
        RMSvalue = rmsValue;
        PhaseAngleDegrees = phaseAngleDegrees;
        UpdateRealAndImag();
    }

    /// <summary>
    /// Print the phasor in polar form.
    /// </summary>
    void Print()
    {
        std::cout << RMSvalue << " @ " << PhaseAngleDegrees << "deg\n";
    }
    /// <summary>
    /// Returns a string in the same format PrintPhasor() prints to the terminal (without the new line character at the end)
    /// </summary>
    /// <returns>The phasor in printable string form</returns>
    std::string PhasorToString()
    {
        return std::to_string(RMSvalue) + " @ " + std::to_string(PhaseAngleDegrees) + "deg";
    }

    /// <summary>
    /// Use '+' to add two phasors together like you would with two numbers.
    /// </summary>
    phasor operator+(const phasor& p2)
    {
        phasor sum;
        sum.real = this->real + p2.real;
        sum.imaginary = this->imaginary + p2.imaginary;
        sum.UpdateRMSandPhase();
        return sum;
    }
    /// <summary>
    /// Use '-' to subtract the right phasor from the left phasor like you would with two numbers.
    /// </summary>
    phasor operator-(const phasor& p2)
    {
        phasor difference;
        difference.real = this->real - p2.real;
        difference.imaginary = this->imaginary - p2.imaginary;
        difference.UpdateRMSandPhase();
        return difference;
    }
    /// <summary>
    /// Use '*' to multiply two phasors like you would with two numbers.
    /// </summary>
    phasor operator*(const phasor& p2)
    {
        phasor product;
        product.RMSvalue = this->RMSvalue * p2.RMSvalue;
        product.PhaseAngleDegrees = this->PhaseAngleDegrees + p2.PhaseAngleDegrees;

        // Keep -180deg < phase_angle_degrees <= 180deg
        while (product.PhaseAngleDegrees > 180) product.PhaseAngleDegrees -= 360;
        while (product.PhaseAngleDegrees <= -180) product.PhaseAngleDegrees += 360;

        product.UpdateRealAndImag();
        return product;
    }
    /// <summary>
    /// Use '/' to divide the left phasor by the right phasor like you would with two numbers. In the division by zero case, an
    /// error message will be printed and the zero phasor will be returned.
    /// </summary>
    phasor operator/(const phasor& p2)
    {
        phasor quotient(0, 0);

        try {   // Handle the divide by zero exception
            if (p2.RMSvalue == 0) throw 360;

            quotient.RMSvalue = this->RMSvalue / p2.RMSvalue;
            quotient.PhaseAngleDegrees = this->PhaseAngleDegrees - p2.PhaseAngleDegrees;

            // Keep -180deg < phase_angle_degrees <= 180deg
            while (quotient.PhaseAngleDegrees > 180) quotient.PhaseAngleDegrees -= 360;
            while (quotient.PhaseAngleDegrees <= -180) quotient.PhaseAngleDegrees += 360;
        }
        catch (int e) {
            if (e == 360) std::cout << "Error: Divisor phasor is 0.\n";
            else std::cout << "Exception number " << e << " has occured.\n";
            quotient.RMSvalue = 0;
            quotient.PhaseAngleDegrees = 0;
        }

        quotient.UpdateRealAndImag();
        return quotient;
    }
    /// <summary>
    /// Raise a phasor to a power. In the zero phasor by a non-zero power case, an error message is printed and the zero phasor is
    /// returned.
    /// </summary>
    /// <param name="base">The phasor to be raised to a power</param>
    /// <param name="power">The exponent or power to raise the phasor to</param>
    /// <returns>The phasor with the power applied</returns>
    phasor Pow(phasor base, double power)
    {
        phasor exponent = phasor(0, 0);

        try {   // Handle the case with the 0 phasor to a nonpositive power
            if ((base.RMSvalue == 0) && (power <= 0)) throw 360;

            exponent.RMSvalue = pow(base.RMSvalue, power);
            exponent.PhaseAngleDegrees = base.PhaseAngleDegrees * power;

            // Keep -180deg < phase_angle_degrees <= 180deg
            while (exponent.PhaseAngleDegrees > 180) exponent.PhaseAngleDegrees -= 360;
            while (exponent.PhaseAngleDegrees <= -180) exponent.PhaseAngleDegrees += 360;
        }
        catch (int e) {
            if (e == 360) std::cout << "Error: Base is 0 and power is non-positive.\n";
            else std::cout << "Exception number " << e << " has occured.\n";
            exponent.RMSvalue = 0;
            exponent.PhaseAngleDegrees = 0;
        }

        exponent.UpdateRealAndImag();
        return exponent;
    }
};


/// <summary>
/// An instantaneous measurement of a voltage or current at an exact time
/// </summary>
struct instantaneousMeasurement {
    /// <summary>
    /// The time in seconds
    /// </summary>
    double time;
    /// <summary>
    /// The instantaneous measurement in base units
    /// </summary>
    double value;
};


/// <summary>
/// A parameter of interest in the power grid like a node voltage or line current.
/// </summary>
class parameter {
private:
    /// <summary>
    /// Find the phasor representation of the data in 'samples' that is assumed to be perfectly sinusoidal.
    /// </summary>
    phasor CalculatePhasor()
    {
        if (NumberOfSamples <= 1) {
            std::cout << "Insufficient number of samples to calculate a phasor.\n";
            return phasor();
        }
        return phasor(RMS(), PhaseAngleDegrees(RMS()));
    }
    /// <summary>
    /// The Root Mean Square is calculated with the formula RMS = sqrt(1/n * SUM(xi^2)) where n is the number of samples and xi is
    /// the ith sample.
    /// </summary>
    /// <returns> The RMS value of the values in 'samples'</returns>
    double RMS()
    {
        double rms = 0;
        for (int i = 0; i < NumberOfSamples; i++) rms += pow(Samples[i].value, 2);
        if (NumberOfSamples > 0) rms = sqrt(rms / NumberOfSamples);
        return rms;
    }
    /// <summary>
    /// The phase angle is calculated from the first data point or time = 0 by finding if the sine wave is increasing or decreasing and 
    /// then calculating the arcsine while adjusting if the phase angle is in the second or third quadrant.
    /// </summary>
    /// <param name="rms">The RMS value of the values found by RMS()</param>
    /// <returns>The phase angle of the phasor representation of the values in 'samples' in degrees</returns>
    double PhaseAngleDegrees(double rms)
    {
        double phaseAngleDegrees = 0;

        // Count the phase angle as 90 or -90 if the first point's magnitude is greater than the peak.
        if (Samples[0].value >= rms * sqrt(2)) phaseAngleDegrees = 90;
        else if (Samples[0].value <= -rms * sqrt(2)) phaseAngleDegrees = -90;

        // Ascending
        else if (Samples[1].value >= Samples[0].value) {
            phaseAngleDegrees = 180 / M_PI * asin(Samples[0].value / (rms * sqrt(2)));
        }

        // Descending
        else {
            // Belongs in Q2
            if (Samples[0].value >= 0) {
                phaseAngleDegrees = 180 - 180 / M_PI * asin(Samples[0].value / (rms * sqrt(2)));
            }

            // Belongs in Q4
            else phaseAngleDegrees = -180 - 180 / M_PI * asin(Samples[0].value / (rms * sqrt(2)));
        }

        return phaseAngleDegrees;
    }

    /// <summary>
    /// Free the dynamically allocated memory and set their pointers to NULL.
    /// </summary>
    void FreeMemory()
    {
        if (Samples != NULL) {
            delete[] Samples;
            Samples = NULL;
        }
    }

    /// <summary>
    /// Display an error message and call FreeMemory().
    /// </summary>
    /// <param=variableName>The name of the variable that failed to get memory allocation</param>
    void MemoryAllocationFailure(std::string variableName)
    {
        std::cout << "Error: node() failed to allocate memory for " << variableName << "\n";
        FreeMemory();
    }

public:
    /// <summary>
    /// A dynamically allocated array of the samples.
    /// </summary>
    instantaneousMeasurement* Samples = NULL;
    /// <summary>
    /// The number of samples allocated in 'samples'
    /// </summary>
    int NumberOfSamples = 0;
    /// <summary>
    /// The phasor representation of the data in 'samples'
    /// </summary>
    phasor Phasor = phasor(0, 0);
    /// <summary>
    /// The name of the parameter
    /// </summary>
    std::string Name = "";
    /// <summary>
    /// The base unit name
    /// </summary>
    std::string Units = "";
    /// <summary>
    /// The number of the starting node (0 is ground).
    /// </summary>
    int StartNodeNumber = 0;
    /// <summary>
    /// The number of the destination node (0 is ground).
    /// </summary>
    int DestinationNodeNumber = 0;

    /// <summary>
    /// The default constructor setting all values to their defaults
    /// </summary>
    parameter() {}
    /// <summary>
    /// This constructor calculates the phasor from the array of instantaneous measurements.
    /// </summary>
    /// <param name="samples">The array of instantaneous measurements</param>
    /// <param name="numberOfSamples">The number of elements in the array or instantaneous measurements</param>
    /// <param name="name">The name to be used for the parameter</param>
    /// <param name="units">The base unit suffix</param>
    /// <param name="startNodeNumber">The starting node number (0 is ground)</param>
    /// <param name="destinationNodeNumber">The destination node number (0 is ground)</param>
    parameter(instantaneousMeasurement* samples, int numberOfSamples, std::string name = "", std::string units = "",
        int startNodeNumber = 0, int destinationNodeNumber = 0)
    {
        Samples = samples;
        NumberOfSamples = numberOfSamples;
        Phasor = CalculatePhasor();
        Name = name;
        Units = units;
        StartNodeNumber = startNodeNumber;
        DestinationNodeNumber = destinationNodeNumber;
    }
    /// <summary>
    /// This constructor doesn't involve a set of instantaneous measurements.
    /// </summary>
    /// <param name="phasorr">The phasor representation of the parameter</param>
    /// <param name="name">The name to be used for the parameter</param>
    /// <param name="units">The base unit suffix</param>
    /// <param name="startNodeNumber">The starting node number (0 is ground)</param>
    /// <param name="destinationNodeNumber">The destination node number (0 is ground)</param>
    parameter(phasor phasorr, std::string name = "", std::string units = "",
        int startNodeNumber = 0, int destinationNodeNumber = 0)
    {
        Samples = NULL;
        NumberOfSamples = 0;
        Phasor = phasorr;
        Name = name;
        Units = units;
        StartNodeNumber = startNodeNumber;
        DestinationNodeNumber = destinationNodeNumber;
    }
    /// <summary>
    /// This will free everything in its pointers
    /// </summary>
    ~parameter()
    {
        FreeMemory();
    }

    /// <summary>
    /// Print the name, number of samples, phasor, and the starting and destination node numbers.
    /// </summary>
    void PrintParameter()
    {
        std::cout << "\nName: " << Name << "\n";
        std::cout << "Number of samples: " << NumberOfSamples << "\n";
        std::cout << "Phasor: " << Phasor.PhasorToString() << Units << "\n";
        std::cout << "Starting Node: " << std::to_string(StartNodeNumber) + "\n";
        std::cout << "Destination Node: " << std::to_string(DestinationNodeNumber) + "\n";
    }
};


/// <summary>
/// The data representation of a point on the grid in a specified time range with the voltage and the currents coming from it
/// </summary>
class nodeSample {
private:
    /// <summary>
    /// Free the dynamically allocated memory and set their pointers to NULL.
    /// </summary>
    void FreeMemory()
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
    void MemoryAllocationFailure(std::string variableName)
    {
        std::cout << "Error: node() failed to allocate memory for " << variableName << "\n";
        FreeMemory();
    }
public:
    /// <summary>
    /// The unique identifying number for the node
    /// </summary>
    int NodeNumber = 0;
    /// <summary>
    /// The pointer to the node voltage (startNodeNum = nodeNum and destNodeNum = 0)
    /// </summary>
    parameter* Voltage = NULL;
    /// <summary>
    /// The pointer to the currents (startNodeNum = nodeNum)
    /// </summary>
    parameter** Currents = NULL;
    /// <summary>
    /// The number of currents the dynamically allocated array 'currents' has
    /// </summary>
    int NumberOfCurrents = 0;
    /// <summary>
    /// The rated voltage (typically the voltage at peak normal usage of the grid)
    /// </summary>
    double RatedVoltage = 250000;
    /// <summary>
    /// The rated current (typically the magnitude for a two line node at peak normal usage of the grid)
    /// </summary>
    double RatedCurrent = 25;

    /// <summary>
    /// This constructor uses previously allocated parameters that will be freed on the deconstructor.
    /// </summary>
    /// <param name="nodeNumber">The unique identifying number for the node</param>
    /// <param name="voltage">The pointer to the node voltage (startNodeNum = nodeNum and destNodeNum = 0)</param>
    /// <param name="currents">The pointer to the currents (startNodeNum = nodeNum)</param>
    /// <param name="numberOfCurrents">The number of currents the dynamically allocated array 'currents' has</param>
    /// <param name="ratedVoltage">The rated voltage (typically the voltage at peak normal usage of the grid)</param>
    /// <param name="ratedCurrent">
    /// The rated current (typically the magnitude for a two line node at peak normal usage of the grid)
    /// </param>
    nodeSample(int nodeNumber, parameter* voltage, parameter** currents, int numberOfCurrents,
        double ratedVoltage = 250000, double ratedCurrent = 25)
    {
        NodeNumber = nodeNumber;
        Voltage = voltage;
        Currents = currents;
        NumberOfCurrents = numberOfCurrents;
        RatedVoltage = ratedVoltage;
        RatedCurrent = ratedCurrent;
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
    nodeSample(int nodeNumber, phasor voltage, phasor* currents, int* currentDestinationNodes, int numberOfCurrents,
        double ratedVoltage = 250000, double ratedCurrent = 25)
    {
        NodeNumber = nodeNumber;

        Voltage = new parameter(voltage, "V" + std::to_string(nodeNumber), "V", nodeNumber, 0);
        if (Voltage == NULL) {
            MemoryAllocationFailure("Voltage");
            return;
        }

        if (numberOfCurrents > 0) {
            Currents = new parameter*[numberOfCurrents];
            if (Currents == NULL) {
                MemoryAllocationFailure("Currents");
                return;
            }
            for (int currentsIndex = 0; currentsIndex < numberOfCurrents; currentsIndex++) Currents[currentsIndex] = NULL;

            for (int currentsIndex = 0; currentsIndex < numberOfCurrents; currentsIndex++) {
                Currents[currentsIndex] = new parameter(currents[currentsIndex],
                    "I" + std::to_string(nodeNumber) + std::to_string(currentDestinationNodes[currentsIndex]),
                    "A", nodeNumber, currentDestinationNodes[currentsIndex]);
                if (Currents[currentsIndex] == NULL) {
                    MemoryAllocationFailure("Currents[currentsIndex]");
                    return;
                }
            }
        }
        NumberOfCurrents = numberOfCurrents;

        RatedVoltage = ratedVoltage;
        RatedCurrent = ratedCurrent;
    }

    /// <summary>
    /// This will free everything in its pointers
    /// </summary>
    ~nodeSample()
    {
        FreeMemory();
    }

    /// <summary>
    /// Prints the node number, voltage phasor, and current phasors
    /// </summary>
    void PrintNode()
    {
        std::cout << "Node " << std::to_string(NodeNumber) << "\n";
        if (Voltage != NULL) std::cout << Voltage->Name << " = " << Voltage->Phasor.PhasorToString() << "V\n";
        else std::cout << "'voltage' not set\n";
        if (Currents != NULL) // std::cout << currents[i].name << " = " << currents[i].ph.PhasorToString() << "A\n"
            for (int i = 0; i < NumberOfCurrents; i++) {
                std::cout << Currents[i]->Name << " = " << Currents[i]->Phasor.PhasorToString() << "A\n";
            }
        else std::cout << "'currents' not set\n";
        std::cout << "Rated Voltage: " << std::to_string(RatedVoltage) << "V\n";
        std::cout << "Rated Current: " << std::to_string(RatedCurrent) << "A\n";
    }
};


/// <summary>
/// This contains a pointer to each node of interest as well as if the line is working or not.
/// </summary>
class lineSample {
private:
    /// <summary>
    /// The first node the line is connected to
    /// </summary>
    nodeSample* node1 = NULL;
    /// <summary>
    /// The second node the line is connected to
    /// </summary>
    nodeSample* node2 = NULL;

    /// <summary>
    /// Free the dynamically allocated memory and set their pointers to NULL. Notice that nodeSample* node1 and nodeSample* node2
    /// are not freed here.
    /// </summary>
    void FreeMemory()
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
    void MemoryAllocationFailure(std::string variableName)
    {
        std::cout << "Error: node() failed to allocate memory for " << variableName << "\n";
        FreeMemory();
    }
public:
    /// <summary>
    /// The normalized (magnitude divided by the current rating) current flowing through the line from node 1
    /// </summary>
    parameter* Node1LineCurrentNorm = NULL;
    /// <summary>
    /// The normalized (magnitude divided by the current rating) current flowing through the line from node 2
    /// </summary>
    parameter* Node2LineCurrentNorm = NULL;

    /// <summary>
    /// The normalized (magnitude divided by the voltage rating) voltage at node 1
    /// </summary>
    parameter* Node1VoltageNorm = NULL;
    /// <summary>
    /// The normalized (magnitude divided by the voltage rating) voltage at node 2
    /// </summary>
    parameter* Node2VoltageNorm = NULL;

    /// <summary>
    /// The normalized (magnitude divided by the current rating) currents flowing from node 1 not counting the line current
    /// </summary>
    parameter** Node1OtherCurrentsNorm = NULL;
    /// <summary>
    /// The number of currents flowing from node 1 not counting the line current
    /// </summary>
    int NumberOfNode1OtherCurrents = 0;
    /// <summary>
    /// The normalized (magnitude divided by the current rating) currents flowing from node 2 not counting the line current
    /// </summary>
    parameter** Node2OtherCurrentsNorm = NULL;
    /// <summary>
    /// The number of currents flowing from node 2 not counting the line current
    /// </summary>
    int NumberOfNode2OtherCurrents = 0;

    /// <summary>
    /// The status of the line
    /// </summary>
    bool IsWorking = true;

    /// <summary>
    /// The constructor
    /// </summary>
    /// <param name="node1">The first node of the line</param>
    /// <param name="node2">The second node of the line</param>
    /// <param name="isWorking">The status of the line</param>
    lineSample(nodeSample* node1, nodeSample* node2, bool isWorking)
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
            std::cout << "Error: Unable to find a current in node1 going to node2!\n";
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
            std::cout << "Error: Unable to find a current in node2 going to node1!\n";
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
            Node1OtherCurrentsNorm = new parameter*[NumberOfNode1OtherCurrents];
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
            Node2OtherCurrentsNorm = new parameter*[NumberOfNode2OtherCurrents];
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
    ~lineSample()
    {
        FreeMemory();
    }

    /// <summary>
    /// Print the nodes, line status, and normalized parameters.
    /// </summary>
    void PrintLine()
    {
        std::cout << "\nNode 1:\n";
        node1->PrintNode();
        std::cout << "\nNode 2:\n";
        node2->PrintNode();
        
        std::cout << "\nLine status: " << std::to_string(IsWorking) << "\n";
        std::cout << "Node 1 Normalized Line Current:\n";
        Node1LineCurrentNorm->PrintParameter();
        std::cout << "Node 2 Normalized Line Current:\n";
        Node2LineCurrentNorm->PrintParameter();
        std::cout << "Node 1 Normalized Node Voltage:\n";
        Node1VoltageNorm->PrintParameter();
        std::cout << "Node 2 Normalized Node Voltage:\n";
        Node2VoltageNorm->PrintParameter();
        std::cout << "Node 1 Normalized Other Currents:\n";
        for (int i = 0; i < NumberOfNode1OtherCurrents; i++) Node1OtherCurrentsNorm[i]->PrintParameter();
        std::cout << "Node 2 Normalized Other Currents:\n";
        for (int i = 0; i < NumberOfNode2OtherCurrents; i++) Node2OtherCurrentsNorm[i]->PrintParameter();
    }
};


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
    bool AreSamplesOfTheSameLine(lineSample* sampleWithKnownStatus, lineSample* sampleWithUnknownStatus)
    {
        bool areSamplesOfTheSameLine = true;

        if ((sampleWithKnownStatus == NULL) || (sampleWithUnknownStatus == NULL)) {
            if (sampleWithKnownStatus == NULL) {
                std::cout << "Error in distanceSample(): lineSample* sampleWithKnownStatus = NULL!\n";
            }
            if (sampleWithUnknownStatus == NULL) {
                std::cout << "Error in distanceSample(): lineSample* sampleWithUnknownStatus = NULL!\n";
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

    /// <summary>
    /// Calculate the weighted euclidean distance between the parameters. This method assumes the two line samples were checked to be
    /// of the same line beforehand.
    /// </summary>
    /// <param name="sampleWithKnownStatus">The line sample with the line status known</param>
    /// <param name="sampleWithUnknownStatus">The line sample with the line status unknown</param>
    /// <returns>The weighted euclidean distance</returns>
    double CalculateDistance(lineSample* sampleWithKnownStatus, lineSample* sampleWithUnknownStatus)
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
public:
    /// <summary>
    /// The distance between the line with a known output and the line with the unknown output
    /// </summary>
    double Distance = 10000000000;
    /// <summary>
    /// The status of the known line
    /// </summary>
    bool IsWorking = true;

    /// <summary>
    /// The constructor
    /// </summary>
    /// <param name="sampleWithKnownStatus">the line sample with a known line status</param>
    /// <param name="sampleWithUnknownStatus">the line sample with an unknown line status</param>
    distanceSample(lineSample* sampleWithKnownStatus, lineSample* sampleWithUnknownStatus)
    {
        if (AreSamplesOfTheSameLine(sampleWithKnownStatus, sampleWithUnknownStatus) == false) {
            std::cout << "distanceSample(): lineSample* known and lineSample* unknown are not samples of the same line.\n";
            return;
        }

        line = sampleWithKnownStatus;
        IsWorking = sampleWithKnownStatus->IsWorking;

        Distance = CalculateDistance(sampleWithKnownStatus, sampleWithUnknownStatus);
    }

    /// <summary>
    /// The deconstructor (frees nothing)
    /// </summary>
    ~distanceSample() {}

    /// <summary>
    /// Print the attached known line, weights, distance, and the status of the known line.
    /// </summary>
    void Print() {
        line->PrintLine();
        std::cout << "Wline = " << std::to_string(wLine) << "\n";
        std::cout << "Wnode = " << std::to_string(wNode) << "\n";
        std::cout << "Wother = " << std::to_string(wOther) << "\n";
        std::cout << "distance = " << std::to_string(Distance) << "\n";
        std::cout << "isWorking = " << std::to_string(IsWorking) << "\n";
    }
};


/// <summary>
/// This contains a set of known line samples, an unknown line sample, and predicts the status of the unknown line sample.
/// </summary>
class knn {
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
    /// Sorts the array of distances when a new distance is added to the tail end of the list
    /// </summary>
    /// <param name="i">The index of the knowns the constructor is currently at</param>
    void SortDistances(int i) {
        // Place the new entry on the proper place on the array
        int minimum = numberOfNearestNeighbors - 1;
        if (i < numberOfNearestNeighbors - 1) minimum = i;

        for (int j = minimum; j > 0; j--) {
            // If the distance is less than it's predescesor, swap their positions
            if (distances[j]->Distance < distances[j - 1]->Distance) {
                distanceSample* dummy = new distanceSample(SamplesWithKnownStatuses[i], SampleWithUnknownStatus);
                *dummy = *distances[j - 1];
                *distances[j - 1] = *distances[j];
                *distances[j] = *dummy;
                delete dummy; dummy = NULL;
            }
        }
    }

    /// <summary>
    /// Predicts the line status of the unknown line sample. It will return false in the case of a tie, but k is usually odd and
    /// the statuses of the nearest neighbors are unanimous in virtually every case.
    /// </summary>
    /// <returns>The predicted status of the unknown line sample</returns>
    bool PredictStatus() {
        int numOfWorkingLines = 0;
        int numOfNotWorkingLines = 0;

        for (int i = 0; i < numberOfNearestNeighbors; i++) {
            if (distances[i]->IsWorking == true) numOfWorkingLines += 1;
            else numOfNotWorkingLines += 1;
        }

        if (numOfWorkingLines > numOfNotWorkingLines) return true;
        else return false;
    }

    /// <summary>
    /// Free the dynamically allocated memory and set their pointers to NULL. Notice that only distanceSample** distances is freed.
    /// </summary>
    void FreeMemory()
    {
        if (distances != NULL) {
            for (int i = 0; i < numberOfNearestNeighbors; i++) {
                if (distances[i] != NULL) { delete distances[i]; distances[i] = NULL; }
            }
            delete[] distances; distances = NULL;
        }
    }

    /// <summary>
    /// Display an error message and call FreeMemory().
    /// </summary>
    /// <param=variableName>The name of the variable that failed to get memory allocation</param>
    void MemoryAllocationFailure(std::string variableName)
    {
        std::cout << "Error: node() failed to allocate memory for " << variableName << "\n";
        FreeMemory();
    }
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
    /// The constructor
    /// </summary>
    /// <param name="samplesWithKnownStatuses">The array of line samples with known line statuses</param>
    /// <param name="numberOfKnownStatuses">The number of elements in the samplesWithKnownStatuses array</param>
    /// <param name="sampleWithUnknownStatus">A line sample with an unknown line status</param>
    /// <param name="numberOfNearestNeighbors">The number of nearest neighbors to consider</param>
    knn(lineSample** samplesWithKnownStatuses, int numberOfKnownStatuses, lineSample* sampleWithUnknownStatus,
        int numberOfNearestNeighbors = 5)
    {
        SamplesWithKnownStatuses = samplesWithKnownStatuses;
        NumberOfKnownStatuses = numberOfKnownStatuses;
        SampleWithUnknownStatus = sampleWithUnknownStatus;
        this->numberOfNearestNeighbors = numberOfNearestNeighbors;

        distances = new distanceSample*[numberOfNearestNeighbors];
        if (distances == NULL) {
            MemoryAllocationFailure("distances");
            return;
        }
        for (int initializingIndex = 0; initializingIndex < numberOfNearestNeighbors; initializingIndex++) {
            distances[initializingIndex] = NULL;
        }

        for (int knownSampleIndex = 0; knownSampleIndex < numberOfKnownStatuses; knownSampleIndex++) {
            // Special case where not all elements of distances are filled
            if (knownSampleIndex < numberOfNearestNeighbors) {
                distances[knownSampleIndex] = new distanceSample(samplesWithKnownStatuses[knownSampleIndex], sampleWithUnknownStatus);
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
                    new distanceSample(samplesWithKnownStatuses[knownSampleIndex], sampleWithUnknownStatus);
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

        this->PredictedStatus = PredictStatus();
    }

    /// <summary>
    /// The deconstructor
    /// </summary>
    ~knn()
    {
        FreeMemory();
    }

    /// <summary>
    /// Print the distances of the nearest neighbors and the line status prediction.
    /// </summary>
    void Print() {
        std::cout << "\nKNN Algorithm:\n";
        for (int i = 0; i < numberOfNearestNeighbors; i++) std::cout << "distances[" << std::to_string(i) << "] distance: " <<
            std::to_string(distances[i]->Distance) << "\n";
        std::cout << "Line Status Prediction: " << std::to_string(PredictedStatus) << "\n";
    }
};


void TestDivideByZeroPhasorExceptionMemoryAllocationFailure(std::string, phasor*, phasor*, phasor*);
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
void TestDivideByZeroPhasorExceptionMemoryAllocationFailure(std::string variableName, phasor* p1, phasor* p2, phasor* pdiv)
{
    std::cout << "Error: TestDivideByZeroPhasorException() failed to allocate memory for " << variableName << "\n";
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


void TestZeroToNonPositivePowerExceptionMemoryAllocationFailure(std::string, phasor*, phasor*);
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
void TestZeroToNonPositivePowerExceptionMemoryAllocationFailure(std::string variableName, phasor* ph, phasor* pexp)
{
    std::cout << "Error: TestZeroToNonPositivePowerException() failed to allocate memory for " << variableName << "\n";
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


void TestParameterPhasorCalcAccuracyMemoryAllocationFailure(std::string, phasor*, instantaneousMeasurement*, parameter*, phasor*);
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
        samples[i].time = (double)i / (double)samplesPerSecond;

        // samples[i].value = A * sin(wt + theta)
        double wt = 2 * M_PI * f * samples[i].time;                            // Time dependent angle
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
    std::cout << "\nCalculated Phasor: " << testParameter->Phasor.PhasorToString() << "\n";
    std::cout << "Reference Phasor: " << referencePhasor->PhasorToString() << "\n";

    // Calculate and print the percent error
    difference = new phasor();
    if (difference == NULL) {
        TestParameterPhasorCalcAccuracyMemoryAllocationFailure("difference", referencePhasor, samples, testParameter, difference);
        return;
    }
    *difference = *referencePhasor - testParameter->Phasor;
    double percentError = 100 * difference->RMSvalue / referencePhasor->RMSvalue;
    std::cout << "The percent error is " << percentError << "\n";

    TestParameterPhasorCalcAccuracyFreeMemory(referencePhasor, samples, testParameter, difference);
}
/// <summary>
/// Display an error message when the method TestParameterPhasorCalcAccuracy() fails to allocate memory for a variable.
/// </summary>
void TestParameterPhasorCalcAccuracyMemoryAllocationFailure(std::string variableName, phasor* referencePhasor,
    instantaneousMeasurement* samples, parameter* testParameter, phasor* difference)
{
    std::cout << "Error: TestParameterPhasorCalcAccuracyFreeMemory() failed to allocate memory for " << variableName << "\n";
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


void TestNodeClassMemoryAllocationFailure(std::string, phasor*, int*, nodeSample*);
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
void TestNodeClassMemoryAllocationFailure(std::string variableName, phasor* currents, int* currentDestNodes, nodeSample* testNode)
{
    std::cout << "Error: TestNodeClass() failed to allocate memory for " << variableName << "\n";
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


void TestLineClassMemoryAllocationFailure(std::string, phasor*, phasor*, int*, int*, nodeSample*, nodeSample*, lineSample*);
void TestLineClassFreeMemory(phasor*, phasor*, int*, int*, nodeSample*, nodeSample*, lineSample*);
/// <summary>
/// Create a sample line and print it.
/// </summary>
void TestLineClass()
{
    phasor* node1Currents = NULL;
    phasor* node2Currents = NULL;
    int* node1CurrentDestNodes = NULL;
    int* node2CurrentDestNodes = NULL;
    nodeSample* node1 = NULL;
    nodeSample* node2 = NULL;
    lineSample* testLine = NULL;

    // Node 1
    phasor node1Voltage = phasor(250000, 15);
    node1Currents = new phasor[2];
    if (node1Currents == NULL) {
        TestLineClassMemoryAllocationFailure("node1Currents", node1Currents, node2Currents, node1CurrentDestNodes, node2CurrentDestNodes,
            node1, node2, testLine);
        return;
    }
    node1Currents[0] = phasor(25, -165); node1Currents[1] = phasor(25, 15);
    node1CurrentDestNodes = new int[2];
    if (node1CurrentDestNodes == NULL) {
        TestLineClassMemoryAllocationFailure("node1CurrentDestNodes", node1Currents, node2Currents,
            node1CurrentDestNodes, node2CurrentDestNodes, node1, node2, testLine);
        return;
    }
    node1CurrentDestNodes[0] = 0; node1CurrentDestNodes[1] = 2;
    node1 = new nodeSample(1, node1Voltage, node1Currents, node1CurrentDestNodes, 2);
    if (node1 == NULL) {
        TestLineClassMemoryAllocationFailure("node1", node1Currents, node2Currents, node1CurrentDestNodes, node2CurrentDestNodes,
            node1, node2, testLine);
        return;
    }

    // Node 2
    phasor node2Voltage = phasor(245000, 13);
    node2Currents = new phasor[3];
    if (node2Currents == NULL) {
        TestLineClassMemoryAllocationFailure("node2Currents", node1Currents, node2Currents, node1CurrentDestNodes, node2CurrentDestNodes,
            node1, node2, testLine);
        return;
    }
    node2Currents[0] = phasor(25, -165); node2Currents[1] = phasor(15, 15); node2Currents[2] = phasor(10, 15);
    node2CurrentDestNodes = new int[3];
    if (node2CurrentDestNodes == NULL) {
        TestLineClassMemoryAllocationFailure("node2CurrentDestNodes", node1Currents, node2Currents,
            node1CurrentDestNodes, node2CurrentDestNodes, node1, node2, testLine);
        return;
    }
    node2CurrentDestNodes[0] = 1; node2CurrentDestNodes[1] = 0; node2CurrentDestNodes[2] = 3;
    node2 = new nodeSample(2, node2Voltage, node2Currents, node2CurrentDestNodes, 3);
    if (node2 == NULL) {
        TestLineClassMemoryAllocationFailure("node2", node1Currents, node2Currents, node1CurrentDestNodes, node2CurrentDestNodes,
            node1, node2, testLine);
        return;
    }

    testLine = new lineSample(node1, node2, true);
    if (testLine == NULL) {
        TestLineClassMemoryAllocationFailure("testLine", node1Currents, node2Currents, node1CurrentDestNodes, node2CurrentDestNodes,
            node1, node2, testLine);
        return;
    }
    testLine->PrintLine();

    TestLineClassFreeMemory(node1Currents, node2Currents, node1CurrentDestNodes, node2CurrentDestNodes, node1, node2, testLine);
}
/// <summary>
/// Display an error message when the method TestLineClass() fails to allocate memory for a variable.
/// </summary>
/// <param name="variableName">The variable the method failed to allocate memory for</param>
void TestLineClassMemoryAllocationFailure(std::string variableName, phasor* node1Currents, phasor* node2Currents,
    int* node1CurrentDestNodes, int* node2CurrentDestNodes, nodeSample* node1, nodeSample* node2, lineSample* testLine)
{
    std::cout << "Error: TestLineClass() failed to allocate memory for " << variableName << "\n";
    TestLineClassFreeMemory(node1Currents, node2Currents, node1CurrentDestNodes, node2CurrentDestNodes, node1, node2, testLine);
}
/// <summary>
/// Free allocated memory for the method TestLineClass().
/// </summary>
void TestLineClassFreeMemory(phasor* node1Currents, phasor* node2Currents, int* node1CurrentDestNodes, int* node2CurrentDestNodes,
    nodeSample* node1, nodeSample* node2, lineSample* testLine)
{
    if (node1Currents != NULL) {
        delete[] node1Currents;
        node1Currents = NULL;
    }
    if (node1CurrentDestNodes != NULL) {
        delete[] node1CurrentDestNodes;
        node1CurrentDestNodes = NULL;
    }
    if (node1 != NULL) {
        delete node1;
        node1 = NULL;
    }

    if (node2Currents != NULL) {
        delete[] node2Currents;
        node2Currents = NULL;
    }
    if (node2CurrentDestNodes != NULL) {
        delete[] node2CurrentDestNodes;
        node2CurrentDestNodes = NULL;
    }
    if (node2 != NULL) {
        delete node2;
        node2 = NULL;
    }

    if (testLine != NULL) {
        delete testLine;
        testLine = NULL;
    }
}


void TestDistanceClassMemoryAllocationFailure(std::string, phasor*, phasor*, phasor*, phasor*, int*, int*, int*, int*,
    nodeSample*, nodeSample*, nodeSample*, nodeSample*, lineSample*, lineSample*, distanceSample*);
void TestDistanceClassFreeMemory(phasor*, phasor*, phasor*, phasor*, int*, int*, int*, int*,
    nodeSample*, nodeSample*, nodeSample*, nodeSample*, lineSample*, lineSample*, distanceSample*);
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
    nodeSample* sample1Node1 = NULL;
    nodeSample* sample1Node2 = NULL;
    nodeSample* sample2Node1 = NULL;
    nodeSample* sample2Node2 = NULL;
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
            sample1Node1, sample1Node2, sample2Node1, sample2Node2, sample1, sample2, testDistance);
        return;
    }
    sample1Node1Currents[0] = phasor(25, -165); sample1Node1Currents[1] = phasor(25, 15);
    sample1Node1CurrentDestNodes = new int[2];
    if (sample1Node1CurrentDestNodes == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample1Node1CurrentDestNodes",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1Node1, sample1Node2, sample2Node1, sample2Node2, sample1, sample2, testDistance);
        return;
    }
    sample1Node1CurrentDestNodes[0] = 0; sample1Node1CurrentDestNodes[1] = 2;
    sample1Node1 = new nodeSample(1, sample1Node1Voltage, sample1Node1Currents, sample1Node1CurrentDestNodes, 2);
    if (sample1Node1 == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample1Node1",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1Node1, sample1Node2, sample2Node1, sample2Node2, sample1, sample2, testDistance);
        return;
    }

    // Sample 1, Node 2
    phasor sample1Node2Voltage = phasor(245000, 13);
    sample1Node2Currents = new phasor[3];
    if (sample1Node2Currents == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample1Node2Currents",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1Node1, sample1Node2, sample2Node1, sample2Node2, sample1, sample2, testDistance);
        return;
    }
    sample1Node2Currents[0] = phasor(25, -165); sample1Node2Currents[1] = phasor(15, 15); sample1Node2Currents[2] = phasor(10, 15);
    sample1Node2CurrentDestNodes = new int[3];
    if (sample1Node2CurrentDestNodes == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample1Node2CurrentDestNodes",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1Node1, sample1Node2, sample2Node1, sample2Node2, sample1, sample2, testDistance);
        return;
    }
    sample1Node2CurrentDestNodes[0] = 1; sample1Node2CurrentDestNodes[1] = 0; sample1Node2CurrentDestNodes[2] = 3;
    sample1Node2 = new nodeSample(2, sample1Node2Voltage, sample1Node2Currents, sample1Node2CurrentDestNodes, 3);
    if (sample1Node2 == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample1Node2",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1Node1, sample1Node2, sample2Node1, sample2Node2, sample1, sample2, testDistance);
        return;
    }

    // Sample 1
    sample1 = new lineSample(sample1Node1, sample1Node2, true);
    if (sample1 == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample1",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1Node1, sample1Node2, sample2Node1, sample2Node2, sample1, sample2, testDistance);
        return;
    }

    // Sample 2, Node 1
    phasor sample2Node1Voltage = phasor(252500, 15);
    sample2Node1Currents = new phasor[2];
    if (sample2Node1Currents == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample2Node1Currents",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1Node1, sample1Node2, sample2Node1, sample2Node2, sample1, sample2, testDistance);
        return;
    }
    sample2Node1Currents[0] = phasor(25, -165); sample2Node1Currents[1] = phasor(24.75, 15);
    sample2Node1CurrentDestNodes = new int[2];
    if (sample2Node1CurrentDestNodes == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample2Node1CurrentDestNodes",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1Node1, sample1Node2, sample2Node1, sample2Node2, sample1, sample2, testDistance);
        return;
    }
    sample2Node1CurrentDestNodes[0] = 0; sample2Node1CurrentDestNodes[1] = 2;
    sample2Node1 = new nodeSample(1, sample2Node1Voltage, sample2Node1Currents, sample2Node1CurrentDestNodes, 2);
    if (sample2Node1 == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample2Node1",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1Node1, sample1Node2, sample2Node1, sample2Node2, sample1, sample2, testDistance);
        return;
    }

    // Sample 2, Node 2
    phasor sample2Node2Voltage = phasor(250000, 13);
    sample2Node2Currents = new phasor[3];
    if (sample2Node2Currents == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample2Node2Currents",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1Node1, sample1Node2, sample2Node1, sample2Node2, sample1, sample2, testDistance);
        return;
    }
    sample2Node2Currents[0] = phasor(25.25, -165); sample2Node2Currents[1] = phasor(15.15, 15); sample2Node2Currents[2] = phasor(10.10, 15);
    sample2Node2CurrentDestNodes = new int[3];
    if (sample2Node2CurrentDestNodes == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample2Node2CurrentDestNodes",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1Node1, sample1Node2, sample2Node1, sample2Node2, sample1, sample2, testDistance);
        return;
    }
    sample2Node2CurrentDestNodes[0] = 1; sample2Node2CurrentDestNodes[1] = 0; sample2Node2CurrentDestNodes[2] = 3;
    sample2Node2 = new nodeSample(2, sample2Node2Voltage, sample2Node2Currents, sample2Node2CurrentDestNodes, 3);
    if (sample2Node2 == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample2Node2",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1Node1, sample1Node2, sample2Node1, sample2Node2, sample1, sample2, testDistance);
        return;
    }

    // Sample 2
    sample2 = new lineSample(sample2Node1, sample2Node2, true);
    if (sample2 == NULL) {
        TestDistanceClassMemoryAllocationFailure("sample2",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1Node1, sample1Node2, sample2Node1, sample2Node2, sample1, sample2, testDistance);
        return;
    }

    testDistance = new distanceSample(sample1, sample2);
    if (testDistance == NULL) {
        TestDistanceClassMemoryAllocationFailure("testDistance",
            sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
            sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
            sample1Node1, sample1Node2, sample2Node1, sample2Node2, sample1, sample2, testDistance);
        return;
    }
    testDistance->Print();

    // Free Memory
    TestDistanceClassFreeMemory(sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
        sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
        sample1Node1, sample1Node2, sample2Node1, sample2Node2, sample1, sample2, testDistance);
}
/// <summary>
/// Display an error message when the method TestDistanceClass() fails to allocate memory for a variable.
/// </summary>
/// <param name="variableName">The variable the method failed to allocate memory for</param>
void TestDistanceClassMemoryAllocationFailure(std::string variableName,
    phasor* sample1Node1Currents, phasor* sample1Node2Currents, phasor* sample2Node1Currents, phasor* sample2Node2Currents,
    int* sample1Node1CurrentDestNodes, int* sample1Node2CurrentDestNodes,
    int* sample2Node1CurrentDestNodes, int* sample2Node2CurrentDestNodes,
    nodeSample* sample1Node1, nodeSample* sample1Node2, nodeSample* sample2Node1, nodeSample* sample2Node2,
    lineSample* sample1, lineSample* sample2, distanceSample* testDistance)
{
    std::cout << "Error: TestDistanceClass() failed to allocate memory for " << variableName << "\n";
    TestDistanceClassFreeMemory(sample1Node1Currents, sample1Node2Currents, sample2Node1Currents, sample2Node2Currents,
        sample1Node1CurrentDestNodes, sample1Node2CurrentDestNodes, sample2Node1CurrentDestNodes, sample2Node2CurrentDestNodes,
        sample1Node1, sample1Node2, sample2Node1, sample2Node2, sample1, sample2, testDistance);
}
/// <summary>
/// Free allocated memory for the method TestDistanceClass().
/// </summary>
void TestDistanceClassFreeMemory(
    phasor* sample1Node1Currents, phasor* sample1Node2Currents, phasor* sample2Node1Currents, phasor* sample2Node2Currents,
    int* sample1Node1CurrentDestNodes, int* sample1Node2CurrentDestNodes,
    int* sample2Node1CurrentDestNodes, int* sample2Node2CurrentDestNodes,
    nodeSample* sample1Node1, nodeSample* sample1Node2, nodeSample* sample2Node1, nodeSample* sample2Node2,
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
    if (sample1Node1 != NULL) {
        delete sample1Node1;
        sample1Node1 = NULL;
    }

    if (sample1Node2Currents != NULL) {
        delete[] sample1Node2Currents;
        sample1Node2Currents = NULL;
    }
    if (sample1Node2CurrentDestNodes != NULL) {
        delete[] sample1Node2CurrentDestNodes;
        sample1Node2CurrentDestNodes = NULL;
    }
    if (sample1Node2 != NULL) {
        delete sample1Node2;
        sample1Node2 = NULL;
    }

    if (sample2Node1Currents != NULL) {
        delete[] sample2Node1Currents;
        sample2Node1Currents = NULL;
    }
    if (sample2Node1CurrentDestNodes != NULL) {
        delete[] sample2Node1CurrentDestNodes;
        sample2Node1CurrentDestNodes = NULL;
    }
    if (sample2Node1 != NULL) {
        delete sample2Node1;
        sample2Node1 = NULL;
    }

    if (sample2Node2Currents != NULL) {
        delete[] sample2Node2Currents;
        sample2Node2Currents = NULL;
    }
    if (sample2Node2CurrentDestNodes != NULL) {
        delete[] sample2Node2CurrentDestNodes;
        sample2Node2CurrentDestNodes = NULL;
    }
    if (sample2Node2 != NULL) {
        delete sample2Node2;
        sample2Node2 = NULL;
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


void TestKnnClassMemoryAllocationFailure(std::string, nodeSample**, nodeSample**, phasor*, phasor*, phasor*, phasor*, int*, int*,
    lineSample**, nodeSample*, nodeSample*, lineSample*, knn*, int, int, int);
void TestKnnClassFreeMemory(nodeSample**, nodeSample**, phasor*, phasor*, phasor*, phasor*, int*, int*, lineSample**,
    nodeSample*, nodeSample*, lineSample*, knn*, int, int, int);
/// <summary>
/// Create a set of 10 line samples with known line statuses where 6 have lines working as well as a random line sample with an unknown
/// line status. The ones without the line working will be significantly different. 
/// </summary>
void TestKnnClass(int numberOfLinesWorking = 6, int numberOfLinesNotWorking = 4)
{
    nodeSample** node1SamplesWithKnownStatuses = NULL;
    nodeSample** node2SamplesWithKnownStatuses = NULL;
    phasor* node1WorkingCurrentAveragePhasors = NULL;
    phasor* node2WorkingCurrentAveragePhasors = NULL;
    phasor* node1NotWorkingCurrentAveragePhasors = NULL;
    phasor* node2NotWorkingCurrentAveragePhasors = NULL;
    int* node1CurrentDestNodes = NULL;
    int* node2CurrentDestNodes = NULL;
    lineSample** samplesWithKnownStatuses = NULL;
    nodeSample* node1SampleWithUnknownStatus = NULL;
    nodeSample* node2SampleWithUnknownStatus = NULL;
    lineSample* sampleWithUnknownStatus = NULL;
    knn* testKNN = NULL;

    int node1Number = 1;
    int node2Number = 2;
    int numberOfSamplesWithKnownStatuses = numberOfLinesWorking + numberOfLinesNotWorking;
    int numberOfNode1Currents = 2;
    int numberOfNode2Currents = 2;


    // Node 1 Knowns
    node1SamplesWithKnownStatuses = new nodeSample * [numberOfSamplesWithKnownStatuses];
    if (node1SamplesWithKnownStatuses == NULL) {
        TestKnnClassMemoryAllocationFailure("node1SamplesWithKnownStatuses",
            node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
            node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
            node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
            node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
            node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, sampleWithUnknownStatus, testKNN,
            numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
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
            node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, sampleWithUnknownStatus, testKNN,
            numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
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
            node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, sampleWithUnknownStatus, testKNN,
            numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
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
            node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, sampleWithUnknownStatus, testKNN,
            numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
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
                    node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, sampleWithUnknownStatus, testKNN,
                    numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
                return;
            }
            currents[0] = node1WorkingCurrentAveragePhasors[0] *
                phasor(0.9 + 0.2 * (double)sampleIndex / (double)numberOfLinesWorking, 0);
            currents[1] = node1WorkingCurrentAveragePhasors[1] *
                phasor(0.9 + 0.2 * (double)sampleIndex / (double)numberOfLinesWorking, 0);
            
            node1SamplesWithKnownStatuses[sampleIndex] = new nodeSample(node1Number, voltage, currents, node1CurrentDestNodes, 2);
            if (node1SamplesWithKnownStatuses[sampleIndex] == NULL) {
                TestKnnClassMemoryAllocationFailure("node1SamplesWithKnownStatuses[sampleIndex]",
                    node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
                    node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
                    node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
                    node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
                    node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, sampleWithUnknownStatus, testKNN,
                    numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
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
                    node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, sampleWithUnknownStatus, testKNN,
                    numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
                return;
            }
            currents[0] = node1NotWorkingCurrentAveragePhasors[0] *
                phasor(0.9 + 0.2 * (double)(sampleIndex - numberOfLinesWorking) / (double)numberOfLinesNotWorking, 0);
            currents[1] = node1NotWorkingCurrentAveragePhasors[1] *
                phasor(0.9 + 0.2 * (double)(sampleIndex - numberOfLinesWorking) / (double)numberOfLinesNotWorking, 0);
            
            node1SamplesWithKnownStatuses[sampleIndex] = new nodeSample(node1Number, voltage, currents, node1CurrentDestNodes, 2);
            if (node1SamplesWithKnownStatuses[sampleIndex] == NULL) {
                TestKnnClassMemoryAllocationFailure("node1SamplesWithKnownStatuses[sampleIndex]",
                    node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
                    node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
                    node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
                    node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
                    node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, sampleWithUnknownStatus, testKNN,
                    numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
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
    node2SamplesWithKnownStatuses = new nodeSample * [numberOfSamplesWithKnownStatuses];
    if (node2SamplesWithKnownStatuses == NULL) {
        TestKnnClassMemoryAllocationFailure("node2SamplesWithKnownStatuses",
            node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
            node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
            node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
            node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
            node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, sampleWithUnknownStatus, testKNN,
            numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
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
            node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, sampleWithUnknownStatus, testKNN,
            numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
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
            node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, sampleWithUnknownStatus, testKNN,
            numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
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
            node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, sampleWithUnknownStatus, testKNN,
            numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
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
                    node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, sampleWithUnknownStatus, testKNN,
                    numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
                return;
            }
            currents[0] = node2WorkingCurrentAveragePhasors[0] *
                phasor(0.9 + 0.2 * (double)sampleIndex / (double)numberOfLinesWorking, 0);
            currents[1] = node2WorkingCurrentAveragePhasors[1] *
                phasor(0.9 + 0.2 * (double)sampleIndex / (double)numberOfLinesWorking, 0);

            node2SamplesWithKnownStatuses[sampleIndex] = new nodeSample(node2Number, voltage, currents, node2CurrentDestNodes, 2);
            if (node2SamplesWithKnownStatuses[sampleIndex] == NULL) {
                TestKnnClassMemoryAllocationFailure("node2SamplesWithKnownStatuses[sampleIndex]",
                    node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
                    node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
                    node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
                    node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
                    node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, sampleWithUnknownStatus, testKNN,
                    numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
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
                    node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, sampleWithUnknownStatus, testKNN,
                    numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
                return;
            }
            currents[0] = node2NotWorkingCurrentAveragePhasors[0] *
                phasor(0.9 + 0.2 * (double)(sampleIndex - numberOfLinesWorking) / (double)numberOfLinesNotWorking, 0);
            currents[1] = node2NotWorkingCurrentAveragePhasors[1] *
                phasor(0.9 + 0.2 * (double)(sampleIndex - numberOfLinesWorking) / (double)numberOfLinesNotWorking, 0);

            node2SamplesWithKnownStatuses[sampleIndex] = new nodeSample(node2Number, voltage, currents, node2CurrentDestNodes, 2);
            if (node2SamplesWithKnownStatuses[sampleIndex] == NULL) {
                TestKnnClassMemoryAllocationFailure("node2SamplesWithKnownStatuses[sampleIndex]",
                    node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
                    node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
                    node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
                    node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
                    node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, sampleWithUnknownStatus, testKNN,
                    numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
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
            node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, sampleWithUnknownStatus, testKNN,
            numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
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
                    node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, sampleWithUnknownStatus, testKNN,
                    numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
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
                    node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, sampleWithUnknownStatus, testKNN,
                    numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
            }
        }
    }


    // unknown
    node1SampleWithUnknownStatus = new nodeSample(node1Number, node1WorkingVoltageAveragePhasor,
        node1WorkingCurrentAveragePhasors, node1CurrentDestNodes, 2);
    node2SampleWithUnknownStatus = new nodeSample(node2Number, node2WorkingVoltageAveragePhasor,
        node2WorkingCurrentAveragePhasors, node2CurrentDestNodes, 2);
    sampleWithUnknownStatus = new lineSample(node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, true);


    testKNN = new knn(samplesWithKnownStatuses, numberOfSamplesWithKnownStatuses, sampleWithUnknownStatus, 3);
    testKNN->Print();

    TestKnnClassFreeMemory(node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
        node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
        node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
        node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
        node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, sampleWithUnknownStatus, testKNN,
        numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
}
void TestKnnClassMemoryAllocationFailure(std::string variableName,
    nodeSample** node1SamplesWithKnownStatuses, nodeSample** node2SamplesWithKnownStatuses,
    phasor* node1WorkingCurrentAveragePhasors, phasor* node2WorkingCurrentAveragePhasors,
    phasor* node1NotWorkingCurrentAveragePhasors, phasor* node2NotWorkingCurrentAveragePhasors,
    int* node1CurrentDestNodes, int* node2CurrentDestNodes, lineSample** samplesWithKnownStatuses,
    nodeSample* node1SampleWithUnknownStatus, nodeSample* node2SampleWithUnknownStatus, lineSample* sampleWithUnknownStatus,
    knn* testKNN, int numberOfSamplesWithKnownStatuses, int numberOfNode1Currents, int numberOfNode2Currents)
{
    std::cout << "Error: TestKnnClass() failed to allocate memory for " << variableName << "\n";
    TestKnnClassFreeMemory(node1SamplesWithKnownStatuses, node2SamplesWithKnownStatuses,
        node1WorkingCurrentAveragePhasors, node2WorkingCurrentAveragePhasors,
        node1NotWorkingCurrentAveragePhasors, node2NotWorkingCurrentAveragePhasors,
        node1CurrentDestNodes, node2CurrentDestNodes, samplesWithKnownStatuses,
        node1SampleWithUnknownStatus, node2SampleWithUnknownStatus, sampleWithUnknownStatus, testKNN,
        numberOfSamplesWithKnownStatuses, numberOfNode1Currents, numberOfNode2Currents);
}
/// <summary>
/// Free allocated memory for the method TestKnnClass().
/// </summary>
void TestKnnClassFreeMemory(nodeSample** node1SamplesWithKnownStatuses, nodeSample** node2SamplesWithKnownStatuses,
    phasor* node1WorkingCurrentAveragePhasors, phasor* node2WorkingCurrentAveragePhasors,
    phasor* node1NotWorkingCurrentAveragePhasors, phasor* node2NotWorkingCurrentAveragePhasors,
    int* node1CurrentDestNodes, int* node2CurrentDestNodes, lineSample** samplesWithKnownStatuses,
    nodeSample* node1SampleWithUnknownStatus, nodeSample* node2SampleWithUnknownStatus, lineSample* sampleWithUnknownStatus,
    knn* testKNN, int numberOfSamplesWithKnownStatuses, int numberOfNode1Currents, int numberOfNode2Currents)
{
    if (node1SamplesWithKnownStatuses != NULL) {
        for (int deletingIndex = 0; deletingIndex < numberOfSamplesWithKnownStatuses; deletingIndex++) {
            if (node1SamplesWithKnownStatuses[deletingIndex] != NULL) {
                delete node1SamplesWithKnownStatuses[deletingIndex];
                node1SamplesWithKnownStatuses[deletingIndex] = NULL;
            }
        }
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
        for (int deletingIndex = 0; deletingIndex < numberOfSamplesWithKnownStatuses; deletingIndex++) {
            if (node2SamplesWithKnownStatuses[deletingIndex] != NULL) {
                delete node2SamplesWithKnownStatuses[deletingIndex];
                node2SamplesWithKnownStatuses[deletingIndex] = NULL;
            }
        }
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

    if (node1SampleWithUnknownStatus != NULL) {
        delete node1SampleWithUnknownStatus;
        node1SampleWithUnknownStatus = NULL;
    }
    if (node2SampleWithUnknownStatus != NULL) {
        delete node2SampleWithUnknownStatus;
        node2SampleWithUnknownStatus = NULL;
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