/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                          <jrheinlaender[at]users.sourceforge.net>       *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef FEM_CONSTRAINT_H
#define FEM_CONSTRAINT_H

#include <Base/Vector3D.h>
#include <App/FeaturePython.h>
#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <App/PropertyGeo.h>

namespace Fem {

/**
 * @brief Base class of all Constraint Objects of the Fem module.
 *
 * @details
 *  @ref Constraint isn't intended to be used directely. Actual Constraints
 *  used to specify a simulation are children of this class. The base class
 *  essentially does two things: Most importantely it has a property @ref
 *  Constraint::References which is a list of all sub objects the constraint
 *  applys to. Defining it in the base class exposes a common interface to code
 *  using different constraints.
 *
 *  The second purpose of @ref Constraint is to support the redering to the
 *  screen done by the View Provider @ref FemGui::ViewProviderFemConstraint.
 *  The rendering is decoupled from the objects listed in the @ref References
 *  property by using a point cloud a normal vector and a scale factor which is
 *  generated by this class. The View Provider doesn't know of the references
 *  it just asks @ref Constraint for those values and renders a widget for each
 *  point scaled by the scale factor pointing in the direction of the normal
 *  vector. These values are exposed by the two properties @ref NormalDirection
 *  and @ref Scale and the protected method @ref getPoints(points&, normals&,
 *  scale&).
 */
class AppFemExport Constraint : public App::DocumentObject {
    PROPERTY_HEADER(Fem::Constraint);

public:
    Constraint();
    virtual ~Constraint();

    /**
     * @brief List of objects the constraints applies to.
     *
     * @details
     *  This is a list of subobjects (e.g. Faces, Edges, ...) the constraint
     *  applies to. It's only supposed to contain objects of or derived from
     *  Part::Feature. Altering this property triggers a update of @ref
     *  NormalDirection and @ref Scale.
     *
     * @note
     *  Undefined behaviour if a unsupported (not derived from Part::Feature)
     *  Document Object is added to the @References.
     */
    App::PropertyLinkSubList References;

    /**
     * @brief Vector pointing into the effective direction of the constraint.
     *
     * @details
     *  If @ref References contains only one face of a shape than @ref
     *  NormalDirection is the normal vector of that face. If more than one
     *  face is referenced that it is the normal vector of the first face. If
     *  @ref References is empty or doesn't contain a face the value of @ref
     *  NormalDirection is the Z-axis or its previous value.
     */
    App::PropertyVector NormalDirection;

    /**
     * @brief Supposed to reflect the size of the @ref References.
     *
     * @details
     *  This property should be a scale factor for the widgets rendered by the
     *  View Provider but it's always 1. It isn't updated when @ref References
     *  changes.
     */
    App::PropertyInteger Scale;

    /**
     * @brief Updates @ref NormalDirection.
     *
     * @details
     *  Updates @ref NormalDirection using new @ref References. It does so by
     *  calling @ref onChanged once with the @ref References property and once
     *  with the @ref Scale property. The second call doesn't do anything.
     *
     * @note
     *  Calling @ref onChanged does touch the Document Object but that flag is
     *  cleared right after the @ref execute call by the recompute mechanism.
     *  See Document::recompute() and DocumentObject::purgeTouched().
     */
    virtual App::DocumentObjectExecReturn *execute();

    /**
     * @brief Calculates scale factor based on length of edge.
     *
     * @details
     *  Used to calculate the scale factor returned by @ref getPoints when the
     *  scale factor is calculated for a face.
     *
     * @note
     *  This method does a really crazy calculation that I didn't dare to try
     *  to understand.
     */
    int calcDrawScaleFactor(double lparam) const;

    /**
     * @brief Calculates scale factor based on size of face.
     *
     * @details
     *  Used to calculate the scale factor returned by @ref getPoints when the
     *  scale factor is calculated for a edge.
     *
     * @note
     *  This method does a really crazy calculation that I didn't dare to try
     *  to understand.
     */
    int calcDrawScaleFactor(double lvparam, double luparam) const;

    /**
     * @brief Returns default scale factor of 1.
     *
     * @details
     *  This is just used to make code more understandable. Other versions
     *  (overloads) of this function do useful calculations based on faces or
     *  edges. Used by @ref getPoints if no useful shape information is
     *  avaliable.
     *
     * @return always the integer 1
     */
    int calcDrawScaleFactor() const;

    virtual const char* getViewProviderName() const {
        return "FemGui::ViewProviderFemConstraint";
    }

protected:

    /**
     * @brief Updates NormalDirection if References change.
     */
    virtual void onChanged(const App::Property* prop);

    /**
     * @brief Triggers @ref onChanged to update View Provider.
     *
     * @note
     *  This should not be nessesary and is properly a bug in the View Provider
     *  of FemConstraint.
     */
    virtual void onDocumentRestored();

    /**
     * @brief Returns data based on References relevant for rendering widgets.
     *
     * @details
     *  Extracts data from all objects inside References relevant for widget
     *  rendering by the View Provider. This includes the points at which
     *  widgets shall be drawn, a vector per point indicating the direction the
     *  widget should face and a global scale factor for all widgets. Two
     *  vectors of equal length are used to return the points and their normal
     *  vectors. The normal vector of points[i] can be found with the same
     *  index in normals[i].
     *
     * @param[out] points
     *  For each vertex a point equal to the location of that vertix is pushed
     *  into the points vector. For each edge at least to points, the beginning
     *  and the end of the edge, are pushed into the vector. Depending on the
     *  length of the edge more points may be added in between. For each face a
     *  number of points depending on the size of the face and the step size
     *  calculated internally are pushed into the vector.
     * @param[out] normals
     *  For vertexes and edges normal vectors equal to the NormalDirection are
     *  pushed onto the vector. For each point of a face a Base::Vector3d equal
     *  to the normal vector of the face at that position is added to the
     *  vector.
     * @param[out] scale
     *  The scale contains a scale value for the object in References that was
     *  processed last. For calculation various versions of @ref
     *  calcDrawScaleFactor are used.
     *
     * @return
     *  If the calculation of points, normals and scale was successful it
     *  returns true. If an error occured and the data couldn't be extracted
     *  properly false is returned.
     */
    bool getPoints(
            std::vector<Base::Vector3d>& points,
            std::vector<Base::Vector3d>& normals,
            int * scale) const;

    /**
     * @brief Extract properties of cylindrical face.
     *
     * @note
     *  This method is very specific and doesn't requre access to member
     *  variables. It should be rewritten at a different palce.
     */
    bool getCylinder(
            double& radius, double& height,
            Base::Vector3d& base, Base::Vector3d& axis) const;

    /**
     * @brief Calculate point of cylidrical face where to render widget.
     *
     * @note
     *  This method is very specific and doesn't requre access to member
     *  variables. It should be rewritten at a different palce.
     */
    Base::Vector3d getBasePoint(const Base::Vector3d& base, const Base::Vector3d& axis,
                                const App::PropertyLinkSub &location, const double& dist);
    /**
     * @brief Get normal vector of point calculated by @ref getBasePoint.
     *
     * @note
     *  This method is very specific and doesn't requre access to member
     *  variables. It should be rewritten at a different palce.
     */
    const Base::Vector3d getDirection(const App::PropertyLinkSub &direction);
};

typedef App::FeaturePythonT<Constraint> ConstraintPython;


} //namespace Fem


#endif // FEM_CONSTRAINT_H
