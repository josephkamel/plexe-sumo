/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.org/sumo
// Copyright (C) 2001-2018 German Aerospace Center (DLR) and others.
// This program and the accompanying materials
// are made available under the terms of the Eclipse Public License v2.0
// which accompanies this distribution, and is available at
// http://www.eclipse.org/legal/epl-v20.html
// SPDX-License-Identifier: EPL-2.0
/****************************************************************************/
/// @file    RealisticSensor.cpp
/// @author  Marco Iorio
/// @date    Fri, 27 Sep 2019
/// @version $Id$
///
/****************************************************************************/

#include "RealisticSensor.h"

#include <algorithm>
#include <cmath>
#include <limits>


RealisticSensor::RealisticSensor() :
        RealisticSensor(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max(),
                        std::numeric_limits<double>::digits10, 0, 0, 0, true, 0) {
}

RealisticSensor::RealisticSensor(double minValue, double maxValue, int decimalDigits, double updateInterval,
                                 double absoluteError, double percentageError, bool sumErrors, int seed) :
        minValue(minValue), maxValue(maxValue), decimalDigits(decimalDigits), updateInterval(updateInterval),
        absoluteError(absoluteError), percentageError(percentageError), sumErrors(sumErrors),
        generator(seed), cachedTime(std::numeric_limits<double>::lowest()), cachedValue(0) {
}

bool RealisticSensor::isValueOutOfRange(double value) const {
    return value < minValue || value > maxValue;
}

double RealisticSensor::getReading(double value, double currentTime) {

    // Return the cached value if the updateInterval has not yet passed
    if (currentTime < cachedTime + updateInterval) {
        return cachedValue;
    }

    // Compute the error interval associated to the sensor
    double relativeError = value * (percentageError / 100);
    double error = sumErrors ? (absoluteError + relativeError) : std::max(absoluteError, relativeError);

    // Draw a random measurement from the computed interval
    std::uniform_real_distribution<> extractMeasurement(value - error, value + error);
    double measurement = error > 0 ? extractMeasurement(generator) : value;

    if (measurement < minValue) {
        return minValue;
    }
    if (measurement > maxValue) {
        return maxValue;
    }

    cachedTime = currentTime;

    // Round the measurement depending on the specified precision
    double factor = std::pow(10, decimalDigits);
    cachedValue = std::round(measurement * factor) / factor;

    return cachedValue;
}

double RealisticSensor::getReading(double value, double currentTime, double& samplingTime) {
    // Update the cached value if necessary
    getReading(value, currentTime);

    samplingTime = cachedTime;
    return cachedValue;
}

void RealisticSensor::setSeed(int seed) {
    generator = std::mt19937(seed);
}