/*This will generate all the terrain for
the map. This includes mountains and flats
ect. I am going to use libnoise for now.*/

#include <mineserver/world/generator/terrain.h>

bool worldGeneration::terrainGen::noiseInit(long seed)
{
  seed = 21;
  mountainTerrain.SetSeed(seed);
  mountainTerrain.SetFrequency(0.005);
  mountainTerrain.SetOctaveCount(5);

  mountainScale.SetSourceModule(0, mountainTerrain);
  mountainScale.SetScale(-1.0);
  mountainScale.SetBias(- (1.0 / 128) * 13);

  baseFlatTerrain.SetSeed(seed);
  baseFlatTerrain.SetFrequency(0.005);
  baseFlatTerrain.SetOctaveCount(5);
  baseFlatTerrain.SetPersistence(0.5);

  flatTerrain.SetSourceModule(0, baseFlatTerrain);
  flatTerrain.SetScale(-1.0);
  flatTerrain.SetBias(- (1.0 / 128) * 13);

  terrainType.SetSeed(seed);
  terrainType.SetFrequency(0.005);
  terrainType.SetOctaveCount(4);
  terrainType.SetPersistence(0.5);

  terrainSelector.SetSourceModule(0, flatTerrain);
  terrainSelector.SetSourceModule(1, mountainScale);
  terrainSelector.SetControlModule(terrainType);
  terrainSelector.SetBounds(0.5, 1000.0);
  terrainSelector.SetEdgeFalloff(0.125);

  finalTerrain.SetSourceModule(0, terrainSelector);
  finalTerrain.SetScale(62);
  finalTerrain.SetBias(62);

  return true;
}

int32_t (&worldGeneration::terrainGen::noiseWrite( int32_t(&heightMap)[16][16], int32_t cX,int32_t cZ))[16][16]
{

  int32_t xBlockpos = cX << 4;
  int32_t zBlockpos = cZ << 4;

  for(int32_t x=0;x<16;x++)
  {
    for(int32_t z=0;z<16;z++)
    {
      heightMap[x][z] = (int32_t)(finalTerrain.GetValue(xBlockpos +x, 0, zBlockpos + z));
    }
  }
  return heightMap;
}
