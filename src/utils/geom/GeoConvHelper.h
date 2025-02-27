/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.org/sumo
// Copyright (C) 2001-2018 German Aerospace Center (DLR) and others.
// This program and the accompanying materials
// are made available under the terms of the Eclipse Public License v2.0
// which accompanies this distribution, and is available at
// http://www.eclipse.org/legal/epl-v20.html
// SPDX-License-Identifier: EPL-2.0
/****************************************************************************/
/// @file    GeoConvHelper.h
/// @author  Daniel Krajzewicz
/// @author  Jakob Erdmann
/// @author  Michael Behrisch
/// @date    2006-08-01
/// @version $Id$
///
// static methods for processing the coordinates conversion for the current net
/****************************************************************************/
#ifndef GeoConvHelper_h
#define GeoConvHelper_h


// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#include <map>
#include <string>
#include <utils/geom/Position.h>
#include <utils/geom/Boundary.h>

#ifdef HAVE_PROJ
#include <proj_api.h>
#endif


// ===========================================================================
// class declarations
// ===========================================================================

class OptionsCont;
class PositionVector;
class OutputDevice;

// ===========================================================================
// class definitions
// ===========================================================================
/**
 * @class GeoConvHelper
 * @brief static methods for processing the coordinates conversion for the current net
 */
class GeoConvHelper {
public:

    /**@brief Constructor based on the stored options
     * @param[in] oc The OptionsCont from which to read options
     */
    GeoConvHelper(OptionsCont& oc);

    /// @brief Constructor
    GeoConvHelper(const std::string& proj, const Position& offset,
                  const Boundary& orig, const Boundary& conv, double scale = 1.0, double rot = 0.0, bool inverse = false);

    /// @brief Destructor
    ~GeoConvHelper();

    /**@brief Adds projection options to the given container
     * @param[in] oc The options container to add the options to
     * @todo let the container be retrieved
     */
    static void addProjectionOptions(OptionsCont& oc);

    /// @brief Initialises the processing and the final instance using the given options
    static bool init(OptionsCont& oc);

    /// @brief Initialises the processing and the final instance using the given proj.4-definition and complete network parameter
    static void init(const std::string& proj, const Position& offset, const Boundary& orig,
                     const Boundary& conv, double scale = 1.0);

    /**@brief the coordinate transformation to use for input conversion and processing
     * @note instance is modified during use: boundary may adapt to new coordinates
     */
    static GeoConvHelper& getProcessing() {
        return myProcessing;
    }

    /// @brief the coordinate transformation that was loaded fron an input file
    static GeoConvHelper& getLoaded() {
        return myLoaded;
    }

    static int getNumLoaded() {
        return myNumLoaded;
    }

    /**@brief compute the location attributes which will be used for output
     * based on the loaded location data, the given options and the transformations applied during processing
     */
    static void computeFinal(bool lefthand = false);

    /// @brief the coordinate transformation for writing the location element and for tracking the original coordinate system
    static const GeoConvHelper& getFinal() {
        return myFinal;
    }

    /// @brief sets the coordinate transformation loaded from a location element
    static void setLoaded(const GeoConvHelper& loaded);

    /// @brief @brief resets loaded location elements
    static void resetLoaded();

    /// @brief Converts the given cartesian (shifted) position to its geo (lat/long) representation
    void cartesian2geo(Position& cartesian) const;

    /**@brief Converts the given coordinate into a cartesian and optionally update myConvBoundary
     * @note: initializes UTM / DHDN projection on first use (select zone)
     */
    bool x2cartesian(Position& from, bool includeInBoundary = true);

    /// @brief Converts the given coordinate into a cartesian using the previous initialisation
    bool x2cartesian_const(Position& from) const;

    /// @brief Returns whether a transformation from geo to metric coordinates will be performed
    bool usingGeoProjection() const;

    /// @brief Returns the information whether an inverse transformation will happen
    bool usingInverseGeoProjection() const;

    /// @brief Shifts the converted boundary by the given amounts
    void moveConvertedBy(double x, double y);

    /// @brief Returns the original boundary
    const Boundary& getOrigBoundary() const;

    /// @brief Returns the converted boundary
    const Boundary& getConvBoundary() const;

    /// @brief sets the converted boundary
    void setConvBoundary(const Boundary& boundary) {
        myConvBoundary = boundary;
    }

    /// @brief Returns the network offset
    const Position getOffset() const;

    /// @brief Returns the network base
    const Position getOffsetBase() const;

    /// @brief Returns the network offset
    const std::string& getProjString() const;

    /// @brief @brief writes the location element
    static void writeLocation(OutputDevice& into);

    bool operator==(const GeoConvHelper& o) const;

    bool operator!=(const GeoConvHelper& o) const {
        return !(*this == o);
    }

private:
    /// @brief projection method
    enum ProjectionMethod {
        NONE,
        SIMPLE,
        UTM,
        DHDN,
        DHDN_UTM,
        PROJ
    };

    /// @brief A proj options string describing the proj.4-projection to use
    std::string myProjString;

#ifdef HAVE_PROJ
    /// @brief The proj.4-projection to use
    projPJ myProjection;

    /// @brief The inverse proj.4-projection to use first
    projPJ myInverseProjection;

    /// @brief The geo proj.4-projection which is the target of the inverse projection
    projPJ myGeoProjection;
#endif

    /// @brief The offset to apply
    Position myOffset;

    /// @brief The scaling to apply to geo-coordinates
    double myGeoScale;

    /// @brief The rotation to apply to geo-coordinates
    double mySin;
    double myCos;

    /// @brief Information whether no projection shall be done
    ProjectionMethod myProjectionMethod;

    /// @brief Information whether inverse projection shall be used
    bool myUseInverseProjection;

    /// @brief The boundary before conversion (x2cartesian)
    Boundary myOrigBoundary;

    /// @brief The boundary after conversion (x2cartesian)
    Boundary myConvBoundary;

    /// @brief coordinate transformation to use for input conversion and processing
    static GeoConvHelper myProcessing;

    /// @brief coordinate transformation loaded from a location element
    static GeoConvHelper myLoaded;

    /// @brief coordinate transformation to use for writing the location element and for tracking the original coordinate system
    static GeoConvHelper myFinal;

    /// @brief the numer of coordinate transformations loaded from location elements
    static int myNumLoaded;

    /// @brief make assignment operator private
    GeoConvHelper& operator=(const GeoConvHelper&);

    /// @brief invalidated copy constructor.
    GeoConvHelper(const GeoConvHelper&) = delete;
};


#endif

/****************************************************************************/

