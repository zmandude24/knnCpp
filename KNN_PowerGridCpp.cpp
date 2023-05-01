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
    void UpdateRealAndImag() {
        real = rmsValue * cos(M_PI / 180 * phaseAngleDegrees);
        imaginary = rmsValue * sin(M_PI / 180 * phaseAngleDegrees);
    }
    /// <summary>
    /// Recalculate 'rms_value' and 'phase_angle_degrees' from new values of 'real' and 'imaginary'
    /// </summary>
    void UpdateRMSandPhase() {
        rmsValue = hypot(real, imaginary);

        if (real == 0) {   // special case to avoid division by zero
            if (imaginary < 0) phaseAngleDegrees = -90;       // phasor is on border of Q3 and Q4
            else if (imaginary > 0) phaseAngleDegrees = 90;   // phasor is on border of Q1 and Q2
            else phaseAngleDegrees = 0;                       // This is the zero phasor case
        }
        else {
            phaseAngleDegrees = 180 / M_PI * atan(imaginary / real);
            if (real < 0) {
                if (imaginary >= 0) phaseAngleDegrees += 180; // phasor should be in Q2 and atan would've put it in Q4
                else phaseAngleDegrees -= 180;                // phasor should be in Q3 and atan would've put it in Q1
            }
        }
    }

public:
    /// <summary>
    /// magnitude of the complex number
    /// </summary>
    double rmsValue = 0;
    /// <summary>
    /// phase angle of the complex number
    /// </summary>
    double phaseAngleDegrees = 0;

    phasor() {
        real = 0;
        imaginary = 0;
        rmsValue = 0;
        phaseAngleDegrees = 0;
    }
    phasor(double rmsValue, double phaseAngleDegrees) {
        this->rmsValue = rmsValue;
        this->phaseAngleDegrees = phaseAngleDegrees;
        UpdateRealAndImag();
    }

    /// <summary>
    /// Print the phasor in polar form.
    /// </summary>
    void PrintPhasor() {
        std::cout << rmsValue << " @ " << phaseAngleDegrees << "deg\n";
    }
    /// <summary>
    /// Returns a string in the same format PrintPhasor() prints to the terminal (without the new line character at the end)
    /// </summary>
    /// <returns>The phasor in printable string form</returns>
    std::string PhasorToString() {
        return std::to_string(rmsValue) + " @ " + std::to_string(phaseAngleDegrees) + "deg";
    }

    phasor operator+(const phasor& p2) {
        phasor sum;
        sum.real = this->real + p2.real;
        sum.imaginary = this->imaginary + p2.imaginary;
        sum.UpdateRMSandPhase();
        return sum;
    }
    phasor operator-(const phasor& p2) {
        phasor difference;
        difference.real = this->real - p2.real;
        difference.imaginary = this->imaginary - p2.imaginary;
        difference.UpdateRMSandPhase();
        return difference;
    }
    phasor operator*(const phasor& p2) {
        phasor product;
        product.rmsValue = this->rmsValue * p2.rmsValue;
        product.phaseAngleDegrees = this->phaseAngleDegrees + p2.phaseAngleDegrees;

        // Keep -180deg < phase_angle_degrees <= 180deg
        while (product.phaseAngleDegrees > 180) product.phaseAngleDegrees -= 360;
        while (product.phaseAngleDegrees <= -180) product.phaseAngleDegrees += 360;

        product.UpdateRealAndImag();
        return product;
    }
    phasor operator/(const phasor& p2) {
        phasor quotient(0, 0);

        try {   // Handle the divide by zero exception
            if (p2.rmsValue == 0) throw 360;

            quotient.rmsValue = this->rmsValue / p2.rmsValue;
            quotient.phaseAngleDegrees = this->phaseAngleDegrees - p2.phaseAngleDegrees;

            // Keep -180deg < phase_angle_degrees <= 180deg
            while (quotient.phaseAngleDegrees > 180) quotient.phaseAngleDegrees -= 360;
            while (quotient.phaseAngleDegrees <= -180) quotient.phaseAngleDegrees += 360;
        }
        catch (int e) {
            if (e == 360) std::cout << "Error: Divisor phasor is 0.\n";
            else std::cout << "Exception number " << e << " has occured.\n";
            quotient.rmsValue = 0;
            quotient.phaseAngleDegrees = 0;
        }

        quotient.UpdateRealAndImag();
        return quotient;
    }
    phasor Pow(phasor base, double power) {
        phasor exponent = phasor(0, 0);

        try {   // Handle the case with the 0 phasor to a nonpositive power
            if ((base.rmsValue == 0) && (power <= 0)) throw 360;

            exponent.rmsValue = pow(base.rmsValue, power);
            exponent.phaseAngleDegrees = base.phaseAngleDegrees * power;

            // Keep -180deg < phase_angle_degrees <= 180deg
            while (exponent.phaseAngleDegrees > 180) exponent.phaseAngleDegrees -= 360;
            while (exponent.phaseAngleDegrees <= -180) exponent.phaseAngleDegrees += 360;
        }
        catch (int e) {
            if (e == 360) std::cout << "Error: Base is 0 and power is non-positive.\n";
            else std::cout << "Exception number " << e << " has occured.\n";
            exponent.rmsValue = 0;
            exponent.phaseAngleDegrees = 0;
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
    phasor CalculatePhasor() {
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
    double RMS() {
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
    double PhaseAngleDegrees(double rms) {
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
    phasor ph = phasor(0, 0);
    /// <summary>
    /// The name of the parameter
    /// </summary>
    std::string name = "";
    /// <summary>
    /// The base unit name
    /// </summary>
    std::string units = "";
    /// <summary>
    /// The number of the starting node (0 is ground).
    /// </summary>
    int startNodeNum = 0;
    /// <summary>
    /// The number of the destination node (0 is ground).
    /// </summary>
    int destNodeNum = 0;

    parameter() {
        Samples = NULL;
        NumberOfSamples = 0;
        ph = phasor(0, 0);
        name = "";
        units = "";
        startNodeNum = 0;
        destNodeNum = 0;
    }
    parameter(instantaneousMeasurement* samples, int num_of_samples, std::string name = "", std::string units = "",
        int startNodeNum = 0, int destNodeNum = 0) {
        this->Samples = samples;
        this->NumberOfSamples = num_of_samples;
        this->ph = CalculatePhasor();
        this->name = name;
        this->units = units;
        this->startNodeNum = startNodeNum;
        this->destNodeNum = destNodeNum;
    }
    parameter(phasor ph, std::string name = "", std::string units = "", int startNodeNum = 0, int destNodeNum = 0) {
        this->Samples = NULL;
        this->NumberOfSamples = 0;
        this->ph = ph;
        this->name = name;
        this->units = units;
        this->startNodeNum = startNodeNum;
        this->destNodeNum = destNodeNum;
    }
    /// <summary>
    /// This will free everything in its pointers
    /// </summary>
    ~parameter() {
        if (Samples != NULL) { delete[] Samples; Samples = NULL; }
    }

    /// <summary>
    /// Print the name, number of samples, phasor, and the starting and destination node numbers.
    /// </summary>
    void PrintParameter() {
        std::cout << "\nName: " << name << "\n";
        std::cout << "Number of samples: " << NumberOfSamples << "\n";
        std::cout << "Phasor: " << ph.PhasorToString() << units << "\n";
        std::cout << "Starting Node: " << std::to_string(startNodeNum) + "\n";
        std::cout << "Destination Node: " << std::to_string(destNodeNum) + "\n";
    }
};

/// <summary>
/// The data representation of a point on the grid in a specified time range with the voltage and the currents coming from it
/// </summary>
class nodeSample {
public:
    /// <summary>
    /// The nominal number (should NEVER share this with another node)
    /// </summary>
    int nodeNum = 0;
    /// <summary>
    /// The pointer to the node voltage (startNodeNum = nodeNum and destNodeNum = 0)
    /// </summary>
    parameter* voltage = NULL;
    /// <summary>
    /// The pointer to the currents (startNodeNum = nodeNum)
    /// </summary>
    parameter** currents = NULL;
    /// <summary>
    /// The number of currents the dynamically allocated array 'currents' has.
    /// </summary>
    int numOfCurrents = 0;
    /// <summary>
    /// The rated voltage (typically the voltage at peak normal usage of the grid)
    /// </summary>
    double ratedVoltage = 250000;
    /// <summary>
    /// The rated current (typically the magnitude for a two line node at peak normal usage of the grid)
    /// </summary>
    double ratedCurrent = 25;

    nodeSample(int nodeNum, parameter* voltage, parameter** currents, int numOfCurrents,
        double ratedVoltage = 250000, double ratedCurrent = 25) {
        this->nodeNum = nodeNum;
        this->voltage = voltage;
        this->currents = currents;
        this->numOfCurrents = numOfCurrents;
        this->ratedVoltage = ratedVoltage;
        this->ratedCurrent = ratedCurrent;
    }
    nodeSample(int nodeNum, phasor voltage, phasor* currents, int* currentDestNodes, int numOfCurrents,
        double ratedVoltage = 250000, double ratedCurrent = 25) {
        this->nodeNum = nodeNum;

        this->voltage = new parameter(voltage, "V" + std::to_string(nodeNum), "V", nodeNum, 0);
        if (this->voltage == NULL) {
            std::cout << "Error: node() failed to allocate memory for parameter* voltage!\n";
            return;
        }

        if (numOfCurrents > 0) {
            this->currents = new parameter*[numOfCurrents];
            if (this->currents == NULL) {
                std::cout << "Error: node() failed to allocate memory for parameter** currents!\n";
                if (this->voltage != NULL) { delete this->voltage; this->voltage = NULL; }
                return;
            }
            for (int index = 0; index < numOfCurrents; index++) this->currents[index] = NULL;
            for (int index = 0; index < numOfCurrents; index++) {
                this->currents[index] = new parameter(currents[index],
                    "I" + std::to_string(nodeNum) + std::to_string(currentDestNodes[index]), "A", nodeNum, currentDestNodes[index]);
                if (this->currents[index] == NULL) {
                    std::cout << "Error: node() failed to allocate memory for parameter* currents[i]!\n";
                    for (int freeingIndex = 0; freeingIndex < numOfCurrents; freeingIndex++) {
                        if (this->currents[freeingIndex] != NULL) {
                            delete this->currents[freeingIndex];
                            this->currents[freeingIndex] = NULL;
                        }
                    }
                    delete this->currents; this->currents = NULL;
                    if (this->voltage != NULL) { delete this->voltage; this->voltage = NULL; }
                    return;
                }
            }
        }
        this->numOfCurrents = numOfCurrents;

        this->ratedVoltage = ratedVoltage;
        this->ratedCurrent = ratedCurrent;
    }
    /// <summary>
    /// This will free everything in its pointers
    /// </summary>
    ~nodeSample() {
        if (voltage != NULL) { delete voltage; voltage = NULL; }
        if (currents != NULL) {
            for (int i = 0; i < numOfCurrents; i++) {
                if (currents[i] != NULL) { delete currents[i]; currents[i] = NULL; }
            }
            delete[] currents; currents = NULL;
        }
    }
    /// <summary>
    /// Prints the node number, voltage phasor, and current phasors
    /// </summary>
    void PrintNode() {
        std::cout << "Node " << std::to_string(nodeNum) << "\n";
        if (voltage != NULL) std::cout << voltage->name << " = " << voltage->ph.PhasorToString() << "V\n";
        else std::cout << "'voltage' not set\n";
        if (currents != NULL) // std::cout << currents[i].name << " = " << currents[i].ph.PhasorToString() << "A\n"
            for (int i = 0; i < numOfCurrents; i++) {
                std::cout << currents[i]->name << " = " << currents[i]->ph.PhasorToString() << "A\n";
            }
        else std::cout << "'currents' not set\n";
        std::cout << "Rated Voltage: " << std::to_string(ratedVoltage) << "V\n";
        std::cout << "Rated Current: " << std::to_string(ratedCurrent) << "A\n";
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

public:
    /// <summary>
    /// The normalized (magnitude divided by the current rating) current flowing through the line from node 1
    /// </summary>
    parameter* node1LineCurrentNorm = NULL;
    /// <summary>
    /// The normalized (magnitude divided by the current rating) current flowing through the line from node 2
    /// </summary>
    parameter* node2LineCurrentNorm = NULL;

    /// <summary>
    /// The normalized (magnitude divided by the voltage rating) voltage at node 1
    /// </summary>
    parameter* node1VoltageNorm = NULL;
    /// <summary>
    /// The normalized (magnitude divided by the voltage rating) voltage at node 2
    /// </summary>
    parameter* node2VoltageNorm = NULL;

    /// <summary>
    /// The normalized (magnitude divided by the current rating) currents flowing from node 1 not counting the line current
    /// </summary>
    parameter** node1OtherCurrentsNorm = NULL;
    /// <summary>
    /// The number of currents flowing from node 1 not counting the line current
    /// </summary>
    int numOfNode1OtherCurrents = 0;
    /// <summary>
    /// The normalized (magnitude divided by the current rating) currents flowing from node 2 not counting the line current
    /// </summary>
    parameter** node2OtherCurrentsNorm = NULL;
    /// <summary>
    /// The number of currents flowing from node 2 not counting the line current
    /// </summary>
    int numOfNode2OtherCurrents = 0;

    /// <summary>
    /// The status of the line
    /// </summary>
    bool isWorking = true;

    lineSample(nodeSample* node1, nodeSample* node2, bool isWorking) {
        this->node1 = node1;
        this->node2 = node2;
        this->isWorking = isWorking;

        // Node 1 line current
        node1LineCurrentNorm = new parameter();
        if (node1LineCurrentNorm == NULL) {
            std::cout << "Error: lineSample() failed to allocate memory for node1LineCurrentNorm!\n";
            return;
        }
        bool foundCurrent = false;
        for (int i = 0; i < node1->numOfCurrents; i++) {                // Find the current going to node 2
            if (node1->currents[i]->destNodeNum == node2->nodeNum) {    // Found the current going to node 2
                *node1LineCurrentNorm = *node1->currents[i];
                node1LineCurrentNorm->ph = node1->currents[i]->ph / phasor(node1->ratedCurrent, 0);
                foundCurrent = true;
                break;
            }
        }
        if (foundCurrent == false) {    // Failed to find the current going to node 2
            std::cout << "Error: Unable to find a current in node1 going to node2!\n";
            delete node1LineCurrentNorm; node1LineCurrentNorm = NULL;
            return;
        }

        // Node 2 line current
        node2LineCurrentNorm = new parameter();
        if (node2LineCurrentNorm == NULL) {
            std::cout << "Error: lineSample() failed to allocate memory for node2LineCurrentNorm!\n";
            if (node1LineCurrentNorm != NULL) { delete node1LineCurrentNorm; node1LineCurrentNorm = NULL; }
            return;
        }
        foundCurrent = false;
        for (int i = 0; i < node2->numOfCurrents; i++) {                // Find the current going to node 1
            if (node2->currents[i]->destNodeNum == node1->nodeNum) {    // Found the current going to node 1
                *node2LineCurrentNorm = *node2->currents[i];
                node2LineCurrentNorm->ph = node2->currents[i]->ph / phasor(node2->ratedCurrent, 0);
                foundCurrent = true;
                break;
            }
        }
        if (foundCurrent == false) {    // Failed to find the current going to node 1
            std::cout << "Error: Unable to find a current in node2 going to node1!\n";
            delete node2LineCurrentNorm; node2LineCurrentNorm = NULL;
            if (node1LineCurrentNorm != NULL) { delete node1LineCurrentNorm; node1LineCurrentNorm = NULL; }
            return;
        }

        // Node 1 voltage
        node1VoltageNorm = new parameter();
        if (node1VoltageNorm == NULL) {
            std::cout << "Error: lineSample() failed to allocate memory for node1VoltageNorm!\n";
            if (node2LineCurrentNorm != NULL) { delete node2LineCurrentNorm; node2LineCurrentNorm = NULL; }
            if (node1LineCurrentNorm != NULL) { delete node1LineCurrentNorm; node1LineCurrentNorm = NULL; }
            return;
        }
        *node1VoltageNorm = *node1->voltage;
        node1VoltageNorm->ph = node1->voltage->ph / phasor(node1->ratedVoltage, 0);

        // Node 2 voltage
        node2VoltageNorm = new parameter();
        if (node2VoltageNorm == NULL) {
            std::cout << "Error: lineSample() failed to allocate memory for node2VoltageNorm!\n";
            if (node1VoltageNorm != NULL) { delete node1VoltageNorm; node1VoltageNorm = NULL; }
            if (node2LineCurrentNorm != NULL) { delete node2LineCurrentNorm; node2LineCurrentNorm = NULL; }
            if (node1LineCurrentNorm != NULL) { delete node1LineCurrentNorm; node1LineCurrentNorm = NULL; }
            return;
        }
        *node2VoltageNorm = *node2->voltage;
        node2VoltageNorm->ph = node2->voltage->ph / phasor(node2->ratedVoltage, 0);

        // Node 1 other currents
        if (node1->numOfCurrents <= 1) numOfNode1OtherCurrents = 0;
        else {
            numOfNode1OtherCurrents = node1->numOfCurrents - 1;
            node1OtherCurrentsNorm = new parameter*[numOfNode1OtherCurrents];
            if (node1OtherCurrentsNorm == NULL) {
                std::cout << "Error: lineSample() failed to allocate memory for node1OtherCurrentsNorm!\n";
                if (node2VoltageNorm != NULL) { delete node2VoltageNorm; node2VoltageNorm = NULL; }
                if (node1VoltageNorm != NULL) { delete node1VoltageNorm; node1VoltageNorm = NULL; }
                if (node2LineCurrentNorm != NULL) { delete node2LineCurrentNorm; node2LineCurrentNorm = NULL; }
                if (node1LineCurrentNorm != NULL) { delete node1LineCurrentNorm; node1LineCurrentNorm = NULL; }
                return;
            }
            for (int i = 0; i < numOfNode1OtherCurrents; i++) node1OtherCurrentsNorm[i] = NULL;

            foundCurrent = false;
            for (int i = 0; i < node1->numOfCurrents; i++) {
                if (foundCurrent == false) {
                    if (node1->currents[i]->destNodeNum == node2->nodeNum) foundCurrent = true;
                    else {
                        node1OtherCurrentsNorm[i] = new parameter();
                        if (node1OtherCurrentsNorm[i] == NULL) {
                            std::cout << "Error: lineSample() failed to allocate memory for node1OtherCurrentsNorm[i]!\n";
                            for (int j = 0; j < numOfNode1OtherCurrents; j++) {
                                if (node1OtherCurrentsNorm[j] != NULL) {
                                    delete node1OtherCurrentsNorm[j];
                                    node1OtherCurrentsNorm[j] = NULL;
                                }
                            }
                            delete node1OtherCurrentsNorm; node1OtherCurrentsNorm = NULL;
                            if (node1VoltageNorm != NULL) { delete node1VoltageNorm; node1VoltageNorm = NULL; }
                            if (node2LineCurrentNorm != NULL) { delete node2LineCurrentNorm; node2LineCurrentNorm = NULL; }
                            if (node1LineCurrentNorm != NULL) { delete node1LineCurrentNorm; node1LineCurrentNorm = NULL; }
                            return;
                        }
                        *node1OtherCurrentsNorm[i] = *node1->currents[i];
                        node1OtherCurrentsNorm[i]->ph = node1->currents[i]->ph / phasor(node1->ratedCurrent, 0);
                    }
                }
                else {
                    node1OtherCurrentsNorm[i - 1] = new parameter();
                    if (node1OtherCurrentsNorm[i - 1] == NULL) {
                        std::cout << "Error: lineSample() failed to allocate memory for node1OtherCurrentsNorm[i]!\n";
                        for (int j = 0; j < numOfNode1OtherCurrents; j++) {
                            if (node1OtherCurrentsNorm[j] != NULL) {
                                delete node1OtherCurrentsNorm[j];
                                node1OtherCurrentsNorm[j] = NULL;
                            }
                        }
                        delete node1OtherCurrentsNorm; node1OtherCurrentsNorm = NULL;
                        if (node1VoltageNorm != NULL) { delete node1VoltageNorm; node1VoltageNorm = NULL; }
                        if (node2LineCurrentNorm != NULL) { delete node2LineCurrentNorm; node2LineCurrentNorm = NULL; }
                        if (node1LineCurrentNorm != NULL) { delete node1LineCurrentNorm; node1LineCurrentNorm = NULL; }
                        return;
                    }
                    *node1OtherCurrentsNorm[i - 1] = *node1->currents[i];
                    node1OtherCurrentsNorm[i - 1]->ph = node1->currents[i]->ph / phasor(node1->ratedCurrent, 0);
                }
            }
        }

        // Node 2 other currents
        if (node2->numOfCurrents <= 1) numOfNode2OtherCurrents = 0;
        else {
            numOfNode2OtherCurrents = node2->numOfCurrents - 1;
            node2OtherCurrentsNorm = new parameter*[numOfNode2OtherCurrents];
            if (node2OtherCurrentsNorm == NULL) {
                std::cout << "Error: lineSample() failed to allocate memory for node2OtherCurrentsNorm!\n";
                if (node1OtherCurrentsNorm != NULL) {
                    for (int i = 0; i < numOfNode1OtherCurrents; i++) {
                        if (node1OtherCurrentsNorm[i] != NULL) {
                            delete node1OtherCurrentsNorm[i];
                            node1OtherCurrentsNorm[i] = NULL;
                        }
                    }
                    delete[] node1OtherCurrentsNorm; node1OtherCurrentsNorm = NULL;
                }
                if (node2VoltageNorm != NULL) { delete node2VoltageNorm; node2VoltageNorm = NULL; }
                if (node1VoltageNorm != NULL) { delete node1VoltageNorm; node1VoltageNorm = NULL; }
                if (node2LineCurrentNorm != NULL) { delete node2LineCurrentNorm; node2LineCurrentNorm = NULL; }
                if (node1LineCurrentNorm != NULL) { delete node1LineCurrentNorm; node1LineCurrentNorm = NULL; }
                return;
            }
            for (int i = 0; i < numOfNode2OtherCurrents; i++) node2OtherCurrentsNorm[i] = NULL;

            foundCurrent = false;
            for (int i = 0; i < node2->numOfCurrents; i++) {
                if (foundCurrent == false) {
                    if (node2->currents[i]->destNodeNum == node1->nodeNum) foundCurrent = true;
                    else {
                        node2OtherCurrentsNorm[i] = new parameter();
                        if (node2OtherCurrentsNorm[i] == NULL) {
                            std::cout << "Error: lineSample() failed to allocate memory for node2OtherCurrentsNorm[i]!\n";
                            for (int j = 0; j < numOfNode1OtherCurrents; j++) {
                                if (node2OtherCurrentsNorm[j] != NULL) {
                                    delete node2OtherCurrentsNorm[j];
                                    node2OtherCurrentsNorm[j] = NULL;
                                }
                            }
                            delete node2OtherCurrentsNorm; node2OtherCurrentsNorm = NULL;
                            if (node1OtherCurrentsNorm != NULL) {
                                for (int i = 0; i < numOfNode1OtherCurrents; i++) {
                                    if (node1OtherCurrentsNorm[i] != NULL) {
                                        delete node1OtherCurrentsNorm[i];
                                        node1OtherCurrentsNorm[i] = NULL;
                                    }
                                }
                                delete[] node1OtherCurrentsNorm; node1OtherCurrentsNorm = NULL;
                            }
                            if (node1VoltageNorm != NULL) { delete node1VoltageNorm; node1VoltageNorm = NULL; }
                            if (node2LineCurrentNorm != NULL) { delete node2LineCurrentNorm; node2LineCurrentNorm = NULL; }
                            if (node1LineCurrentNorm != NULL) { delete node1LineCurrentNorm; node1LineCurrentNorm = NULL; }
                            return;
                        }
                        *node2OtherCurrentsNorm[i] = *node2->currents[i];
                        node2OtherCurrentsNorm[i]->ph = node2->currents[i]->ph / phasor(node2->ratedCurrent, 0);
                    }
                }
                else {
                    node2OtherCurrentsNorm[i - 1] = new parameter();
                    if (node2OtherCurrentsNorm[i - 1] == NULL) {
                        std::cout << "Error: lineSample() failed to allocate memory for node2OtherCurrentsNorm[i]!\n";
                        for (int j = 0; j < numOfNode2OtherCurrents; j++) {
                            if (node2OtherCurrentsNorm[j] != NULL) {
                                delete node2OtherCurrentsNorm[j];
                                node2OtherCurrentsNorm[j] = NULL;
                            }
                        }
                        delete node2OtherCurrentsNorm; node2OtherCurrentsNorm = NULL;
                        if (node1OtherCurrentsNorm != NULL) {
                            for (int i = 0; i < numOfNode1OtherCurrents; i++) {
                                if (node1OtherCurrentsNorm[i] != NULL) {
                                    delete node1OtherCurrentsNorm[i];
                                    node1OtherCurrentsNorm[i] = NULL;
                                }
                            }
                            delete[] node1OtherCurrentsNorm; node1OtherCurrentsNorm = NULL;
                        }
                        if (node1VoltageNorm != NULL) { delete node1VoltageNorm; node1VoltageNorm = NULL; }
                        if (node2LineCurrentNorm != NULL) { delete node2LineCurrentNorm; node2LineCurrentNorm = NULL; }
                        if (node1LineCurrentNorm != NULL) { delete node1LineCurrentNorm; node1LineCurrentNorm = NULL; }
                        return;
                    }
                    *node2OtherCurrentsNorm[i - 1] = *node2->currents[i];
                    node2OtherCurrentsNorm[i - 1]->ph = node2->currents[i]->ph / phasor(node2->ratedCurrent, 0);
                }
            }
        }
    }
    ~lineSample() {
        if (node1LineCurrentNorm != NULL) { delete node1LineCurrentNorm; node1LineCurrentNorm = NULL; }
        if (node2LineCurrentNorm != NULL) { delete node2LineCurrentNorm; node2LineCurrentNorm = NULL; }
        if (node1VoltageNorm != NULL) { delete node1VoltageNorm; node1VoltageNorm = NULL; }
        if (node2VoltageNorm != NULL) { delete node2VoltageNorm; node2VoltageNorm = NULL; }
        if (node1OtherCurrentsNorm != NULL) {
            for (int i = 0; i < numOfNode1OtherCurrents; i++) {
                if (node1OtherCurrentsNorm[i] != NULL) { delete node1OtherCurrentsNorm[i]; node1OtherCurrentsNorm[i] = NULL; }
            }
            delete node1OtherCurrentsNorm; node1OtherCurrentsNorm = NULL;
        }
        if (node2OtherCurrentsNorm != NULL) {
            for (int i = 0; i < numOfNode2OtherCurrents; i++) {
                if (node2OtherCurrentsNorm[i] != NULL) { delete node2OtherCurrentsNorm[i]; node2OtherCurrentsNorm[i] = NULL; }
            }
            delete node2OtherCurrentsNorm; node2OtherCurrentsNorm = NULL;
        }
    }

    void PrintLine() {
        std::cout << "\nNode 1:\n";
        node1->PrintNode();
        std::cout << "\nNode 2:\n";
        node2->PrintNode();
        
        std::cout << "\nLine status: " << std::to_string(isWorking) << "\n";
        std::cout << "Node 1 Normalized Line Current:\n";
        node1LineCurrentNorm->PrintParameter();
        std::cout << "Node 2 Normalized Line Current:\n";
        node2LineCurrentNorm->PrintParameter();
        std::cout << "Node 1 Normalized Node Voltage:\n";
        node1VoltageNorm->PrintParameter();
        std::cout << "Node 2 Normalized Node Voltage:\n";
        node2VoltageNorm->PrintParameter();
        std::cout << "Node 1 Normalized Other Currents:\n";
        for (int i = 0; i < numOfNode1OtherCurrents; i++) node1OtherCurrentsNorm[i]->PrintParameter();
        std::cout << "Node 2 Normalized Other Currents:\n";
        for (int i = 0; i < numOfNode2OtherCurrents; i++) node2OtherCurrentsNorm[i]->PrintParameter();
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
    double Wline = 20;
    /// <summary>
    /// The weight of the node voltages
    /// </summary>
    double Wnode = 4;
    /// <summary>
    /// The weight of the other currents flowing from the nodes not counting the line currents
    /// </summary>
    double Wother = 1;
    /// <summary>
    /// Verify if the line samples are samples of the same line.
    /// </summary>
    /// <param name="known">The line sample with the status known</param>
    /// <param name="unknown">The line sample with the status unknown</param>
    /// <returns>True if the samples are of the same line</returns>
    bool AreSamplesOfTheSameLine(lineSample* known, lineSample* unknown) {
        if ((known == NULL) || (unknown == NULL)) {
            if (known == NULL) std::cout << "Error in distanceSample(): lineSample* known = NULL!\n";
            if (unknown == NULL) std::cout << "Error in distanceSample(): lineSample* unknown = NULL!\n";
            return false;
        }

        // Check Line Currents
        if (known->node1LineCurrentNorm->startNodeNum != unknown->node1LineCurrentNorm->startNodeNum) return false;
        if (known->node1LineCurrentNorm->destNodeNum != unknown->node1LineCurrentNorm->destNodeNum) return false;
        if (known->node2LineCurrentNorm->startNodeNum != unknown->node2LineCurrentNorm->startNodeNum) return false;
        if (known->node2LineCurrentNorm->destNodeNum != unknown->node2LineCurrentNorm->destNodeNum) return false;

        // Check Node Voltages
        if (known->node1VoltageNorm->startNodeNum != unknown->node1VoltageNorm->startNodeNum) return false;
        if (known->node2VoltageNorm->startNodeNum != unknown->node2VoltageNorm->startNodeNum) return false;

        // Check Other Currents
        if (known->numOfNode1OtherCurrents != unknown->numOfNode1OtherCurrents) return false;
        for (int i = 0; i < known->numOfNode1OtherCurrents; i++) {
            if (known->node1OtherCurrentsNorm[i]->startNodeNum != unknown->node1OtherCurrentsNorm[i]->startNodeNum) return false;
            if (known->node1OtherCurrentsNorm[i]->destNodeNum != unknown->node1OtherCurrentsNorm[i]->destNodeNum) return false;
        }
        if (known->numOfNode2OtherCurrents != unknown->numOfNode2OtherCurrents) return false;
        for (int i = 0; i < known->numOfNode2OtherCurrents; i++) {
            if (known->node2OtherCurrentsNorm[i]->startNodeNum != unknown->node2OtherCurrentsNorm[i]->startNodeNum) return false;
            if (known->node2OtherCurrentsNorm[i]->destNodeNum != unknown->node2OtherCurrentsNorm[i]->destNodeNum) return false;
        }

        return true;
    }
    /// <summary>
    /// Calculate the weighted euclidean distance between the parameters. This method assumes the two line samples were checked to be
    /// of the same line beforehand.
    /// </summary>
    /// <param name="known">The line sample with the line status known</param>
    /// <param name="unknown">The line sample with the line status unknown</param>
    /// <returns>The weighted euclidean distance</returns>
    double CalculateDistance(lineSample* known, lineSample* unknown) {
        double dist = 0;

        // The line currents
        dist += Wline / 2 * pow((known->node1LineCurrentNorm->ph - unknown->node1LineCurrentNorm->ph).rmsValue, 2);
        dist += Wline / 2 * pow((known->node2LineCurrentNorm->ph - unknown->node2LineCurrentNorm->ph).rmsValue, 2);

        // The node voltages
        dist += Wnode / 2 * pow((known->node1VoltageNorm->ph - unknown->node1VoltageNorm->ph).rmsValue, 2);
        dist += Wnode / 2 * pow((known->node2VoltageNorm->ph - unknown->node2VoltageNorm->ph).rmsValue, 2);

        // The other currents
        double n = (double)known->numOfNode1OtherCurrents;
        for (int i = 0; i < known->numOfNode1OtherCurrents; i++) {
            dist += Wother / (2 * n) * pow((known->node1OtherCurrentsNorm[i]->ph - unknown->node1OtherCurrentsNorm[i]->ph).rmsValue, 2);
        }
        n = (double)known->numOfNode2OtherCurrents;
        for (int i = 0; i < known->numOfNode2OtherCurrents; i++) {
            dist += Wother / (2 * n) * pow((known->node2OtherCurrentsNorm[i]->ph - unknown->node2OtherCurrentsNorm[i]->ph).rmsValue, 2);
        }

        dist = sqrt(dist);
        return dist;
    }
public:
    /// <summary>
    /// The distance between the line with a known output and the line with the unknown output
    /// </summary>
    double distance = 10000000000;
    /// <summary>
    /// The status of the known line
    /// </summary>
    bool isWorking = true;
    distanceSample(lineSample* known, lineSample* unknown) {
        if (AreSamplesOfTheSameLine(known, unknown) == false) {
            std::cout << "distanceSample(): lineSample* known and lineSample* unknown are not samples of the same line.\n";
            return;
        }

        line = known;
        isWorking = known->isWorking;

        distance = CalculateDistance(known, unknown);
    }
    ~distanceSample() {}

    /// <summary>
    /// Print the attached known line, weights, distance, and the status of the known line.
    /// </summary>
    void Print() {
        line->PrintLine();
        std::cout << "Wline = " << std::to_string(Wline) << "\n";
        std::cout << "Wnode = " << std::to_string(Wnode) << "\n";
        std::cout << "Wother = " << std::to_string(Wother) << "\n";
        std::cout << "distance = " << std::to_string(distance) << "\n";
        std::cout << "isWorking = " << std::to_string(isWorking) << "\n";
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
    int k = 5;
    /// <summary>
    /// Sorts the array of distances when a new distance is added to the tail end of the list
    /// </summary>
    /// <param name="i">The index of the knowns the constructor is currently at</param>
    void SortDistances(int i) {
        // Place the new entry on the proper place on the array
        int minimum = k - 1;
        if (i < k - 1) minimum = i;

        for (int j = minimum; j > 0; j--) {
            // If the distance is less than it's predescesor, swap their positions
            if (distances[j]->distance < distances[j - 1]->distance) {
                distanceSample* dummy = new distanceSample(knowns[i], unknown);
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

        for (int i = 0; i < k; i++) {
            if (distances[i]->isWorking == true) numOfWorkingLines += 1;
            else numOfNotWorkingLines += 1;
        }

        if (numOfWorkingLines > numOfNotWorkingLines) return true;
        else return false;
    }
public:
    /// <summary>
    /// The array of line samples with a known line status
    /// </summary>
    lineSample** knowns = NULL;
    /// <summary>
    /// The number of known line samples
    /// </summary>
    int numOfKnowns = 0;
    /// <summary>
    /// The line sample with an unknown line status
    /// </summary>
    lineSample* unknown = NULL;
    /// <summary>
    /// The predicted status
    /// </summary>
    bool predictedStatus = true;
    knn(lineSample** knowns, int numOfKnowns, lineSample* unknown, int k = 5) {
        this->knowns = knowns;
        this->numOfKnowns = numOfKnowns;
        this->unknown = unknown;
        this->k = k;

        distances = new distanceSample*[k];
        if (distances == NULL) {
            std::cout << "Error: knn() failed to allocate memory for distanceSample** distances!\n";
            this->knowns = NULL;
            this->numOfKnowns = 0;
            this->unknown = NULL;
            return;
        }
        for (int i = 0; i < k; i++) distances[i] = NULL;
        for (int i = 0; i < numOfKnowns; i++) {
            // Special case where not all elements of distances are filled
            if (i < k) {
                distances[i] = new distanceSample(knowns[i], unknown);
                if (distances[i] == NULL) {
                    std::cout << "Error: knn() failed to allocate memory for distanceSample* distances[i]!\n";
                    for (int j = 0; j < k; j++) {
                        if (distances[j] != NULL) { delete distances[j]; distances[j] = NULL; }
                    }
                    delete[] distances; distances = NULL;
                    this->knowns = NULL;
                    this->numOfKnowns = 0;
                    this->unknown = NULL;
                    return;
                }
                if (i > 0) {
                    SortDistances(i);
                }
            }

            else {
                distanceSample* distanceAtIndexI = new distanceSample(knowns[i], unknown);
                if (distanceAtIndexI == NULL) {
                    std::cout << "Error: knn() failed to allocate memory for distanceSample* distanceAtIndexI!\n";
                    for (int j = 0; j < k; j++) {
                        if (distances[j] != NULL) { delete distances[j]; distances[j] = NULL; }
                    }
                    delete[] distances; distances = NULL;
                    this->knowns = NULL;
                    this->numOfKnowns = 0;
                    this->unknown = NULL;
                    return;
                }
                if (distanceAtIndexI->distance < distances[k - 1]->distance) {
                    delete distances[k - 1]; distances[k - 1] = distanceAtIndexI; distanceAtIndexI = NULL;
                    SortDistances(i);
                }
                else { delete distanceAtIndexI; distanceAtIndexI = NULL; }
            }
        }

        this->predictedStatus = PredictStatus();
    }
    ~knn() {
        if (distances != NULL) {
            for (int i = 0; i < k; i++) {
                if (distances[i] != NULL) { delete distances[i]; distances[i] = NULL; }
            }
            delete[] distances; distances = NULL;
        }
    }

    void Print() {
        std::cout << "\nKNN Algorithm:\n";
        for (int i = 0; i < k; i++) std::cout << "distances[" << std::to_string(i) << "] distance: " <<
            std::to_string(distances[i]->distance) << "\n";
        std::cout << "Line Status Prediction: " << std::to_string(predictedStatus) << "\n";
    }
};


// Prototypes
void TestDivideByZeroPhasorException();
void TestZeroToNonPositivePowerException();
void TestParameterPhasorCalcAccuracy();
void TestNodeClass();
void TestLineClass();
void TestDistanceClass();
void TestKnnClass(int, int);


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


/// <summary>
/// Attempt to divide a phasor by a zero phasor where the special case catch should output "Error: Divisor phasor is 0." 
/// </summary>
void TestDivideByZeroPhasorException()
{
    phasor* p1 = new phasor(5, 30);
    if (p1 == NULL) { std::cout << "Error: unable to allocate memory for phasor* p1!\n"; return; }

    phasor* p2 = new phasor(0, 0);
    if (p2 == NULL) {
        std::cout << "Error: unable to allocate memory for phasor* p2!\n";
        if (p1 != NULL) { delete p1; p1 = NULL; }
        return;
    }

    phasor* pdiv = new phasor();
    if (pdiv == NULL) {
        std::cout << "Error: unable to allocate memory for phasor* pdiv!\n";
        if (p1 != NULL) { delete p1; p1 = NULL; }
        if (p2 != NULL) { delete p2; p2 = NULL; }
        return;
    }
    *pdiv = *p1 / *p2;

    if (p1 != NULL) { delete p1; p1 = NULL; }
    if (p2 != NULL) { delete p2; p2 = NULL; }
    if (pdiv != NULL) { delete pdiv; pdiv = NULL; }
}

/// <summary>
/// Attempt to raise a zero phasor to a non-positive power where the special case catch should output "Error: Base is 0 and power
/// is non-positive."
/// </summary>
void TestZeroToNonPositivePowerException()
{
    phasor* ph = new phasor(0, 0);
    if (ph == NULL) { std::cout << "Error: unable to allocate memory for phasor* ph!\n"; return; }

    //phasor pexp = ph->Pow(*ph, 0);
    phasor* pexp = new phasor();
    if (pexp == NULL) {
        std::cout << "Error: unable to allocate memory for phasor* pexp!\n";
        if (ph != NULL) { delete ph; ph = NULL; }
        return;
    }
    *pexp = ph->Pow(*ph, 0);

    if (ph != NULL) { delete ph; ph = NULL; }
    if (pexp != NULL) { delete pexp; pexp = NULL; }
}

/// <summary>
/// This will test the accuracy of the parameter class's phasor calculation by creating sample points from a reference phasor and
/// the phasor calculated will be compared to the reference phasor. 
/// </summary>
void TestParameterPhasorCalcAccuracy()
{
    phasor* referencePhasor = new phasor(120, 30);
    if (referencePhasor == NULL) { std::cout << "Error: unable ot allocate memory for phasor* referencePhasor!\n"; return; }
    int samplesPerSecond = 32000;
    double totalSamplingTime = 1;
    double f = 60;  // sine wave frequency in Hz

    // Create the array of samples
    int numOfSamples = (int)((double)samplesPerSecond * totalSamplingTime);
    instantaneousMeasurement* samples = new instantaneousMeasurement[numOfSamples];
    if (samples == NULL) {
        std::cout << "Error: unable to allocate memory for sample* samples!\n";
        if (referencePhasor != NULL) { delete referencePhasor; referencePhasor = NULL; }
        return;
    }
    for (int i = 0; i < numOfSamples; i++) {
        samples[i].time = (double)i / (double)samplesPerSecond;

        // samples[i].value = A * sin(wt + theta)
        double wt = 2 * M_PI * f * samples[i].time;                            // Time dependent angle
        double A = sqrt(2) * referencePhasor->rmsValue;                     // Amplitude
        double theta = M_PI / 180 * referencePhasor->phaseAngleDegrees;     // Angle offset
        samples[i].value = A * sin(wt + theta);
    }

    // Allocate the test parameter
    parameter* testParameter = new parameter(samples, numOfSamples, "V1", "V", 1, 0);
    if (testParameter == NULL) {
        std::cout << "Error: unable to allocate memory for parameter* testParameter!\n";
        if (samples != NULL) { delete[] samples; samples = NULL; }
        if (referencePhasor != NULL) { delete referencePhasor; referencePhasor = NULL; }
        return;
    }
    samples = NULL;

    // Print the test parameter, the calculated phasor, and the reference phasor
    testParameter->PrintParameter();
    std::cout << "\nCalculated Phasor: " << testParameter->ph.PhasorToString() << "\n";
    std::cout << "Reference Phasor: " << referencePhasor->PhasorToString() << "\n";

    // Calculate and print the percent error
    phasor* difference = new phasor();
    if (difference == NULL) {
        std::cout << "Error: unable to allocate memory for phasor* difference!\n";
        if (testParameter != NULL) { delete testParameter; testParameter = NULL; }
        if (referencePhasor != NULL) { delete referencePhasor; referencePhasor = NULL; }
        return;
    }
    *difference = *referencePhasor - testParameter->ph;
    double percentError = 100 * difference->rmsValue / referencePhasor->rmsValue;
    std::cout << "The percent error is " << percentError << "\n";

    // Free Memory
    if (difference != NULL) { delete difference; difference = NULL; }
    if (testParameter != NULL) { delete testParameter; testParameter = NULL; }
    if (referencePhasor != NULL) { delete referencePhasor; referencePhasor = NULL; }
}

/// <summary>
/// Create a sample node and print it.
/// </summary>
void TestNodeClass()
{
    phasor voltage = phasor(250000, 15);

    phasor* currents = new phasor[2];
    currents[0] = phasor(25, -165); currents[1] = phasor(25, 15);
    
    int* currentDestNodes = new int[2];
    currentDestNodes[0] = 0; currentDestNodes[1] = 2;

    nodeSample* testNode = new nodeSample(1, voltage, currents, currentDestNodes, 2);
    testNode->PrintNode();

    if (testNode != NULL) { delete testNode; testNode = NULL; }
    if (currentDestNodes != NULL) { delete[] currentDestNodes; currentDestNodes = NULL; }
    if (currents != NULL) { delete[] currents; currents = NULL; }
}

/// <summary>
/// Create a sample line and print it.
/// </summary>
void TestLineClass()
{
    // Node 1
    phasor node1Voltage = phasor(250000, 15);
    phasor* node1Currents = new phasor[2];
    node1Currents[0] = phasor(25, -165); node1Currents[1] = phasor(25, 15);
    int* node1CurrentDestNodes = new int[2];
    node1CurrentDestNodes[0] = 0; node1CurrentDestNodes[1] = 2;
    nodeSample* node1 = new nodeSample(1, node1Voltage, node1Currents, node1CurrentDestNodes, 2);

    // Node 2
    phasor node2Voltage = phasor(245000, 13);
    phasor* node2Currents = new phasor[3];
    node2Currents[0] = phasor(25, -165); node2Currents[1] = phasor(15, 15); node2Currents[2] = phasor(10, 15);
    int* node2CurrentDestNodes = new int[3];
    node2CurrentDestNodes[0] = 1; node2CurrentDestNodes[1] = 0; node2CurrentDestNodes[2] = 3;
    nodeSample* node2 = new nodeSample(2, node2Voltage, node2Currents, node2CurrentDestNodes, 3);

    lineSample* testLine = new lineSample(node1, node2, true);
    testLine->PrintLine();

    // Free Memory
    if (testLine != NULL) { delete testLine; testLine = NULL; }
    if (node2 != NULL) { delete node2; node2 = NULL; }
    if (node2CurrentDestNodes != NULL) { delete[] node2CurrentDestNodes; node2CurrentDestNodes = NULL; }
    if (node2Currents != NULL) { delete[] node2Currents; node2Currents = NULL; }
    if (node1 != NULL) { delete node1; node1 = NULL; }
    if (node1CurrentDestNodes != NULL) { delete[] node1CurrentDestNodes; node1CurrentDestNodes = NULL; }
    if (node1Currents != NULL) { delete[] node1Currents; node1Currents = NULL; }
}

/// <summary>
/// Create two line samples and a distance sample from it
/// </summary>
void TestDistanceClass()
{
    // Sample 1, Node 1
    phasor sample1Node1Voltage = phasor(250000, 15);
    phasor* sample1Node1Currents = new phasor[2];
    sample1Node1Currents[0] = phasor(25, -165); sample1Node1Currents[1] = phasor(25, 15);
    int* sample1Node1CurrentDestNodes = new int[2];
    sample1Node1CurrentDestNodes[0] = 0; sample1Node1CurrentDestNodes[1] = 2;
    nodeSample* sample1Node1 = new nodeSample(1, sample1Node1Voltage, sample1Node1Currents, sample1Node1CurrentDestNodes, 2);

    // Sample 1, Node 2
    phasor sample1Node2Voltage = phasor(245000, 13);
    phasor* sample1Node2Currents = new phasor[3];
    sample1Node2Currents[0] = phasor(25, -165); sample1Node2Currents[1] = phasor(15, 15); sample1Node2Currents[2] = phasor(10, 15);
    int* sample1Node2CurrentDestNodes = new int[3];
    sample1Node2CurrentDestNodes[0] = 1; sample1Node2CurrentDestNodes[1] = 0; sample1Node2CurrentDestNodes[2] = 3;
    nodeSample* sample1Node2 = new nodeSample(2, sample1Node2Voltage, sample1Node2Currents, sample1Node2CurrentDestNodes, 3);
    
    // Sample 1
    lineSample* sample1 = new lineSample(sample1Node1, sample1Node2, true);

    // Sample 2, Node 1
    phasor sample2Node1Voltage = phasor(252500, 15);
    phasor* sample2Node1Currents = new phasor[2];
    sample2Node1Currents[0] = phasor(25, -165); sample2Node1Currents[1] = phasor(24.75, 15);
    int* sample2Node1CurrentDestNodes = new int[2];
    sample2Node1CurrentDestNodes[0] = 0; sample2Node1CurrentDestNodes[1] = 2;
    nodeSample* sample2Node1 = new nodeSample(1, sample2Node1Voltage, sample2Node1Currents, sample2Node1CurrentDestNodes, 2);

    // Sample 2, Node 2
    phasor sample2Node2Voltage = phasor(250000, 13);
    phasor* sample2Node2Currents = new phasor[3];
    sample2Node2Currents[0] = phasor(25.25, -165); sample2Node2Currents[1] = phasor(15.15, 15); sample2Node2Currents[2] = phasor(10.10, 15);
    int* sample2Node2CurrentDestNodes = new int[3];
    sample2Node2CurrentDestNodes[0] = 1; sample2Node2CurrentDestNodes[1] = 0; sample2Node2CurrentDestNodes[2] = 3;
    nodeSample* sample2Node2 = new nodeSample(2, sample2Node2Voltage, sample2Node2Currents, sample2Node2CurrentDestNodes, 3);

    // Sample 2
    lineSample* sample2 = new lineSample(sample2Node1, sample2Node2, true);

    distanceSample* testDistance = new distanceSample(sample1, sample2);
    testDistance->Print();

    // Free Memory
    if (testDistance != NULL) { delete testDistance; testDistance = NULL; }
    if (sample2 != NULL) { delete sample2; sample2 = NULL; }
    if (sample2Node2 != NULL) { delete sample2Node2; sample2Node2 = NULL; }
    if (sample2Node2CurrentDestNodes != NULL) { delete[] sample2Node2CurrentDestNodes; sample2Node2CurrentDestNodes = NULL; }
    if (sample2Node2Currents != NULL) { delete[] sample2Node2Currents; sample2Node2Currents = NULL; }
    if (sample2Node1 != NULL) { delete sample2Node1; sample2Node1 = NULL; }
    if (sample2Node1CurrentDestNodes != NULL) { delete[] sample2Node1CurrentDestNodes; sample2Node1CurrentDestNodes = NULL; }
    if (sample2Node1Currents != NULL) { delete[] sample2Node1Currents; sample2Node1Currents = NULL; }
    if (sample1 != NULL) { delete sample1; sample1 = NULL; }
    if (sample1Node2 != NULL) { delete sample1Node2; sample1Node2 = NULL; }
    if (sample1Node2CurrentDestNodes != NULL) { delete[] sample1Node2CurrentDestNodes; sample1Node2CurrentDestNodes = NULL; }
    if (sample1Node2Currents != NULL) { delete[] sample1Node2Currents; sample1Node2Currents = NULL; }
    if (sample1Node1 != NULL) { delete sample1Node1; sample1Node1 = NULL; }
    if (sample1Node1CurrentDestNodes != NULL) { delete[] sample1Node1CurrentDestNodes; sample1Node1CurrentDestNodes = NULL; }
    if (sample1Node1Currents != NULL) { delete[] sample1Node1Currents; sample1Node1Currents = NULL; }
}

/// <summary>
/// Create a set of 10 line samples with known line statuses where 6 have lines working as well as a random line sample with an unknown
/// line status. The ones without the line working will be significantly different. 
/// </summary>
void TestKnnClass(int numOfLineWorking = 6, int numOfLineNotWorking = 4)
{
    int node1Num = 1; int node2Num = 2;
    int numOfKnowns = numOfLineWorking + numOfLineNotWorking;


    // Node 1 Knowns
    nodeSample** node1Knowns = new nodeSample*[numOfKnowns];
    if (node1Knowns == NULL) {
        std::cout << "Error: TestKnnClass() failed to allocate memory for nodeSample** node1Knowns!\n";
        return;
    }
    for (int i = 0; i < numOfKnowns; i++) node1Knowns[i] = NULL;

    phasor node1WorkingVoltageAveragePhasor = phasor(250000, 15);
    phasor node1NotWorkingVoltageAveragePhasor = phasor(50000, -150);

    phasor* node1WorkingCurrentAveragePhasors = new phasor[2];
    if (node1WorkingCurrentAveragePhasors == NULL) {
        std::cout << "Error: TestKnnClass() failed to allocate memory for phasor* node1WorkingCurrentAveragePhasors!\n";
        if (node1Knowns != NULL) {
            for (int i = 0; i < numOfKnowns; i++) {
                if (node1Knowns[i] != NULL) { delete node1Knowns[i]; node1Knowns[i] = NULL; }
            }
            delete[] node1Knowns; node1Knowns = NULL;
        }
        return;
    }
    node1WorkingCurrentAveragePhasors[0] = phasor(25, 165); node1WorkingCurrentAveragePhasors[1] = phasor(25, -15);
    phasor* node1NotWorkingCurrentAveragePhasors = new phasor[2];
    if (node1NotWorkingCurrentAveragePhasors == NULL) {
        std::cout << "Error: TestKnnClass() failed to allocate memory for phasor* node1NotWorkingCurrentAveragePhasors!\n";
        if (node1WorkingCurrentAveragePhasors != NULL) {
            delete[] node1WorkingCurrentAveragePhasors;
            node1WorkingCurrentAveragePhasors = NULL;
        }
        if (node1Knowns != NULL) {
            for (int i = 0; i < numOfKnowns; i++) {
                if (node1Knowns[i] != NULL) { delete node1Knowns[i]; node1Knowns[i] = NULL; }
            }
            delete[] node1Knowns; node1Knowns = NULL;
        }
        return;
    }
    node1NotWorkingCurrentAveragePhasors[0] = phasor(250, -70); node1NotWorkingCurrentAveragePhasors[1] = phasor(250, 110);

    int* node1CurrentDestNodes = new int[2];
    if (node1CurrentDestNodes == NULL) {
        std::cout << "Error: TestKnnClass() failed to allocate memory for int* node1CurrentDestNodes!\n";
        if (node1NotWorkingCurrentAveragePhasors != NULL) {
            delete[] node1NotWorkingCurrentAveragePhasors;
            node1NotWorkingCurrentAveragePhasors = NULL;
        }
        if (node1WorkingCurrentAveragePhasors != NULL) {
            delete[] node1WorkingCurrentAveragePhasors;
            node1WorkingCurrentAveragePhasors = NULL;
        }
        if (node1Knowns != NULL) {
            for (int i = 0; i < numOfKnowns; i++) {
                if (node1Knowns[i] != NULL) { delete node1Knowns[i]; node1Knowns[i] = NULL; }
            }
            delete[] node1Knowns; node1Knowns = NULL;
        }
        return;
    }
    node1CurrentDestNodes[0] = 0; node1CurrentDestNodes[1] = 2;

    for (int i = 0; i < numOfKnowns; i++) {
        // Create node sample where the line is working
        if (i < numOfLineWorking) {
            phasor voltage = node1WorkingVoltageAveragePhasor * phasor(0.9 + 0.2 * (double)i / (double)numOfLineWorking, 0);
            phasor* currents = new phasor[2];
            if (currents == NULL) {
                std::cout << "Error: TestKnnClass() failed to allocate memory for phasor* currents!\n";
                if (node1CurrentDestNodes != NULL) { delete[] node1CurrentDestNodes; node1CurrentDestNodes = NULL; }
                if (node1NotWorkingCurrentAveragePhasors != NULL) {
                    delete[] node1NotWorkingCurrentAveragePhasors;
                    node1NotWorkingCurrentAveragePhasors = NULL;
                }
                if (node1WorkingCurrentAveragePhasors != NULL) {
                    delete[] node1WorkingCurrentAveragePhasors;
                    node1WorkingCurrentAveragePhasors = NULL;
                }
                if (node1Knowns != NULL) {
                    for (int j = 0; j < numOfKnowns; j++) {
                        if (node1Knowns[j] != NULL) { delete node1Knowns[j]; node1Knowns[j] = NULL; }
                    }
                    delete[] node1Knowns; node1Knowns = NULL;
                }
                return;
            }
            currents[0] = node1WorkingCurrentAveragePhasors[0] * phasor(0.9 + 0.2 * (double)i / (double)numOfLineWorking, 0);
            currents[1] = node1WorkingCurrentAveragePhasors[1] * phasor(0.9 + 0.2 * (double)i / (double)numOfLineWorking, 0);
            node1Knowns[i] = new nodeSample(node1Num, voltage, currents, node1CurrentDestNodes, 2);
            if (node1Knowns[i] == NULL) {
                std::cout << "Error: TestKnnClass() failed to allocate memory for nodeSample* node1Knowns[i]!\n";
                if (currents != NULL) { delete[] currents; currents = NULL; }
                if (node1CurrentDestNodes != NULL) { delete[] node1CurrentDestNodes; node1CurrentDestNodes = NULL; }
                if (node1NotWorkingCurrentAveragePhasors != NULL) {
                    delete[] node1NotWorkingCurrentAveragePhasors;
                    node1NotWorkingCurrentAveragePhasors = NULL;
                }
                if (node1WorkingCurrentAveragePhasors != NULL) {
                    delete[] node1WorkingCurrentAveragePhasors;
                    node1WorkingCurrentAveragePhasors = NULL;
                }
                if (node1Knowns != NULL) {
                    for (int j = 0; j < numOfKnowns; j++) {
                        if (node1Knowns[j] != NULL) { delete node1Knowns[j]; node1Knowns[j] = NULL; }
                    }
                    delete[] node1Knowns; node1Knowns = NULL;
                }
                return;
            }
            if (currents != NULL) { delete[] currents; currents = NULL; }
        }

        // Create node sample where the line is not working
        else {
            phasor voltage = node1NotWorkingVoltageAveragePhasor *
                phasor(0.9 + 0.2 * (double)(i - numOfLineWorking) / (double)numOfLineNotWorking, 0);
            phasor* currents = new phasor[2];
            if (currents == NULL) {
                std::cout << "Error: TestKnnClass() failed to allocate memory for phasor* currents!\n";
                if (node1CurrentDestNodes != NULL) { delete[] node1CurrentDestNodes; node1CurrentDestNodes = NULL; }
                if (node1NotWorkingCurrentAveragePhasors != NULL) {
                    delete[] node1NotWorkingCurrentAveragePhasors;
                    node1NotWorkingCurrentAveragePhasors = NULL;
                }
                if (node1WorkingCurrentAveragePhasors != NULL) {
                    delete[] node1WorkingCurrentAveragePhasors;
                    node1WorkingCurrentAveragePhasors = NULL;
                }
                if (node1Knowns != NULL) {
                    for (int j = 0; j < numOfKnowns; j++) {
                        if (node1Knowns[j] != NULL) { delete node1Knowns[j]; node1Knowns[j] = NULL; }
                    }
                    delete[] node1Knowns; node1Knowns = NULL;
                }
                return;
            }
            currents[0] = node1NotWorkingCurrentAveragePhasors[0] *
                phasor(0.9 + 0.2 * (double)(i - numOfLineWorking) / (double)numOfLineNotWorking, 0);
            currents[1] = node1NotWorkingCurrentAveragePhasors[1] *
                phasor(0.9 + 0.2 * (double)(i - numOfLineWorking) / (double)numOfLineNotWorking, 0);
            node1Knowns[i] = new nodeSample(node1Num, voltage, currents, node1CurrentDestNodes, 2);
            if (node1Knowns[i] == NULL) {
                std::cout << "Error: TestKnnClass() failed to allocate memory for nodeSample* node1Knowns[i]!\n";
                if (currents != NULL) { delete[] currents; currents = NULL; }
                if (node1CurrentDestNodes != NULL) { delete[] node1CurrentDestNodes; node1CurrentDestNodes = NULL; }
                if (node1NotWorkingCurrentAveragePhasors != NULL) {
                    delete[] node1NotWorkingCurrentAveragePhasors;
                    node1NotWorkingCurrentAveragePhasors = NULL;
                }
                if (node1WorkingCurrentAveragePhasors != NULL) {
                    delete[] node1WorkingCurrentAveragePhasors;
                    node1WorkingCurrentAveragePhasors = NULL;
                }
                if (node1Knowns != NULL) {
                    for (int j = 0; j < numOfKnowns; j++) {
                        if (node1Knowns[j] != NULL) { delete node1Knowns[j]; node1Knowns[j] = NULL; }
                    }
                    delete[] node1Knowns; node1Knowns = NULL;
                }
                return;
            }
            if (currents != NULL) { delete[] currents; currents = NULL; }
        }
    }


    // Node 2 Knowns
    nodeSample** node2Knowns = new nodeSample * [numOfKnowns];
    if (node2Knowns == NULL) {
        std::cout << "Error: TestKnnClass() failed to allocate memory for nodeSample** node2Knowns!\n";
        if (node1Knowns != NULL) {
            for (int i = 0; i < numOfKnowns; i++) {
                if (node1Knowns[i] != NULL) { delete node1Knowns[i]; node1Knowns[i] = NULL; }
            }
            delete[] node1Knowns; node1Knowns = NULL;
        }
        return;
    }
    for (int i = 0; i < numOfKnowns; i++) node2Knowns[i] = NULL;

    phasor node2WorkingVoltageAveragePhasor = phasor(250000, 15);
    phasor node2NotWorkingVoltageAveragePhasor = phasor(75000, -120);

    phasor* node2WorkingCurrentAveragePhasors = new phasor[2];
    if (node2WorkingCurrentAveragePhasors == NULL) {
        std::cout << "Error: TestKnnClass() failed to allocate memory for phasor* node2WorkingCurrentAveragePhasors!\n";
        if (node2Knowns != NULL) {
            for (int i = 0; i < numOfKnowns; i++) {
                if (node2Knowns[i] != NULL) { delete node2Knowns[i]; node2Knowns[i] = NULL; }
            }
            delete[] node2Knowns; node2Knowns = NULL;
        }
        if (node1Knowns != NULL) {
            for (int i = 0; i < numOfKnowns; i++) {
                if (node1Knowns[i] != NULL) { delete node1Knowns[i]; node1Knowns[i] = NULL; }
            }
            delete[] node1Knowns; node1Knowns = NULL;
        }
        return;
    }
    node2WorkingCurrentAveragePhasors[0] = phasor(25, -15); node2WorkingCurrentAveragePhasors[1] = phasor(25, 165);
    phasor* node2NotWorkingCurrentAveragePhasors = new phasor[2];
    if (node2NotWorkingCurrentAveragePhasors == NULL) {
        std::cout << "Error: TestKnnClass() failed to allocate memory for phasor* node2NotWorkingCurrentAveragePhasors!\n";
        if (node2WorkingCurrentAveragePhasors != NULL) {
            delete[] node2WorkingCurrentAveragePhasors;
            node2WorkingCurrentAveragePhasors = NULL;
        }
        if (node2Knowns != NULL) {
            for (int i = 0; i < numOfKnowns; i++) {
                if (node2Knowns[i] != NULL) { delete node2Knowns[i]; node2Knowns[i] = NULL; }
            }
            delete[] node2Knowns; node2Knowns = NULL;
        }
        if (node1Knowns != NULL) {
            for (int i = 0; i < numOfKnowns; i++) {
                if (node1Knowns[i] != NULL) { delete node1Knowns[i]; node1Knowns[i] = NULL; }
            }
            delete[] node1Knowns; node1Knowns = NULL;
        }
        return;
    }
    node2NotWorkingCurrentAveragePhasors[0] = phasor(250, 70); node2NotWorkingCurrentAveragePhasors[1] = phasor(250, -110);

    int* node2CurrentDestNodes = new int[2];
    if (node2CurrentDestNodes == NULL) {
        std::cout << "Error: TestKnnClass() failed to allocate memory for int* node2CurrentDestNodes!\n";
        if (node2NotWorkingCurrentAveragePhasors != NULL) {
            delete[] node2NotWorkingCurrentAveragePhasors;
            node2NotWorkingCurrentAveragePhasors = NULL;
        }
        if (node2WorkingCurrentAveragePhasors != NULL) {
            delete[] node2WorkingCurrentAveragePhasors;
            node2WorkingCurrentAveragePhasors = NULL;
        }
        if (node2Knowns != NULL) {
            for (int i = 0; i < numOfKnowns; i++) {
                if (node2Knowns[i] != NULL) { delete node2Knowns[i]; node2Knowns[i] = NULL; }
            }
            delete[] node2Knowns; node2Knowns = NULL;
        }
        if (node1Knowns != NULL) {
            for (int i = 0; i < numOfKnowns; i++) {
                if (node1Knowns[i] != NULL) { delete node1Knowns[i]; node1Knowns[i] = NULL; }
            }
            delete[] node1Knowns; node1Knowns = NULL;
        }
        return;
    }
    node2CurrentDestNodes[0] = 1; node2CurrentDestNodes[1] = 0;

    for (int i = 0; i < numOfKnowns; i++) {
        // Create node sample where the line is working
        if (i < numOfLineWorking) {
            phasor voltage = node2WorkingVoltageAveragePhasor * phasor(0.9 + 0.2 * (double)i / (double)numOfLineWorking, 0);
            phasor* currents = new phasor[2];
            if (currents == NULL) {
                std::cout << "Error: TestKnnClass() failed to allocate memory for phasor* currents!\n";
                if (node2CurrentDestNodes != NULL) { delete[] node2CurrentDestNodes; node2CurrentDestNodes = NULL; }
                if (node2NotWorkingCurrentAveragePhasors != NULL) {
                    delete[] node2NotWorkingCurrentAveragePhasors;
                    node2NotWorkingCurrentAveragePhasors = NULL;
                }
                if (node2WorkingCurrentAveragePhasors != NULL) {
                    delete[] node2WorkingCurrentAveragePhasors;
                    node2WorkingCurrentAveragePhasors = NULL;
                }
                if (node2Knowns != NULL) {
                    for (int j = 0; j < numOfKnowns; j++) {
                        if (node2Knowns[j] != NULL) { delete node2Knowns[j]; node2Knowns[j] = NULL; }
                    }
                    delete[] node2Knowns; node2Knowns = NULL;
                }
                if (node1Knowns != NULL) {
                    for (int j = 0; j < numOfKnowns; j++) {
                        if (node1Knowns[j] != NULL) { delete node1Knowns[j]; node1Knowns[j] = NULL; }
                    }
                    delete[] node1Knowns; node1Knowns = NULL;
                }
                return;
            }
            currents[0] = node2WorkingCurrentAveragePhasors[0] * phasor(0.9 + 0.2 * (double)i / (double)numOfLineWorking, 0);
            currents[1] = node2WorkingCurrentAveragePhasors[1] * phasor(0.9 + 0.2 * (double)i / (double)numOfLineWorking, 0);
            node2Knowns[i] = new nodeSample(node2Num, voltage, currents, node2CurrentDestNodes, 2);
            if (node2Knowns[i] == NULL) {
                std::cout << "Error: TestKnnClass() failed to allocate memory for nodeSample* node2Knowns[i]!\n";
                if (currents != NULL) { delete[] currents; currents = NULL; }
                if (node2CurrentDestNodes != NULL) { delete[] node2CurrentDestNodes; node2CurrentDestNodes = NULL; }
                if (node2NotWorkingCurrentAveragePhasors != NULL) {
                    delete[] node2NotWorkingCurrentAveragePhasors;
                    node2NotWorkingCurrentAveragePhasors = NULL;
                }
                if (node2WorkingCurrentAveragePhasors != NULL) {
                    delete[] node2WorkingCurrentAveragePhasors;
                    node2WorkingCurrentAveragePhasors = NULL;
                }
                if (node2Knowns != NULL) {
                    for (int j = 0; j < numOfKnowns; j++) {
                        if (node2Knowns[j] != NULL) { delete node2Knowns[j]; node2Knowns[j] = NULL; }
                    }
                    delete[] node2Knowns; node2Knowns = NULL;
                }
                if (node1Knowns != NULL) {
                    for (int j = 0; j < numOfKnowns; j++) {
                        if (node1Knowns[j] != NULL) { delete node1Knowns[j]; node1Knowns[j] = NULL; }
                    }
                    delete[] node1Knowns; node1Knowns = NULL;
                }
                return;
            }
            if (currents != NULL) { delete[] currents; currents = NULL; }
        }

        // Create node sample where the line is not working
        else {
            phasor voltage = node2NotWorkingVoltageAveragePhasor *
                phasor(0.9 + 0.2 * (double)(i - numOfLineWorking) / (double)numOfLineNotWorking, 0);
            phasor* currents = new phasor[2];
            if (currents == NULL) {
                std::cout << "Error: TestKnnClass() failed to allocate memory for phasor* currents!\n";
                if (node2CurrentDestNodes != NULL) { delete[] node2CurrentDestNodes; node2CurrentDestNodes = NULL; }
                if (node2NotWorkingCurrentAveragePhasors != NULL) {
                    delete[] node2NotWorkingCurrentAveragePhasors;
                    node2NotWorkingCurrentAveragePhasors = NULL;
                }
                if (node2WorkingCurrentAveragePhasors != NULL) {
                    delete[] node2WorkingCurrentAveragePhasors;
                    node2WorkingCurrentAveragePhasors = NULL;
                }
                if (node2Knowns != NULL) {
                    for (int j = 0; j < numOfKnowns; j++) {
                        if (node2Knowns[j] != NULL) { delete node2Knowns[j]; node2Knowns[j] = NULL; }
                    }
                    delete[] node2Knowns; node2Knowns = NULL;
                }
                if (node1Knowns != NULL) {
                    for (int j = 0; j < numOfKnowns; j++) {
                        if (node1Knowns[j] != NULL) { delete node1Knowns[j]; node1Knowns[j] = NULL; }
                    }
                    delete[] node1Knowns; node1Knowns = NULL;
                }
                return;
            }
            currents[0] = node2NotWorkingCurrentAveragePhasors[0] *
                phasor(0.9 + 0.2 * (double)(i - numOfLineWorking) / (double)numOfLineNotWorking, 0);
            currents[1] = node2NotWorkingCurrentAveragePhasors[1] *
                phasor(0.9 + 0.2 * (double)(i - numOfLineWorking) / (double)numOfLineNotWorking, 0);
            node2Knowns[i] = new nodeSample(node2Num, voltage, currents, node2CurrentDestNodes, 2);
            if (node2Knowns[i] == NULL) {
                std::cout << "Error: TestKnnClass() failed to allocate memory for nodeSample* node2Knowns[i]!\n";
                if (currents != NULL) { delete[] currents; currents = NULL; }
                if (node2CurrentDestNodes != NULL) { delete[] node2CurrentDestNodes; node2CurrentDestNodes = NULL; }
                if (node2NotWorkingCurrentAveragePhasors != NULL) {
                    delete[] node2NotWorkingCurrentAveragePhasors;
                    node2NotWorkingCurrentAveragePhasors = NULL;
                }
                if (node2WorkingCurrentAveragePhasors != NULL) {
                    delete[] node2WorkingCurrentAveragePhasors;
                    node2WorkingCurrentAveragePhasors = NULL;
                }
                if (node2Knowns != NULL) {
                    for (int j = 0; j < numOfKnowns; j++) {
                        if (node2Knowns[j] != NULL) { delete node2Knowns[j]; node2Knowns[j] = NULL; }
                    }
                    delete[] node2Knowns; node2Knowns = NULL;
                }
                if (node1Knowns != NULL) {
                    for (int j = 0; j < numOfKnowns; j++) {
                        if (node1Knowns[j] != NULL) { delete node1Knowns[j]; node1Knowns[j] = NULL; }
                    }
                    delete[] node1Knowns; node1Knowns = NULL;
                }
                return;
            }
            if (currents != NULL) { delete[] currents; currents = NULL; }
        }
    }


    // knowns
    lineSample** knowns = new lineSample*[numOfKnowns];
    if (knowns == NULL) {
        std::cout << "Error: TestKnnClass() failed to allocate memory for lineSample** knowns\n";
        if (node1Knowns != NULL) {
            for (int i = 0; i < numOfKnowns; i++) {
                if (node1Knowns[i] != NULL) { delete node1Knowns[i]; node1Knowns[i] = NULL; }
            }
            delete[] node1Knowns; node1Knowns = NULL;
        }
        if (node2Knowns != NULL) {
            for (int i = 0; i < numOfKnowns; i++) {
                if (node2Knowns[i] != NULL) { delete node2Knowns[i]; node2Knowns[i] = NULL; }
            }
            delete[] node2Knowns; node2Knowns = NULL;
        }
    }
    for (int i = 0; i < numOfKnowns; i++) knowns[i] = NULL;
    for (int i = 0; i < numOfKnowns; i++) {
        if (i < numOfLineWorking) {
            knowns[i] = new lineSample(node1Knowns[i], node2Knowns[i], true);
            if (knowns[i] == NULL) {
                std::cout << "Error: TestKnnClass() failed to allocate memory for lineSample* knowns[i]\n";
                for (int j = 0; j < numOfKnowns; j++) {
                    if (knowns[j] != NULL) { delete knowns[j]; knowns[j] = NULL; }
                }
                delete[] knowns; knowns = NULL;
                if (node1Knowns != NULL) {
                    for (int j = 0; j < numOfKnowns; j++) {
                        if (node1Knowns[j] != NULL) { delete node1Knowns[j]; node1Knowns[j] = NULL; }
                    }
                    delete[] node1Knowns; node1Knowns = NULL;
                }
                if (node2Knowns != NULL) {
                    for (int j = 0; j < numOfKnowns; j++) {
                        if (node2Knowns[j] != NULL) { delete node2Knowns[j]; node2Knowns[j] = NULL; }
                    }
                    delete[] node2Knowns; node2Knowns = NULL;
                }
            }
        }
        else {
            knowns[i] = new lineSample(node1Knowns[i], node2Knowns[i], false);
            if (knowns[i] == NULL) {
                std::cout << "Error: TestKnnClass() failed to allocate memory for lineSample* knowns[i]\n";
                for (int j = 0; j < numOfKnowns; j++) {
                    if (knowns[j] != NULL) { delete knowns[j]; knowns[j] = NULL; }
                }
                delete[] knowns; knowns = NULL;
                if (node1Knowns != NULL) {
                    for (int j = 0; j < numOfKnowns; j++) {
                        if (node1Knowns[j] != NULL) { delete node1Knowns[j]; node1Knowns[j] = NULL; }
                    }
                    delete[] node1Knowns; node1Knowns = NULL;
                }
                if (node2Knowns != NULL) {
                    for (int j = 0; j < numOfKnowns; j++) {
                        if (node2Knowns[j] != NULL) { delete node2Knowns[j]; node2Knowns[j] = NULL; }
                    }
                    delete[] node2Knowns; node2Knowns = NULL;
                }
            }
        }
    }


    // unknown
    nodeSample* node1Unknown = new nodeSample(node1Num, node1WorkingVoltageAveragePhasor, node1WorkingCurrentAveragePhasors,
        node1CurrentDestNodes, 2);
    nodeSample* node2Unknown = new nodeSample(node2Num, node2WorkingVoltageAveragePhasor, node2WorkingCurrentAveragePhasors,
        node2CurrentDestNodes, 2);
    lineSample* unknown = new lineSample(node1Unknown, node2Unknown, true);


    knn* testKNN = new knn(knowns, numOfKnowns, unknown, 3);
    testKNN->Print();


    // Free Memory
    if (node1Knowns != NULL) {
        for (int i = 0; i < numOfKnowns; i++) {
            if (node1Knowns[i] != NULL) { delete node1Knowns[i]; node1Knowns[i] = NULL; }
        }
        delete[] node1Knowns; node1Knowns = NULL;
    }
    if (node1CurrentDestNodes != NULL) { delete[] node1CurrentDestNodes; node1CurrentDestNodes = NULL; }
    if (node1NotWorkingCurrentAveragePhasors != NULL) {
        delete[] node1NotWorkingCurrentAveragePhasors;
        node1NotWorkingCurrentAveragePhasors = NULL;
    }
    if (node1WorkingCurrentAveragePhasors != NULL) {
        delete[] node1WorkingCurrentAveragePhasors;
        node1WorkingCurrentAveragePhasors = NULL;
    }
    if (node2Knowns != NULL) {
        for (int i = 0; i < numOfKnowns; i++) {
            if (node2Knowns[i] != NULL) { delete node2Knowns[i]; node2Knowns[i] = NULL; }
        }
        delete[] node2Knowns; node2Knowns = NULL;
    }
    if (node2CurrentDestNodes != NULL) { delete[] node2CurrentDestNodes; node2CurrentDestNodes = NULL; }
    if (node2NotWorkingCurrentAveragePhasors != NULL) {
        delete[] node2NotWorkingCurrentAveragePhasors;
        node2NotWorkingCurrentAveragePhasors = NULL;
    }
    if (node2WorkingCurrentAveragePhasors != NULL) {
        delete[] node2WorkingCurrentAveragePhasors;
        node2WorkingCurrentAveragePhasors = NULL;
    }
    if (knowns != NULL) {
        for (int i = 0; i < numOfKnowns; i++) {
            if (knowns[i] != NULL) { delete knowns[i]; knowns[i] = NULL; }
        }
        delete[] knowns; knowns = NULL;
    }
    if (node1Unknown != NULL) { delete node1Unknown; node1Unknown = NULL; }
    if (node2Unknown != NULL) { delete node2Unknown; node2Unknown = NULL; }
    if (unknown != NULL) { delete unknown; unknown = NULL; }
    if (testKNN != NULL) { delete testKNN; testKNN = NULL; }
}