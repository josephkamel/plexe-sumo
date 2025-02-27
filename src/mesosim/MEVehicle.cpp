/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.org/sumo
// Copyright (C) 2001-2018 German Aerospace Center (DLR) and others.
// This program and the accompanying materials
// are made available under the terms of the Eclipse Public License v2.0
// which accompanies this distribution, and is available at
// http://www.eclipse.org/legal/epl-v20.html
// SPDX-License-Identifier: EPL-2.0
/****************************************************************************/
/// @file    MEVehicle.cpp
/// @author  Daniel Krajzewicz
/// @date    Tue, May 2005
/// @version $Id$
///
// A vehicle from the mesoscopic point of view
/****************************************************************************/


// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#include <iostream>
#include <cassert>
#include <utils/common/StdDefs.h>
#include <utils/common/FileHelpers.h>
#include <utils/common/MsgHandler.h>
#include <utils/iodevices/BinaryInputDevice.h>
#include <utils/iodevices/OutputDevice.h>
#include <utils/xml/SUMOSAXAttributes.h>
#include <microsim/MSGlobals.h>
#include <microsim/MSEdge.h>
#include <microsim/MSLane.h>
#include <microsim/MSNet.h>
#include <microsim/MSVehicleType.h>
#include <microsim/MSLink.h>
#include <microsim/devices/MSDevice.h>
#include "MELoop.h"
#include "MEVehicle.h"
#include "MESegment.h"


// ===========================================================================
// method definitions
// ===========================================================================
MEVehicle::MEVehicle(SUMOVehicleParameter* pars, const MSRoute* route,
                     MSVehicleType* type, const double speedFactor) :
    MSBaseVehicle(pars, route, type, speedFactor),
    mySegment(0),
    myQueIndex(0),
    myEventTime(SUMOTime_MIN),
    myLastEntryTime(SUMOTime_MIN),
    myBlockTime(SUMOTime_MAX) {
    if (!(*myCurrEdge)->isTazConnector()) {
        if ((*myCurrEdge)->allowedLanes(type->getVehicleClass()) == 0) {
            throw ProcessError("Vehicle '" + pars->id + "' is not allowed to depart on any lane of its first edge.");
        }
        if (pars->departSpeedProcedure == DEPART_SPEED_GIVEN && pars->departSpeed > type->getMaxSpeed()) {
            throw ProcessError("Departure speed for vehicle '" + pars->id +
                               "' is too high for the vehicle type '" + type->getID() + "'.");
        }
    }
}


double
MEVehicle::getBackPositionOnLane(const MSLane* /* lane */) const {
    return getPositionOnLane() - getVehicleType().getLength();
}


double
MEVehicle::getPositionOnLane() const {
// the following interpolation causes problems with arrivals and calibrators
//    const double fracOnSegment = MIN2(double(1), STEPS2TIME(MSNet::getInstance()->getCurrentTimeStep() - myLastEntryTime) / STEPS2TIME(myEventTime - myLastEntryTime));
    return mySegment == 0 ? 0 : (double(mySegment->getIndex()) /* + fracOnSegment */) * mySegment->getLength();
}


double
MEVehicle::getAngle() const {
    const MSLane* const lane = getEdge()->getLanes()[0];
    return lane->getShape().rotationAtOffset(lane->interpolateLanePosToGeometryPos(getPositionOnLane()));
}


double
MEVehicle::getSlope() const {
    const MSLane* const lane = getEdge()->getLanes()[0];
    return lane->getShape().slopeDegreeAtOffset(lane->interpolateLanePosToGeometryPos(getPositionOnLane()));
}


Position
MEVehicle::getPosition(const double offset) const {
    const MSLane* const lane = getEdge()->getLanes()[0];
    return lane->geometryPositionAtOffset(getPositionOnLane() + offset);
}


double
MEVehicle::getSpeed() const {
    if (getWaitingTime() > 0) {
        return 0;
    } else {
        return getAverageSpeed();
    }
}


double
MEVehicle::getAverageSpeed() const {
    return mySegment->getLength() / STEPS2TIME(myEventTime - myLastEntryTime);
}


double
MEVehicle::estimateLeaveSpeed(const MSLink* link) const {
    /// @see MSVehicle.cpp::estimateLeaveSpeed
    const double v = getSpeed();
    return MIN2(link->getViaLaneOrLane()->getVehicleMaxSpeed(this),
                (double)sqrt(2 * link->getLength() * getVehicleType().getCarFollowModel().getMaxAccel() + v * v));
}


double
MEVehicle::getConservativeSpeed(SUMOTime& earliestArrival) const {
    earliestArrival = MAX2(myEventTime, earliestArrival - DELTA_T); // event times have subsecond resolution
    return mySegment->getLength() / STEPS2TIME(earliestArrival - myLastEntryTime);
}


bool
MEVehicle::moveRoutePointer() {
    // vehicle has just entered a new edge. Position is 0
    if (myCurrEdge == myRoute->end() - 1) { // may happen during teleport
        return true;
    }
    ++myCurrEdge;
    if ((*myCurrEdge)->isVaporizing()) {
        return true;
    }
    // update via
    if (myParameter->via.size() > 0 && (*myCurrEdge)->getID() == myParameter->via.front()) {
        myParameter->via.erase(myParameter->via.begin());
    }
    return hasArrived();
}


bool
MEVehicle::hasArrived() const {
    // mySegment may be 0 due to teleporting or arrival
    return myCurrEdge == myRoute->end() - 1 && (
               (mySegment == 0)
               || myEventTime == SUMOTime_MIN
               || getPositionOnLane() > myArrivalPos - POSITION_EPS);
}

bool
MEVehicle::isOnRoad() const {
    return getSegment() != 0;
}


bool
MEVehicle::isParking() const {
    return false; // parking attribute of a stop is not yet evaluated /implemented
}


bool
MEVehicle::replaceRoute(const MSRoute* newRoute, bool onInit, int offset, bool addStops, bool removeStops) {
    UNUSED_PARAMETER(addStops); // @todo recheck!
    UNUSED_PARAMETER(removeStops); // @todo recheck!
    const ConstMSEdgeVector& edges = newRoute->getEdges();
    // assert the vehicle may continue (must not be "teleported" or whatever to another position)
    if (!onInit && !newRoute->contains(*myCurrEdge)) {
        return false;
    }
    MSLink* oldLink = 0;
    MSLink* newLink = 0;
    if (mySegment != 0) {
        oldLink = mySegment->getLink(this);
    }
    // rebuild in-vehicle route information
    if (onInit) {
        myCurrEdge = newRoute->begin();
    } else {
        myCurrEdge = find(edges.begin() + offset, edges.end(), *myCurrEdge);
    }
    // check whether the old route may be deleted (is not used by anyone else)
    newRoute->addReference();
    myRoute->release();
    // assign new route
    myRoute = newRoute;
    if (mySegment != 0) {
        newLink = mySegment->getLink(this);
    }
    // update approaching vehicle information
    if (oldLink != 0 && oldLink != newLink) {
        oldLink->removeApproaching(this);
        MELoop::setApproaching(this, newLink);
    }
    // update arrival definition
    calculateArrivalParams();
    // save information that the vehicle was rerouted
    myNumberReroutes++;
    MSNet::getInstance()->informVehicleStateListener(this, MSNet::VEHICLE_STATE_NEWROUTE);
    calculateArrivalParams();
    return true;
}


bool
MEVehicle::addStop(const SUMOVehicleParameter::Stop& stopPar, std::string& /*errorMsg*/, SUMOTime untilOffset, bool /*collision*/,
                   MSRouteIterator* /* searchStart */) {
    const MSEdge* const edge = MSEdge::dictionary(stopPar.lane.substr(0, stopPar.lane.rfind('_')));
    assert(edge != 0);
    MESegment* stopSeg = MSGlobals::gMesoNet->getSegmentForEdge(*edge, stopPar.endPos);
    myStops[stopSeg].push_back(stopPar);
    myStops[stopSeg].back().until += untilOffset;
    myStopEdges.push_back(edge);
    return true;
}


bool
MEVehicle::isStopped() const {
    return myStops.find(mySegment) != myStops.end();
}


bool
MEVehicle::isStoppedTriggered() const {
    return false;
}


bool
MEVehicle::isStoppedInRange(double pos) const {
    UNUSED_PARAMETER(pos);
    return isStopped();
}


SUMOTime
MEVehicle::getStoptime(const MESegment* const seg, SUMOTime time) const {
    if (myStops.find(seg) != myStops.end()) {
        for (const SUMOVehicleParameter::Stop& stop : myStops.find(seg)->second) {
            time += stop.duration;
            if (stop.until > time) {
                // @note: this assumes the stop is reached at time. With the way this is called in MESegment (time == entryTime),
                // travel time is overestimated of the stop is not at the start of the segment
                time = stop.until;
            }
        }
    }
    return time;
}


double 
MEVehicle::getCurrentStoppingTimeSeconds() const {
    return STEPS2TIME(getStoptime(mySegment, myLastEntryTime) - myLastEntryTime);
}


const ConstMSEdgeVector
MEVehicle::getStopEdges() const {
// TODO: myStopEdges still needs to be updated when leaving a stop
    return myStopEdges;
}


bool
MEVehicle::mayProceed() const {
    return mySegment == 0 || mySegment->isOpen(this);
}


double
MEVehicle::getCurrentLinkPenaltySeconds() const {
    if (mySegment == 0) {
        return 0;
    } else {
        return STEPS2TIME(mySegment->getLinkPenalty(this));
    }
}


void
MEVehicle::updateDetectorForWriting(MSMoveReminder* rem, SUMOTime currentTime, SUMOTime exitTime) {
    for (MoveReminderCont::iterator i = myMoveReminders.begin(); i != myMoveReminders.end(); ++i) {
        if (i->first == rem) {
            rem->updateDetector(*this, mySegment->getIndex() * mySegment->getLength(),
                                (mySegment->getIndex() + 1) * mySegment->getLength(),
                                getLastEntryTime(), currentTime, exitTime, false);
#ifdef _DEBUG
            if (myTraceMoveReminders) {
                traceMoveReminder("notifyMove", i->first, i->second, true);
            }
#endif
            return;
        }
    }
}


void
MEVehicle::updateDetectors(SUMOTime currentTime, const bool isLeave, const MSMoveReminder::Notification reason) {
    // segments of the same edge have the same reminder so no cleaning up must take place
    const bool cleanUp = isLeave && (reason != MSMoveReminder::NOTIFICATION_SEGMENT);
    for (MoveReminderCont::iterator rem = myMoveReminders.begin(); rem != myMoveReminders.end();) {
        if (currentTime != getLastEntryTime()) {
            rem->first->updateDetector(*this, mySegment->getIndex() * mySegment->getLength(),
                                       (mySegment->getIndex() + 1) * mySegment->getLength(),
                                       getLastEntryTime(), currentTime, getEventTime(), cleanUp);
#ifdef _DEBUG
            if (myTraceMoveReminders) {
                traceMoveReminder("notifyMove", rem->first, rem->second, true);
            }
#endif
        }
        if (!isLeave || rem->first->notifyLeave(*this, mySegment->getLength(), reason)) {
#ifdef _DEBUG
            if (isLeave && myTraceMoveReminders) {
                traceMoveReminder("notifyLeave", rem->first, rem->second, true);
            }
#endif
            ++rem;
        } else {
#ifdef _DEBUG
            if (myTraceMoveReminders) {
                traceMoveReminder("remove", rem->first, rem->second, false);
            }
#endif
            rem = myMoveReminders.erase(rem);
        }
    }
}


void
MEVehicle::saveState(OutputDevice& out) {
    MSBaseVehicle::saveState(out);
    assert(mySegment == 0 || *myCurrEdge == &mySegment->getEdge());
    std::vector<SUMOTime> internals;
    internals.push_back(myDeparture);
    internals.push_back((SUMOTime)distance(myRoute->begin(), myCurrEdge));
    internals.push_back(mySegment == 0 ? (SUMOTime) - 1 : (SUMOTime)mySegment->getIndex());
    internals.push_back((SUMOTime)getQueIndex());
    internals.push_back(myEventTime);
    internals.push_back(myLastEntryTime);
    internals.push_back(myBlockTime);
    out.writeAttr(SUMO_ATTR_STATE, toString(internals));
    // save stops and parameters
    for (const auto& it : myStops) {
        for (const SUMOVehicleParameter::Stop& stop : it.second) {
            stop.write(out);
        }
    }
    myParameter->writeParams(out);
    for (std::vector<MSDevice*>::const_iterator dev = myDevices.begin(); dev != myDevices.end(); ++dev) {
        (*dev)->saveState(out);
    }
    out.closeTag();
}


void
MEVehicle::loadState(const SUMOSAXAttributes& attrs, const SUMOTime offset) {
    if (attrs.hasAttribute(SUMO_ATTR_POSITION)) {
        throw ProcessError("Error: Invalid vehicles in state (may be a micro state)!");
    }
    int routeOffset;
    int segIndex;
    int queIndex;
    std::istringstream bis(attrs.getString(SUMO_ATTR_STATE));
    bis >> myDeparture;
    bis >> routeOffset;
    bis >> segIndex;
    bis >> queIndex;
    bis >> myEventTime;
    bis >> myLastEntryTime;
    bis >> myBlockTime;
    if (hasDeparted()) {
        myDeparture -= offset;
        myEventTime -= offset;
        myLastEntryTime -= offset;
        myCurrEdge += routeOffset;
        if (segIndex >= 0) {
            MESegment* seg = MSGlobals::gMesoNet->getSegmentForEdge(**myCurrEdge);
            while (seg->getIndex() != (int)segIndex) {
                seg = seg->getNextSegment();
                assert(seg != 0);
            }
            setSegment(seg, queIndex);
        } else {
            // on teleport
            setSegment(0, 0);
            assert(myEventTime != SUMOTime_MIN);
            MSGlobals::gMesoNet->addLeaderCar(this, 0);
        }
    }
    if (myBlockTime != SUMOTime_MAX) {
        myBlockTime -= offset;
    }
}


/****************************************************************************/

