#pragma once

/// <summary>
/// An instantaneous measurement of a voltage or current at an exact time
/// </summary>
class instantaneousMeasurement {
public:
    /// <summary>
    /// The time in seconds
    /// </summary>
    double timeStamp;
    /// <summary>
    /// The instantaneous measurement in base units
    /// </summary>
    double value;
};

