/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.org/sumo
// Copyright (C) 2001-2018 German Aerospace Center (DLR) and others.
// This program and the accompanying materials
// are made available under the terms of the Eclipse Public License v2.0
// which accompanies this distribution, and is available at
// http://www.eclipse.org/legal/epl-v20.html
// SPDX-License-Identifier: EPL-2.0
/****************************************************************************/
/// @file    GNEVaporizer.cpp
/// @author  Pablo Alvarez Lopez
/// @date    Jun 2016
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
#include <netedit/netelements/GNEEdge.h>
#include <netedit/netelements/GNELane.h>
#include <netedit/GNEViewNet.h>
#include <netedit/GNEUndoList.h>
#include <netedit/GNENet.h>
#include <netedit/changes/GNEChange_Attribute.h>
#include <netedit/GNEViewParent.h>

#include "GNEVaporizer.h"


// ===========================================================================
// member method definitions
// ===========================================================================

GNEVaporizer::GNEVaporizer(GNEViewNet* viewNet, GNEEdge* edge, double startTime, double end) :
    GNEAdditional(viewNet->getNet()->generateVaporizerID(), viewNet, GLO_VAPORIZER, SUMO_TAG_VAPORIZER, ICON_VAPORIZER, false, false),
    myEdge(edge),
    myStartTime(startTime),
    myEnd(end),
    myRelativePositionY(0) {
}


GNEVaporizer::~GNEVaporizer() {
}


void
GNEVaporizer::updateGeometry() {
    // Clear all containers
    myShapeRotations.clear();
    myShapeLengths.clear();

    // clear Shape
    myShape.clear();

    // obtain relative position of vaporizer in edge
    myRelativePositionY = 2 * myEdge->getVaporizerRelativePosition(this);

    // get lanes of edge
    GNELane* firstLane = myEdge->getLanes().at(0);

    // Save number of lanes
    myNumberOfLanes = int(myEdge->getLanes().size());

    // Get shape of lane parent
    double offset = firstLane->getShape().length() < 2.5 ? firstLane->getShape().length() : 2.5;
    myShape.push_back(firstLane->getShape().positionAtOffset(offset));

    // Obtain first position
    Position f = myShape[0] - Position(1, 0);

    // Obtain next position
    Position s = myShape[0] + Position(1, 0);

    // Save rotation (angle) of the vector constructed by points f and s
    myShapeRotations.push_back(firstLane->getShape().rotationDegreeAtOffset(offset) * -1);

    // Set block icon position
    myBlockIconPosition = myShape.getLineCenter();

    // Set offset of the block icon
    myBlockIconOffset = Position(1.1, (-3.06) - myRelativePositionY);

    // Set block icon rotation, and using their rotation for logo
    setBlockIconRotation(firstLane);

    // Refresh element (neccesary to avoid grabbing problems)
    myViewNet->getNet()->refreshElement(this);
}


Position
GNEVaporizer::getPositionInView() const {
    Position A = myEdge->getLanes().front()->getShape().positionAtOffset(5);
    Position B = myEdge->getLanes().back()->getShape().positionAtOffset(5);

    // return Middle point
    return Position((A.x() + B.x()) / 2, (A.y() + B.y()) / 2);
}


void
GNEVaporizer::moveGeometry(const Position&, const Position&) {
    // This additional cannot be moved
}


void
GNEVaporizer::commitGeometryMoving(const Position&, GNEUndoList*) {
    // This additional cannot be moved
}


void
GNEVaporizer::writeAdditional(OutputDevice& device) const {
    // Write parameters
    device.openTag(getTag());
    writeAttribute(device, SUMO_ATTR_EDGE);
    writeAttribute(device, SUMO_ATTR_STARTTIME);
    writeAttribute(device, SUMO_ATTR_END);
    // Close tag
    device.closeTag();
}


const std::string&
GNEVaporizer::getParentName() const {
    return myEdge->getMicrosimID();
}


void
GNEVaporizer::drawGL(const GUIVisualizationSettings& s) const {
    // get values
    glPushName(getGlID());
    double width = (double) 2.0 * s.scale;
    glLineWidth(1.0);
    const double exaggeration = s.addSize.getExaggeration(s);

    // set color
    if (isAttributeCarrierSelected()) {
        GLHelper::setColor(myViewNet->getNet()->selectedAdditionalColor);
    } else {
        GLHelper::setColor(RGBColor(120, 216, 0));
    }
    // draw shape
    glPushMatrix();
    glTranslated(0, 0, getType());
    glTranslated(myShape[0].x(), myShape[0].y(), 0);
    glRotated(myShapeRotations[0], 0, 0, 1);
    glScaled(exaggeration, exaggeration, 1);
    glTranslated(-1.6, -1.6, 0);
    glBegin(GL_QUADS);
    glVertex2d(0,  0.25);
    glVertex2d(0, -0.25);
    glVertex2d((myNumberOfLanes * 3.3), -0.25);
    glVertex2d((myNumberOfLanes * 3.3),  0.25);
    glEnd();
    glTranslated(0, 0, .01);
    glBegin(GL_LINES);
    glVertex2d(0, 0.25 - .1);
    glVertex2d(0, -0.25 + .1);
    glEnd();

    // draw position indicator (White) if isn't being drawn for selecting
    if ((width * exaggeration > 1) && !s.drawForSelecting) {
        if (isAttributeCarrierSelected()) {
            GLHelper::setColor(myViewNet->getNet()->selectionColor);
        } else {
            GLHelper::setColor(RGBColor::WHITE);
        }
        glRotated(90, 0, 0, -1);
        glBegin(GL_LINES);
        glVertex2d(0, 0);
        glVertex2d(0, (myNumberOfLanes * 3.3));
        glEnd();
    }

    // Pop shape matrix
    glPopMatrix();

    // Add a draw matrix for drawing logo
    glPushMatrix();
    glTranslated(myShape[0].x(), myShape[0].y(), getType());
    glRotated(myShapeRotations[0], 0, 0, 1);
    glTranslated((-2.56) - myRelativePositionY, (-1.6), 0);

    // Draw icon depending of Vaporizer is selected and if isn't being drawn for selecting
    if(s.drawForSelecting) {
        GLHelper::setColor(RGBColor::GREEN);
        GLHelper::drawBoxLine(Position(0, 1), 0, 2, 1);
    } else {
        glColor3d(1, 1, 1);
        glRotated(-90, 0, 0, 1);
        if (isAttributeCarrierSelected()) {
            GUITexturesHelper::drawTexturedBox(GUITextureSubSys::getTexture(GNETEXTURE_VAPORIZERSELECTED), 1);
        } else {
            GUITexturesHelper::drawTexturedBox(GUITextureSubSys::getTexture(GNETEXTURE_VAPORIZER), 1);
        }
    }

    // Pop logo matrix
    glPopMatrix();

    // Check if the distance is enought to draw details
    if ((s.scale * exaggeration >= 10) && !s.drawForSelecting) {
        // Show Lock icon depending of the Edit mode
        drawLockIcon(0.4);
    }

    // Finish draw
    drawName(getCenteringBoundary().getCenter(), s.scale, s.addName);
    glPopName();
}


std::string
GNEVaporizer::getAttribute(SumoXMLAttr key) const {
    switch (key) {
        case SUMO_ATTR_ID:
            return getAdditionalID();
        case SUMO_ATTR_EDGE:
            return myEdge->getID();
        case SUMO_ATTR_STARTTIME:
            return toString(myStartTime);
        case SUMO_ATTR_END:
            return toString(myEnd);
        case GNE_ATTR_SELECTED:
            return toString(isAttributeCarrierSelected());
        default:
            throw InvalidArgument(toString(getTag()) + " doesn't have an attribute of type '" + toString(key) + "'");
    }
}


void
GNEVaporizer::setAttribute(SumoXMLAttr key, const std::string& value, GNEUndoList* undoList) {
    if (value == getAttribute(key)) {
        return; //avoid needless changes, later logic relies on the fact that attributes have changed
    }
    switch (key) {
        case SUMO_ATTR_ID:
        case SUMO_ATTR_EDGE:
        case SUMO_ATTR_STARTTIME:
        case SUMO_ATTR_END:
        case GNE_ATTR_SELECTED:
            undoList->p_add(new GNEChange_Attribute(this, key, value));
            break;
        default:
            throw InvalidArgument(toString(getTag()) + " doesn't have an attribute of type '" + toString(key) + "'");
    }
}


bool
GNEVaporizer::isValid(SumoXMLAttr key, const std::string& value) {
    switch (key) {
        case SUMO_ATTR_ID:
            return isValidAdditionalID(value);
        case SUMO_ATTR_EDGE:
            if (myViewNet->getNet()->retrieveEdge(value, false) != nullptr) {
                return true;
            } else {
                return false;
            }
        case SUMO_ATTR_STARTTIME:
            if (canParse<double>(value) && (parse<double>(value) >= 0)) {
                return (parse<double>(value) <= myEnd);
            } else {
                return false;
            }
        case SUMO_ATTR_END:
            if (canParse<double>(value) && (parse<double>(value) >= 0)) {
                return (myStartTime <= parse<double>(value));
            } else {
                return false;
            }
        case GNE_ATTR_SELECTED:
            return canParse<bool>(value);
        default:
            throw InvalidArgument(toString(getTag()) + " doesn't have an attribute of type '" + toString(key) + "'");
    }
}


void
GNEVaporizer::setAttribute(SumoXMLAttr key, const std::string& value) {
    switch (key) {
        case SUMO_ATTR_ID:
            changeAdditionalID(value);
            break;
        case SUMO_ATTR_EDGE:
            myEdge = changeEdge(myEdge, value);
            break;
        case SUMO_ATTR_STARTTIME:
            myStartTime = parse<double>(value);
            break;
        case SUMO_ATTR_END:
            myEnd = parse<double>(value);
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
