//-
// ==========================================================================
// Copyright 2015 Autodesk, Inc.  All rights reserved.
//
// Use of this software is subject to the terms of the Autodesk
// license agreement provided at the time of installation or download,
// or which otherwise accompanies this software in either electronic
// or hard copy form.
// ==========================================================================
//+

////////////////////////////////////////////////////////////////////////
//
// helixTool.cpp
//
// Description:
//     Interactive tool to draw a helix.
//     Uses OpenGL to draw a guideline for the helix.
//
//
// Steps in creating a tool command
//
// 1. Create a tool command class. 
//    Same process as an MPxCommand except define 2 methods
//    for interactive use: cancel, and finalize.
//    There is also an addition constructor MPxToolCommand(), which 
//    is called from your context when the command needs to be invoked.
////////////////////////////////////////////////////////////////////////
#include <stdio.h>
#include <maya/MIOStream.h>
#include <math.h>

#include <maya/MString.h>
#include <maya/MArgList.h>
#include <maya/MEvent.h>
#include <maya/MGlobal.h>
#include <maya/M3dView.h>
#include <maya/MPoint.h>
#include <maya/MPointArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MDagPath.h>

#include <maya/MPxContext.h>
#include <maya/MPxContextCommand.h>
#include <maya/MPxToolCommand.h> 
#include <maya/MToolsInfo.h>

#include <maya/MFnPlugin.h>
#include <maya/MFnNurbsCurve.h> 

#include <maya/MSyntax.h>
#include <maya/MArgParser.h>
#include <maya/MArgDatabase.h>
#include <maya/MCursor.h>

#include <maya/MGL.h>
#include <maya/MUIDrawManager.h>

#define PI 3.1415926

/////////////////////////////////////////////////////////////
// The users tool command
/////////////////////////////////////////////////////////////

class helixTool : public MPxToolCommand
{
public:
	helixTool(); 
	virtual			~helixTool(); 
	static void*	creator();

	MStatus			doIt(const MArgList& args);
	MStatus			redoIt();
	MStatus			undoIt();
	bool			isUndoable() const;


	void			setRadius(double newRadius);
	void			setPitch(double newPitch);
	void			setNumCVs(unsigned newNumCVs);
	void			setUpsideDown(bool newUpsideDown);

private:
	double			radius;     	// Helix radius
	double			pitch;      	// Helix pitch
	unsigned		numCV;			// Helix number of CVs
	bool			upDown;			// Helix upsideDown
	MDagPath		path;			// The dag path to the curve.
	// Don't save the pointer!
};


void* helixTool::creator()
{
	return new helixTool;
}

helixTool::~helixTool() {}

helixTool::helixTool()
{
	setPitch(0.25);
	setUpsideDown(false);
	setRadius(2.0);
	setNumCVs(20);
	setCommandString("helixToolCmd");
}

MStatus helixTool::doIt(const MArgList &args)
	//
	// Description
	//     Sets up the helix parameters from arguments passed to the
	//     MEL command.
	//
{
	return redoIt();
}

MStatus helixTool::redoIt()
	//
	// Description
	//     This method creates the helix curve from the
	//     pitch and radius values
	//
{
	MStatus stat;

	const unsigned  deg     = 3;            // Curve Degree
	const unsigned  ncvs    = numCV;		// Number of CVs
	const unsigned  spans   = ncvs - deg;   // Number of spans
	const unsigned  nknots  = spans+2*deg-1;// Number of knots
	unsigned	    i;
	MPointArray		controlVertices;
	MDoubleArray	knotSequences;

	int upFactor;
	if (upDown) upFactor = -1;
	else upFactor = 1;

	// Set up cvs and knots for the helix
	//
	for (i = 0; i < ncvs; i++)
		controlVertices.append(MPoint(radius * cos((double) i),
		upFactor * pitch * (double) i, 
		radius * sin((double) i)));

	for (i = 0; i < nknots; i++)
		knotSequences.append((double) i);

	// Now create the curve
	//
	MFnNurbsCurve curveFn;

	curveFn.create(controlVertices, knotSequences, deg, 
		MFnNurbsCurve::kOpen, false, false, 
		MObject::kNullObj, &stat);

	if (!stat) {
		stat.perror("Error creating curve");
		return stat;
	}

	stat = curveFn.getPath( path );

	return stat;
}

MStatus helixTool::undoIt()
	//
	// Description
	//     Removes the helix curve from the model.
	//
{
	MStatus stat; 
	MObject transform = path.transform();
	stat = MGlobal::deleteNode( transform );
	return stat;
}

bool helixTool::isUndoable() const
	//
	// Description
	//     Set this command to be undoable.
	//
{
	return true;	
}

void helixTool::setRadius(double newRadius)
{
	radius = newRadius;
}

void helixTool::setPitch(double newPitch)
{
	pitch = newPitch;
}

void helixTool::setNumCVs(unsigned newNumCVs)
{
	numCV = newNumCVs;
}

void helixTool::setUpsideDown(bool newUpsideDown)
{
	upDown = newUpsideDown;
}
///////////////////////////////////////////////////////////////////////
//
// The following routines are used to register/unregister
// the commands we are creating within Maya
//
///////////////////////////////////////////////////////////////////////
MStatus initializePlugin( MObject obj )
{
	MStatus status;
	MFnPlugin plugin(obj, PLUGIN_COMPANY, "3.0", "Any");

	// Register the context creation command and the tool command 
	// that the helixContext will use.
	// 
	status = plugin.registerCommand("helixToolCmd",
		helixTool::creator);
	if (!status) {
		status.perror("registerContextCommand");
		return status;
	}

	return status;
}

MStatus uninitializePlugin( MObject obj)
{
	MStatus status;
	MFnPlugin plugin( obj );

	// Deregister the tool command
	//
	status = plugin.deregisterCommand("helixToolCmd");
	if (!status) {
		status.perror("deregisterCommand");
		return status;
	}

	return status;
}
