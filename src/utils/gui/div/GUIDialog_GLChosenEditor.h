/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.org/sumo
// Copyright (C) 2001-2018 German Aerospace Center (DLR) and others.
// This program and the accompanying materials
// are made available under the terms of the Eclipse Public License v2.0
// which accompanies this distribution, and is available at
// http://www.eclipse.org/legal/epl-v20.html
// SPDX-License-Identifier: EPL-2.0
/****************************************************************************/
/// @file    GUIDialog_GLChosenEditor.h
/// @author  Daniel Krajzewicz
/// @author  Jakob Erdmann
/// @date    Thu, 11.03.2004
/// @version $Id$
///
// Editor for the list of chosen objects
/****************************************************************************/
#ifndef GUIDialog_GLChosenEditor_h
#define GUIDialog_GLChosenEditor_h


// ===========================================================================
// included modules
// ===========================================================================
#ifdef _MSC_VER
#include <windows_config.h>
#else
#include <config.h>
#endif

#include <string>
#include <vector>
#include <fx.h>

#include <utils/gui/div/GUISelectedStorage.h>
#include <utils/gui/windows/GUIMainWindow.h>

// ===========================================================================
// class declarations
// ===========================================================================
class GUIMainWindow;


// ===========================================================================
// class definition
// ===========================================================================
/**
 * @class GUIDialog_GLChosenEditor
 * @brief Editor for the list of chosen objects
 *
 * @see GUIMainWindow
 * @see GUISelectedStorage
 */
class GUIDialog_GLChosenEditor : public FXMainWindow, public GUISelectedStorage::UpdateTarget {
    // FOX-declarations
    FXDECLARE(GUIDialog_GLChosenEditor)

public:
    /** @brief Constructor (Notifies both the parent and the storage about being initialised)
     * @param[in] parent The parent window
     * @param[in] str The storage of object selections to use
     */
    GUIDialog_GLChosenEditor(GUIMainWindow* parent, GUISelectedStorage* str);

    /// @brief Destructor (Notifies both the parent and the storage about being destroyed)
    ~GUIDialog_GLChosenEditor();

    /// @brief Rebuilds the entire list
    void rebuildList();

    // @brief called if the global selection changes
    void selectionUpdated();

    /// @name FOX-callbacks
    /// @{

    /** @brief Called when the user presses the Load-button
     *
     * Opens a file dialog and forces the parent to load the list of selected
     *  objects when a file was chosen. Rebuilds the list, then, and redraws
     *  itself.
     *
     * @todo Recheck loading/saving of selections
     */
    long onCmdLoad(FXObject*, FXSelector, void*);

    /** @brief Called when the user presses the Save-button
     *
     * Opens a file dialog and forces the selection container to save the list
     *  of selected objects when a file was chosen.
     *
     * If the saveing failed, a message window is shown.
     *
     * @todo Recheck loading/saving of selections
     */
    long onCmdSave(FXObject*, FXSelector, void*);

    /** @brief Called when the user presses the Deselect-button
     *
     * Determines which items were chosen and calls GUISelectedStorage::deselect
     *  for each.
     */
    long onCmdDeselect(FXObject*, FXSelector, void*);

    /** @brief Called when the user presses the Clear-button
     *
     * Clear the internal list and calls GUISelectedStorage::clear.
     * Repaints itself
     */
    long onCmdClear(FXObject*, FXSelector, void*);

    /** @brief Called when the user presses the Close-button
     *
     * Closes itself.
     */
    long onCmdClose(FXObject*, FXSelector, void*);
    /// @}

protected:
    /// FOX needs this
    GUIDialog_GLChosenEditor() {}

private:
    /// @brief The list that holds the ids
    FXList* myList;

    /// @brief The parent window
    GUIMainWindow* myParent;

    /// @brief The storage
    GUISelectedStorage* myStorage;
};


#endif

/****************************************************************************/

