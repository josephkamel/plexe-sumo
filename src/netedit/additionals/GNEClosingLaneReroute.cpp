/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.org/sumo
// Copyright (C) 2001-2018 German Aerospace Center (DLR) and others.
// This program and the accompanying materials
// are made available under the terms of the Eclipse Public License v2.0
// which accompanies this distribution, and is available at
// http://www.eclipse.org/legal/epl-v20.html
// SPDX-License-Identifier: EPL-2.0
/****************************************************************************/
/// @file    GNEClosingLaneReroute.cpp
/// @author  Pablo Alvarez Lopez
/// @date    Jan 2017
/// @version $Id$
///
//
/****************************************************************************/

// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#include <utils/common/ToString.h>
#include "GNEClosingLaneReroute.h"
#include <netedit/netelements/GNELane.h>
#include <netedit/netelements/GNEEdge.h>
#include <netedit/changes/GNEChange_Attribute.h>
#include <netedit/dialogs/GNERerouterIntervalDialog.h>

#include <netedit/GNEUndoList.h>
#include "GNERerouter.h"
#include <netedit/GNEViewNet.h>
#include <netedit/GNENet.h>
#include "GNERerouterInterval.h"


// ===========================================================================
// member method definitions
// ===========================================================================

GNEClosingLaneReroute::GNEClosingLaneReroute(GNERerouterIntervalDialog* rerouterIntervalDialog) :
    GNEAttributeCarrier(SUMO_TAG_CLOSING_LANE_REROUTE, ICON_EMPTY),
    myRerouterIntervalParent(rerouterIntervalDialog->getEditedRerouterInterval()),
    myClosedLane(rerouterIntervalDialog->getEditedRerouterInterval()->getRerouterParent()->getEdgeChilds().at(0)->getLanes().at(0)),
    myAllowedVehicles(parseVehicleClasses(getDefaultValue<std::string>(SUMO_TAG_CLOSING_LANE_REROUTE, SUMO_ATTR_ALLOW))),
    myDisallowedVehicles(parseVehicleClasses(getDefaultValue<std::string>(SUMO_TAG_CLOSING_LANE_REROUTE, SUMO_ATTR_DISALLOW))) {
}


GNEClosingLaneReroute::GNEClosingLaneReroute(GNERerouterInterval* rerouterIntervalParent, GNELane* closedLane, SVCPermissions allowedVehicles, SVCPermissions disallowedVehicles) :
    GNEAttributeCarrier(SUMO_TAG_CLOSING_LANE_REROUTE, ICON_EMPTY),
    myRerouterIntervalParent(rerouterIntervalParent),
    myClosedLane(closedLane),
    myAllowedVehicles(allowedVehicles),
    myDisallowedVehicles(disallowedVehicles) {
}


GNEClosingLaneReroute::~GNEClosingLaneReroute() {}


void
GNEClosingLaneReroute::writeClosingLaneReroute(OutputDevice& device) const {
    // open closing reroute tag
    device.openTag(getTag());
    // write Lane ID
    writeAttribute(device, SUMO_ATTR_ID);
    // write Allowed vehicles
    writeAttribute(device, SUMO_ATTR_ALLOW);
    // write disallowed vehicles
    writeAttribute(device, SUMO_ATTR_DISALLOW);
    // close closing reroute tag
    device.closeTag();
}


GNERerouterInterval*
GNEClosingLaneReroute::getRerouterIntervalParent() const {
    return myRerouterIntervalParent;
}


void 
GNEClosingLaneReroute::selectAttributeCarrier(bool) {
    // this AC cannot be selected
}


void 
GNEClosingLaneReroute::unselectAttributeCarrier(bool) {
    // this AC cannot be unselected
}


bool 
GNEClosingLaneReroute::isAttributeCarrierSelected() const {
    // this AC doesn't own a select flag
    return false;
}


std::string
GNEClosingLaneReroute::getAttribute(SumoXMLAttr key) const {
    switch (key) {
        case SUMO_ATTR_ID:
            return myClosedLane->getID();
        case SUMO_ATTR_ALLOW:
            return getVehicleClassNames(myAllowedVehicles);
        case SUMO_ATTR_DISALLOW:
            return getVehicleClassNames(myDisallowedVehicles);
        default:
            throw InvalidArgument(toString(getTag()) + " doesn't have an attribute of type '" + toString(key) + "'");
    }
}


void
GNEClosingLaneReroute::setAttribute(SumoXMLAttr key, const std::string& value, GNEUndoList* undoList) {
    if (value == getAttribute(key)) {
        return; //avoid needless changes, later logic relies on the fact that attributes have changed
    }
    switch (key) {
        case SUMO_ATTR_ID:
        case SUMO_ATTR_ALLOW:
        case SUMO_ATTR_DISALLOW:
            undoList->p_add(new GNEChange_Attribute(this, key, value));
            break;
        default:
            throw InvalidArgument(toString(getTag()) + " doesn't have an attribute of type '" + toString(key) + "'");
    }
}


bool
GNEClosingLaneReroute::isValid(SumoXMLAttr key, const std::string& value) {
    switch (key) {
        case SUMO_ATTR_ID:
            return (myRerouterIntervalParent->getRerouterParent()->getViewNet()->getNet()->retrieveLane(value, false) != nullptr);
        case SUMO_ATTR_ALLOW:
            return canParseVehicleClasses(value);
        case SUMO_ATTR_DISALLOW:
            return canParseVehicleClasses(value);
        default:
            throw InvalidArgument(toString(getTag()) + " doesn't have an attribute of type '" + toString(key) + "'");
    }
}

// ===========================================================================
// private
// ===========================================================================

void
GNEClosingLaneReroute::setAttribute(SumoXMLAttr key, const std::string& value) {
    switch (key) {
        case SUMO_ATTR_ID: {
            myClosedLane = myRerouterIntervalParent->getRerouterParent()->getViewNet()->getNet()->retrieveLane(value);
            break;
        }
        case SUMO_ATTR_ALLOW: {
            myAllowedVehicles = parseVehicleClasses(value);
            break;
        }
        case SUMO_ATTR_DISALLOW: {
            myAllowedVehicles = parseVehicleClasses(value);
            break;
        }
        default:
            throw InvalidArgument(toString(getTag()) + " doesn't have an attribute of type '" + toString(key) + "'");
    }
}

/****************************************************************************/
