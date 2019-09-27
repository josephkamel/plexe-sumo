/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.org/sumo
// Copyright (C) 2001-2018 German Aerospace Center (DLR) and others.
// This program and the accompanying materials
// are made available under the terms of the Eclipse Public License v2.0
// which accompanies this distribution, and is available at
// http://www.eclipse.org/legal/epl-v20.html
// SPDX-License-Identifier: EPL-2.0
/****************************************************************************/
/// @file    RealisticSensor.h
/// @author  Marco Iorio
/// @date    Fri, 27 Sep 2019
/// @version $Id$
///
/****************************************************************************/

#ifndef REALISTICSENSOR_H_
#define REALISTICSENSOR_H_

#include <random>

/// @brief A class used to represent a realistic sensor
class RealisticSensor {
public:
    RealisticSensor();

    RealisticSensor(double minValue, double maxValue, int decimalDigits, double updateInterval,
                    double absoluteError, double percentageError, bool sumErrors, int seed);

    ~RealisticSensor() = default;

    bool isValueOutOfRange(double value) const;
    double getReading(double value, double currentTime);
    double getReading(double value, double currentTime, double& samplingTime);

    void setSeed(int seed);

public:
    /// @brief the minimum value that can be measured by the sensor
    double minValue;
    /// @brief the maximum value that can be measured by the sensor
    double maxValue;
    /// @brief the number of decimal digits of the measurement provided by the sensor
    int decimalDigits;
    /// @brief the amount of time elapsed between two measurements
    double updateInterval;

    /// @brief the absolute error associated to the sensor
    double absoluteError;
    /// @brief the relative error associated to the sensor (in percentage)
    double percentageError;
    /// @brief whether absolute and relative errors are summed or only the highest one is applied
    bool sumErrors;

protected:
    std::mt19937 generator;

    double cachedTime;
    double cachedValue;
};


#endif /* REALISTICSENSOR_H_ */
