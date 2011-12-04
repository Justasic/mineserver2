/*This will generate all the terrain for
the map. This includes mountains and flats
ect. I am going to use libnoise for now.*/

#include <noise/noise.h>
#include <boost/shared_ptr.hpp>

namespace worldGeneration
{
  class terrainGen
  {
    noise::module::RidgedMulti mountainTerrain;
    noise::module::ScaleBias mountainScale;
    noise::module::Billow baseFlatTerrain;
    noise::module::ScaleBias flatTerrain;
    noise::module::Perlin terrainType;
    noise::module::Select terrainSelector;
    noise::module::ScaleBias finalTerrain;

    long noiseSeed;
    public:

    int32_t (&noiseWrite(int32_t(&heightMap)[16][16], int32_t cX,int32_t cZ))[16][16];//not to sure about this

    bool noiseInit(long seed);
  };
}
