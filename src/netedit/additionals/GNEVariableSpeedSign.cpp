/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.org/sumo
// Copyright (C) 2001-2018 German Aerospace Center (DLR) and others.
// This program and the accompanying materials
// are made available under the terms of the Eclipse Public License v2.0
// which accompanies this distribution, and is available at
// http://www.eclipse.org/legal/epl-v20.html
// SPDX-License-Identifier: EPL-2.0
/****************************************************************************/
/// @file    GNEVariableSpeedSign.cpp
/// @author  Pablo Alvarez Lopez
/// @date    Nov 2015
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

#include <string>
#include <iostream>
#include <utility>
#include <utils/geom/GeomConvHelper.h>
#include <utils/geom/PositionVector.h>
#include <utils/common/RandHelper.h>
#include <utils/common/SUMOVehicleClass.h>
#include <utils/common/ToString.h>
#include <utils/geom/GeomHelper.h>
#include <utils/gui/windows/GUISUMOAbstractView.h>
#include <utils/gui/windows/GUIAppEnum.h>
#include <utils/gui/images/GUITextureSubSys.h>
#include <utils/gui/globjects/GUIGLObjectPopupMenu.h>
#include <utils/gui/div/GLHelper.h>
#include <utils/gui/windows/GUIAppEnum.h>
#include <utils/gui/images/GUITexturesHelper.h>
#include <utils/xml/SUMOSAXHandler.h>
#include <netedit/changes/GNEChange_Attribute.h>
#include <netedit/dialogs/GNEVariableSpeedSignDialog.h>
#include <netedit/netelements/GNELane.h>
#include <netedit/GNEViewParent.h>
#include <netedit/GNEViewNet.h>
#include <netedit/GNEUndoList.h>
#include <netedit/GNENet.h>

#include "GNEVariableSpeedSign.h"
#include "GNEVariableSpeedSignStep.h"


// ===========================================================================
// member method definitions
// ===========================================================================

GNEVariableSpeedSign::GNEVariableSpeedSign(const std::string& id, GNEViewNet* viewNet, const Position &pos, const std::vector<GNELane*> &lanes, const std::string& filename, bool blockMovement) :
    GNEAdditional(id, viewNet, GLO_VSS, SUMO_TAG_VSS, ICON_VARIABLESPEEDSIGN, true, blockMovement, lanes),
    myPosition(pos),
    myFilename(filename),
    mySaveInFilename(false) {
}


GNEVariableSpeedSign::~GNEVariableSpeedSign() {
}


void
GNEVariableSpeedSign::updateGeometry() {
    // Clear shape
    myShape.clear();

    // Set block icon position
    myBlockIconPosition = myPosition;

    // Set block icon offset
    myBlockIconOffset = Position(-0.5, -0.5);

    // Set block icon rotation, and using their rotation for draw logo
    setBlockIconRotation();

    // Set position
    myShape.push_back(myPosition);

    // clear mySymbolsPositionAndRotation
    mySymbolsPositionAndRotation.clear();

    // update child connections
    updateChildConnections();


    // Refresh element (neccesary to avoid grabbing problems)
    myViewNet->getNet()->refreshElement(this);
}


Position
GNEVariableSpeedSign::getPositionInView() const {
    return myPosition;
}


void
GNEVariableSpeedSign::openAdditionalDialog() {
    // Open VSS dialog
    GNEVariableSpeedSignDialog(this);
}


void
GNEVariableSpeedSign::moveGeometry(const Position& oldPos, const Position& offset) {
    // restore old position, apply offset and update Geometry
    myPosition = oldPos;
    myPosition.add(offset);
    updateGeometry();
}


void
GNEVariableSpeedSign::commitGeometryMoving(const Position& oldPos, GNEUndoList* undoList) {
    undoList->p_begin("position of " + toString(getTag()));
    undoList->p_add(new GNEChange_Attribute(this, SUMO_ATTR_POSITION, toString(myPosition), true, toString(oldPos)));
    undoList->p_end();
}


void
GNEVariableSpeedSign::writeAdditional(OutputDevice& device) const {
    // Write parameters
    device.openTag(getTag());
    writeAttribute(device, SUMO_ATTR_ID);
    writeAttribute(device, SUMO_ATTR_LANES);
    writeAttribute(device, SUMO_ATTR_POSITION);
    // If filename isn't empty and save in filename is enabled, save in a different file. In other case, save in the same additional XML
    if (!myFilename.empty() && mySaveInFilename) {
        // Write filename attribute
        writeAttribute(device, SUMO_ATTR_FILE);
        // Save values in a different file
        OutputDevice& deviceVSS = OutputDevice::getDevice(/**currentDirectory +**/ myFilename);
        deviceVSS.openTag("VSS");
        // write steps
        for (auto i : mySteps) {
            i->writeStep(device);
        }
        deviceVSS.close();
    } else {
        // write steps
        for (auto i : mySteps) {
            i->writeStep(device);
        }
    }
    // write block movement attribute only if it's enabled
    if (myBlockMovement) {
        writeAttribute(device, GNE_ATTR_BLOCK_MOVEMENT);
    }
    // Close tag
    device.closeTag();
}


void
GNEVariableSpeedSign::addVariableSpeedSignStep(GNEVariableSpeedSignStep* step) {
    auto it = std::find(mySteps.begin(), mySteps.end(), step);
    if (it == mySteps.end()) {
        mySteps.push_back(step);
        // sort steps always after a adding/restoring
        sortVariableSpeedSignSteps();
    } else {
        throw ProcessError("Variable Speed Sign Step already exist");
    }
}


void
GNEVariableSpeedSign::removeVariableSpeedSignStep(GNEVariableSpeedSignStep* step) {
    auto it = std::find(mySteps.begin(), mySteps.end(), step);
    if (it != mySteps.end()) {
        mySteps.erase(it);
        // sort steps always after a adding/restoring
        sortVariableSpeedSignSteps();
    } else {
        throw ProcessError("Variable Speed Sign Step doesn't exist");
    }
}


const std::vector<GNEVariableSpeedSignStep*>&
GNEVariableSpeedSign::getVariableSpeedSignSteps() const {
    return mySteps;
}


void
GNEVariableSpeedSign::sortVariableSpeedSignSteps() {
    // declare a vector to keep sorted steps
    std::vector<GNEVariableSpeedSignStep*> sortedSteps;
    // sort intervals usin time as criterium
    while (mySteps.size() > 0) {
        int time_small = 0;
        // find the interval with the small begin
        for (int i = 0; i < (int)mySteps.size(); i++) {
            if (mySteps.at(i)->getTime() < mySteps.at(time_small)->getTime()) {
                time_small = i;
            }
        }
        // add it to sorted steps and remove it from mySteps
        sortedSteps.push_back(mySteps.at(time_small));
        mySteps.erase(mySteps.begin() + time_small);
    }
    // restore mySteps using sorted steps
    mySteps = sortedSteps;
}


const std::string&
GNEVariableSpeedSign::getParentName() const {
    return myViewNet->getNet()->getMicrosimID();
}


void
GNEVariableSpeedSign::drawGL(const GUIVisualizationSettings& s) const {
    // Start drawing adding an gl identificator
    glPushName(getGlID());

    // Add a draw matrix for drawing logo
    glPushMatrix();
    glTranslated(myShape[0].x(), myShape[0].y(), getType());

    // Draw icon depending of variable speed sign is or if isn't being drawn for selecting
    if(s.drawForSelecting) {
        GLHelper::setColor(RGBColor::WHITE);
        GLHelper::drawBoxLine(Position(0, 1), 0, 2, 1);
    } else {
        glColor3d(1, 1, 1);
        glRotated(180, 0, 0, 1);
        if (isAttributeCarrierSelected()) {
            GUITexturesHelper::drawTexturedBox(GUITextureSubSys::getTexture(GNETEXTURE_VARIABLESPEEDSIGNSELECTED), 1);
        } else {
            GUITexturesHelper::drawTexturedBox(GUITextureSubSys::getTexture(GNETEXTURE_VARIABLESPEEDSIGN), 1);
        }
    }

    // Pop draw icon matrix
    glPopMatrix();

    // Only lock and childs if isn't being drawn for selecting
    if(!s.drawForSelecting) {

        // Show Lock icon depending of the Edit mode
        drawLockIcon(0.4);

        // obtain exxageration
        const double exaggeration = s.addSize.getExaggeration(s);

        // iterate over symbols and rotation
        for (auto i : mySymbolsPositionAndRotation) {
            glPushMatrix();
            glScaled(exaggeration, exaggeration, 1);
            glTranslated(i.first.x(), i.first.y(), getType());
            glRotated(-1 * i.second, 0, 0, 1);
            glTranslated(0, -1.5, 0);

            int noPoints = 9;
            if (s.scale > 25) {
                noPoints = (int)(9.0 + s.scale / 10.0);
                if (noPoints > 36) {
                    noPoints = 36;
                }
            }
            glColor3d(1, 0, 0);
            GLHelper::drawFilledCircle((double) 1.3, noPoints);
            if (s.scale >= 5) {
                glTranslated(0, 0, .1);
                glColor3d(0, 0, 0);
                GLHelper::drawFilledCircle((double) 1.1, noPoints);
                // draw the speed string
                //draw
                glColor3d(1, 1, 0);
                glTranslated(0, 0, .1);
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

                // draw last value string
                GLHelper::drawText("S", Position(0, 0), .1, 1.2, RGBColor(255, 255, 0), 180);
            }
            glPopMatrix();
        }

        // Draw connections
        drawChildConnections();
    }

    // Pop symbol matrix
    glPopMatrix();

    // Draw name if isn't being drawn for selecting
    if(!s.drawForSelecting) {
        drawName(getCenteringBoundary().getCenter(), s.scale, s.addName);
    }

    // Pop name
    glPopName();
}


std::string
GNEVariableSpeedSign::getAttribute(SumoXMLAttr key) const {
    switch (key) {
        case SUMO_ATTR_ID:
            return getAdditionalID();
        case SUMO_ATTR_LANES:
            return parseGNELanes(myLaneChilds);
        case SUMO_ATTR_POSITION:
            return toString(myPosition);
        case SUMO_ATTR_FILE:
            return myFilename;
        case GNE_ATTR_BLOCK_MOVEMENT:
            return toString(myBlockMovement);
        case GNE_ATTR_SELECTED:
            return toString(isAttributeCarrierSelected());
        default:
            throw InvalidArgument(toString(getTag()) + " doesn't have an attribute of type '" + toString(key) + "'");
    }
}


void
GNEVariableSpeedSign::setAttribute(SumoXMLAttr key, const std::string& value, GNEUndoList* undoList) {
    if (value == getAttribute(key)) {
        return; //avoid needless changes, later logic relies on the fact that attributes have changed
    }
    switch (key) {
        case SUMO_ATTR_ID:
        case SUMO_ATTR_LANES:
        case SUMO_ATTR_POSITION:
        case SUMO_ATTR_FILE:
        case GNE_ATTR_BLOCK_MOVEMENT:
        case GNE_ATTR_SELECTED:
            undoList->p_add(new GNEChange_Attribute(this, key, value));
            break;
        default:
            throw InvalidArgument(toString(getTag()) + " doesn't have an attribute of type '" + toString(key) + "'");
    }
}


bool
GNEVariableSpeedSign::isValid(SumoXMLAttr key, const std::string& value) {
    switch (key) {
        case SUMO_ATTR_ID:
            return isValidAdditionalID(value);
        case SUMO_ATTR_POSITION:
            return canParse<Position>(value);
        case SUMO_ATTR_LANES:
            if (value.empty()) {
                return false;
            } else {
                return checkGNELanesValid(myViewNet->getNet(), value, false);
            }
        case SUMO_ATTR_FILE:
            return isValidFilename(value);
        case GNE_ATTR_BLOCK_MOVEMENT:
            return canParse<bool>(value);
        case GNE_ATTR_SELECTED:
            return canParse<bool>(value);
        default:
            throw InvalidArgument(toString(getTag()) + " doesn't have an attribute of type '" + toString(key) + "'");
    }
}


void
GNEVariableSpeedSign::setAttribute(SumoXMLAttr key, const std::string& value) {
    switch (key) {
        case SUMO_ATTR_ID:
            changeAdditionalID(value);
            break;
        case SUMO_ATTR_LANES:
            myLaneChilds = parseGNELanes(myViewNet->getNet(), value);
            break;
        case SUMO_ATTR_POSITION:
            myPosition = parse<Position>(value);
            break;
        case SUMO_ATTR_FILE:
            myFilename = value;
            break;
        case GNE_ATTR_BLOCK_MOVEMENT:
            myBlockMovement = parse<bool>(value);
            break;
        case GNE_ATTR_SELECTED:
            if(parse<bool>(value)) {
                selectAttributeCarrier();
            } else {
                unselectAttributeCarrier();
            }
            break;
        default:
            throw InvalidArgument(toString(getTag()) + " doesn't have an attribute of type '" + toString(key) + "'");
    }
    // After setting attribute always update Geometry
    updateGeometry();
}

/****************************************************************************/
