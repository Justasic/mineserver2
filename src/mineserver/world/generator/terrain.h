/*This will generate all the terrain for
the map. This includes mountains and flats
ect. I am going to use libnoise for now.*/

#include <noise/noise.h>
#include <boost/shared_ptr.hpp>

namespace worldGeneration
{
  class terrainGen
  {

    noise::module::Perlin terrainHieght;
    noise::module::Perlin rainfall;
    noise::module::Perlin tempratureBase;
    noise::module::ScaleBias hieghtScale;
    noise::module::Add temprature;

    noise::module::Perlin ocean;
    noise::module::Billow flatLand;
    noise::module::Perlin hill;
    noise::module::Billow highLand;
    noise::module::RidgedMulti mountain;

    noise::module::ScaleBias oceanScale;
    noise::module::ScaleBias flatLandScale;
    noise::module::ScaleBias hillScale;
    noise::module::ScaleBias highLandScale;
    noise::module::ScaleBias mountainScale;

    noise::module::Select oceanFlatSelect;
    noise::module::Select hillSelect;
    noise::module::Select highLandSelect;
    noise::module::Select mountainSelect;

    //noise::module::RidgedMulti river;
    //noise::module::ScaleBias riverScale;
    //noise::module::Const riverConst;
    //noise::module::Blend riverBlend;

   /*noise::module::RidgedMulti mountain;
    noise::module::Perlin mountainType;
    //noise::module::Select mountainSelect;
    noise::module::RidgedMulti moutainVally;
    noise::module::ScaleBias mountainVallyScale;
    noise::module::Const mountainVallyConst;
    noise::module::Blend blendMountainVally;

    //noise::module::Multiply multMoutain;
    //noise::module::Perlin mountainCliff;


    noise::module::Turbulence mountainTurb;
    noise::module::ScaleBias mountainScale;

    noise::module::Billow flatLands;
    noise::module::ScaleBias flatScale;

    noise::module::Perlin ocean;
    noise::module::ScaleBias oceanScale;

    noise::module::Perlin oceanAndFlatType;
    noise::module::Select oceanAndFlatSelector;
    noise::module::Perlin finalType;
    noise::module::Select finalSelector;*/

    long noiseSeed;
    public:

    int32_t (&noiseWrite(int32_t(&heightMap)[16][16], int32_t cX,int32_t cZ))[16][16];//not to sure about this

    bool noiseInit(long seed);
  };
}
