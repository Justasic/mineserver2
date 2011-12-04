/*This is the main file for generate maps
the generation will start from here and it
should keep everything together*/
#include <vector>

#include <boost/shared_ptr.hpp>

#include <mineserver/world/generator/terrain.h>
#include <mineserver/world/chunk.h>
#include <mineserver/world/generator.h>

namespace Mineserver
{
  class mapGen: public Mineserver::World_Generator
  {
    protected:
    int32_t heightMap [16][16];
    long randFromSeed();
    Mineserver::World_Chunk::pointer_t chunk;

    public:

    typedef boost::shared_ptr<worldGeneration::terrainGen> terrain_t;
    typedef boost::shared_ptr<Mineserver::World_Generator> pointer_t;

    std::vector <terrain_t> terrain_v;

    //chunkX and chunkZ are for libnoise so i can generate the landscape in the right place
    bool processChunk(Mineserver::World_Chunk::pointer_t tempChunk,int32_t chunkX, int32_t chunkZ);
    void init(long seed);

    void writeLandscape();
  };
}
