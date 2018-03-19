#include "ray.h"
#include "material.h"
#include "light.h"
#include "../fileio/bitmap.h"
#include "../fileio/pngimage.h"
#include "../fileio/imageio.h"
#include "../ui/TraceUI.h"


using namespace std;
extern bool debugMode;

extern TraceUI* traceUI;


// Apply the Phong model to this point on the surface of the object, returning
// the color of that point.
Vec3d Material::shade( Scene *scene, const ray& r, const isect& i ) const
{
	// YOUR CODE HERE

	// For now, this method just returns the diffuse color of the object.
	// This gives a single matte color for every distinct surface in the
	// scene, and that's it.  Simple, but enough to get you started.
	// (It's also inconsistent with the Phong model...)

	// Your mission is to fill in this method with the rest of the phong
	// shading model, including the contributions of all the light sources.
    // You will need to call both distanceAttenuation() and shadowAttenuation()
    // somewhere in your code in order to compute shadows and light falloff.
	if( debugMode )
		std::cout << "Debugging the Phong code (or lack thereof...)" << std::endl;

	// When you're iterating through the lights,
	// you'll want to use code that looks something
	// like this:
	//
	// for ( vector<Light*>::const_iterator litr = scene->beginLights(); 
	// 		litr != scene->endLights(); 
	// 		++litr )
	// {
	// 		Light* pLight = *litr;
	// 		.
	// 		.
	// 		.
	// }
	Vec3d Qpoint = r.at(i.t);
	Vec3d intensity = ke(i) + prod(ka(i), scene->ambient());

	for (vector<Light*>::const_iterator litr = scene->beginLights(); litr != scene->endLights(); ++litr)
	{
		if (kd(i).iszero() && ks(i).iszero())
		{
			continue;
		}
		Light* pLight = *litr;
		// Diffuse Term
		Vec3d directionToLight = pLight->getDirection(Qpoint);
		directionToLight.normalize();
		Vec3d lightIntensity = pLight->distanceAttenuation(Qpoint) * pLight->shadowAttenuation(r, Qpoint);
		//Vec3d lightIntensity = pLight->distanceAttenuation(Qpoint) * pLight->shadowAttenuation(Qpoint);

		if (!kd(i).iszero())
		{
			intensity = intensity + prod(kd(i), lightIntensity) * max((i.N * directionToLight), 0.0);
		}
		// Specular term
		if (!ks(i).iszero())
		{
			Vec3d viewingDirection = scene->getCamera().getEye() - Qpoint;
			viewingDirection.normalize();
			Vec3d cosVector = i.N * (directionToLight * i.N);
			Vec3d sinVector = cosVector - directionToLight;
			Vec3d reflectedDirection = cosVector + sinVector;
			reflectedDirection.normalize();
			intensity = intensity + prod(ks(i), lightIntensity) * pow(max(reflectedDirection*viewingDirection, 0.0), shininess(i));
		}
	}
	return intensity;




	//Vec3d phong = ke(i) + ka(i) * scene->ambient();
	//Vec3d normal = i.N;
	//Vec3d iters;
	//Vec3d sample = Vec3d(1.0, 1.0, 1.0);

	//for (vector<Light*>::const_iterator litr = scene->beginLights();
	//	litr != scene->endLights(); ++litr) {
	//	Light* pLight = *litr;

	//	Vec3d atten = pLight->shadowAttenuation(r, r.at(i.t));
	//	Vec3d lightDir = (pLight->getDirection(r.at(i.t)));

	//	float lDotN = normal * (lightDir);

	//	Vec3d reflection = 2 * (lDotN)*normal - lightDir;
	//	double VdotR = (-r.getDirection()) * reflection;

	//	//iters = (atten % pLight->getColor(r.at(i.t))) % (kd(i)*max((lDotN), 0.0f) + ks(i)*pow(max(VdotR, 0.0), shininess(i)));
	//	iters = (atten % pLight->getColor()) % (kd(i)*max((lDotN), 0.0f) + ks(i)*pow(max(VdotR, 0.0), shininess(i)));

	//	iters *= min(1.0, (pLight->distanceAttenuation(r.at(i.t))));

	//	phong += iters;
	//}

	//return phong;
	//return kd(i);
}


TextureMap::TextureMap( string filename )
{
    data = load( filename.c_str(), width, height );
    if( 0 == data )
    {
        width = 0;
        height = 0;
        string error( "Unable to load texture map '" );
        error.append( filename );
        error.append( "'." );
        throw TextureMapException( error );
    }
}

Vec3d TextureMap::getMappedValue( const Vec2d& coord ) const
{
	// YOUR CODE HERE

    // In order to add texture mapping support to the 
    // raytracer, you need to implement this function.
    // What this function should do is convert from
    // parametric space which is the unit square
    // [0, 1] x [0, 1] in 2-space to Image coordinates,
    // and use these to perform bilinear interpolation
    // of the values.
	float xCor = coord[0] * width,
	yCor = coord[1] * height;
	int lowXindex = (int)xCor;
	int lowYindex = (int)yCor;
	float deltaX = xCor - lowXindex, deltaY = yCor - lowYindex;
	float a = (1 - deltaX)*(1 - deltaY), b = (deltaX)*(1 - deltaY), c = (1 - deltaX)*deltaY, d = deltaX * deltaY;
	return a * getPixelAt(lowXindex, lowYindex) + b * getPixelAt(lowXindex + 1, lowYindex) + c * getPixelAt(lowXindex, lowYindex + 1) + d * getPixelAt(lowXindex + 1, lowYindex + 1);

    //return Vec3d(1.0, 1.0, 1.0);
}


Vec3d TextureMap::getPixelAt( int x, int y ) const
{
    // This keeps it from crashing if it can't load
    // the texture, but the person tries to render anyway.
    if (0 == data)
      return Vec3d(1.0, 1.0, 1.0);

    if( x >= width )
       x = width - 1;
    if( y >= height )
       y = height - 1;

    // Find the position in the big data array...
    int pos = (y * width + x) * 3;
    return Vec3d( double(data[pos]) / 255.0, 
       double(data[pos+1]) / 255.0,
       double(data[pos+2]) / 255.0 );
}

Vec3d MaterialParameter::value( const isect& is ) const
{
    if( 0 != _textureMap )
        return _textureMap->getMappedValue( is.uvCoordinates );
    else
        return _value;
}

double MaterialParameter::intensityValue( const isect& is ) const
{
    if( 0 != _textureMap )
    {
        Vec3d value( _textureMap->getMappedValue( is.uvCoordinates ) );
        return (0.299 * value[0]) + (0.587 * value[1]) + (0.114 * value[2]);
    }
    else
        return (0.299 * _value[0]) + (0.587 * _value[1]) + (0.114 * _value[2]);
}

//Start Bumpmap code here
