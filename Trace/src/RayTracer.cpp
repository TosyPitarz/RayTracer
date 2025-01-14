// The main ray tracer.

#pragma warning (disable: 4786)

#include "RayTracer.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/ray.h"

#include "parser/Tokenizer.h"
#include "parser/Parser.h"

#include "ui/TraceUI.h"
#include <cmath>
#include <algorithm>

extern TraceUI* traceUI;

#include <iostream>
#include <fstream>

using namespace std;

// Use this variable to decide if you want to print out
// debugging messages.  Gets set in the "trace single ray" mode
// in TraceGLWindow, for example.
bool debugMode = false;

// Trace a top-level ray through normalized window coordinates (x,y)
// through the projection plane, and out into the scene.  All we do is
// enter the main ray-tracing method, getting things started by plugging
// in an initial ray weight of (0.0,0.0,0.0) and an initial recursion depth of 0.
Vec3d RayTracer::trace( double x, double y )
{
	// Clear out the ray cache in the scene for debugging purposes,
	scene->intersectCache.clear();

    ray r( Vec3d(0,0,0), Vec3d(0,0,0), ray::VISIBILITY );

    scene->getCamera().rayThrough( x,y,r );
	Vec3d ret = traceRay( r, Vec3d(1.0,1.0,1.0), 0 );
	ret.clamp();
	return ret;
}


Vec3d RayTracer::traceRay(const ray& r, const Vec3d& thresh, int depth)
{
	isect i;

	if (scene->intersect(r, i)) {
		// An intersection occurred!

		const Material& m = i.getMaterial();
		Vec3d kt = m.kt(i);
		Vec3d kr = m.kr(i);

		if (depth < traceUI->getDepth()) {

			// Work for Reflection Ray
			Vec3d reflect;
			Vec3d reflectAngle = ((2 * (-r.getDirection() * i.N)) * i.N) + r.getDirection();

			reflectAngle.normalize();
			Vec3d offset = reflectAngle * 0.00001;

			ray reflectRay(r.at(i.t - 0.01)+ offset, reflectAngle, ray::REFLECTION);
			reflect = traceRay(reflectRay, thresh, depth + 1);

			// Work for Refraction Ray
			Vec3d transmit;
			if (!kt.iszero()) {

				ray::RayType rayType;
				Vec3d transmitAngle;
				double eta;
				double incidentIndex, transmitIndex;
				double cos_i, cos_t;

				Vec3d normal;

				if (r.type() == ray::REFRACTION) {
					rayType = ray::VISIBILITY;
					incidentIndex = m.index(i);
					transmitIndex = 1.0;
					normal = -i.N;
				}
				else {
					rayType = ray::REFRACTION;
					incidentIndex = 1.0;
					transmitIndex = m.index(i);
					normal = i.N;
				}

				eta = incidentIndex / transmitIndex;

				cos_i = -(normal * r.getDirection());

				cos_t = sqrt(1 - pow(eta, 2)*(1 - pow(cos_i, 2)));

				transmitAngle = (eta * r.getDirection()) + (eta*cos_i - cos_t)*normal;

				ray transmitRay(r.at(i.t + 0.01), transmitAngle, rayType);

				transmit = traceRay(transmitRay, thresh, depth + 1);
			}

			return m.shade(scene, r, i) + kr % reflect + kt % transmit;
		}
		else {
			return m.shade(scene, r, i);
		}

	}
	else {
		// No intersection.  This ray travels to infinity, so we color
		// it according to the background color, which in this (simple) case
		// is just black.

		return Vec3d(0.0, 0.0, 0.0);
	}
}
//// Do recursive ray tracing!  You'll want to insert a lot of code here
//// (or places called from here) to handle reflection, refraction, etc etc.
//Vec3d RayTracer::traceRay( const ray& r, const Vec3d& thresh, int depth )
//{
//	isect i;
//
//	if( scene->intersect( r, i ) ) {
//		// YOUR CODE HERE
//
//		// An intersection occured!  We've got work to do.  For now,
//		// this code gets the material for the surface that was intersected,
//		// and asks that material to provide a color for the ray.  
//
//		// This is a great place to insert code for recursive ray tracing.
//		// Instead of just returning the result of shade(), add some
//		// more steps: add in the contributions from reflected and refracted
//		// rays.
//
//		const Material& m = i.getMaterial();
//		double t = i.t;
//	/*	Vector offset = resultantReflection * 0.001;
//		Ray reflectionRay(intersectionRayPos + offset, resultantReflection);
//*/
//		
//		Vec3d N = i.N;
//		Vec3d I = m.shade(scene,r,i);
//		if (!m.kr(i).iszero() && depth >= 0 ){
//			//computing reflection direction
//			Vec3d raydir = r.getDirection();
//			Vec3d reflectionDir = 2 * (-raydir * i.N) * i.N + raydir;
//			reflectionDir.normalize();
//
//			Vec3d offset = reflectionDir * 0.001;
//
//			//ray reflectionRay = ray(r.at(i.t), reflectionDir, ray::RayType::REFLECTION);
//			ray reflectionRay = ray(r.at(i.t), reflectionDir, ray::RayType::REFLECTION);
//
//			Vec3d reflection = traceRay(reflectionRay, thresh, depth - 1);
//			Vec3d R = reflection;
//			I += R;
//
//		}
//		return I;
//		//return m.shade(scene, r, i);
//	
//	} else {
//		// No intersection.  This ray travels to infinity, so we color
//		// it according to the background color, which in this (simple) case
//		// is just black.
//
//		return Vec3d( 0.0, 0.0, 0.0 );
//	}
//}

RayTracer::RayTracer()
	: scene( 0 ), buffer( 0 ), buffer_width( 256 ), buffer_height( 256 ), m_bBufferReady( false )
{
}


RayTracer::~RayTracer()
{
	delete scene;
	delete [] buffer;
}

void RayTracer::getBuffer( unsigned char *&buf, int &w, int &h )
{
	buf = buffer;
	w = buffer_width;
	h = buffer_height;
}

double RayTracer::aspectRatio()
{
	return sceneLoaded() ? scene->getCamera().getAspectRatio() : 1;
}

bool RayTracer::loadScene( char* fn )
{
	ifstream ifs( fn );
	if( !ifs ) {
		string msg( "Error: couldn't read scene file " );
		msg.append( fn );
		traceUI->alert( msg );
		return false;
	}
	
	// Strip off filename, leaving only the path:
	string path( fn );
	if( path.find_last_of( "\\/" ) == string::npos )
		path = ".";
	else
		path = path.substr(0, path.find_last_of( "\\/" ));

	// Call this with 'true' for debug output from the tokenizer
	Tokenizer tokenizer( ifs, false );
    Parser parser( tokenizer, path );
	try {
		delete scene;
		scene = 0;
		scene = parser.parseScene();
	} 
	catch( SyntaxErrorException& pe ) {
		traceUI->alert( pe.formattedMessage() );
		return false;
	}
	catch( ParserException& pe ) {
		string msg( "Parser: fatal exception " );
		msg.append( pe.message() );
		traceUI->alert( msg );
		return false;
	}
	catch( TextureMapException e ) {
		string msg( "Texture mapping exception: " );
		msg.append( e.message() );
		traceUI->alert( msg );
		return false;
	}


	if( ! sceneLoaded() )
		return false;

	
	return true;
}

void RayTracer::traceSetup( int w, int h )
{
	if( buffer_width != w || buffer_height != h )
	{
		buffer_width = w;
		buffer_height = h;

		bufferSize = buffer_width * buffer_height * 3;
		delete [] buffer;
		buffer = new unsigned char[ bufferSize ];

	}
	memset( buffer, 0, w*h*3 );
	m_bBufferReady = true;
}

void RayTracer::tracePixel( int i, int j )
{
	Vec3d col;

	if( ! sceneLoaded() )
		return;

	double x = double(i)/double(buffer_width);
	double y = double(j)/double(buffer_height);

	col = trace( x,y );
	unsigned char *pixel = buffer + ( i + j * buffer_width ) * 3;

	pixel[0] = (int)( 255.0 * col[0]);
	pixel[1] = (int)( 255.0 * col[1]);
	pixel[2] = (int)( 255.0 * col[2]);
}

