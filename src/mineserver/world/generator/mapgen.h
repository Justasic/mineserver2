/*This is the main file for generate maps
the generation will start from here and it
should keep everything together*/

//TODO

#include <boost/shared_ptr.hpp>

#include <mineserver/world/chunk.h>
#include <mineserver/world/generator.h>

namespace Mineserver
{
  class mapGen: public Mineserver::World_Generator
  {
    long seed;
    long realSeed;
    public:

    typedef boost::shared_ptr<Mineserver::World_Generator> pointer_t;

    void initialize(long tempSeed);
    long randFromSeed();
    bool processChunk(Mineserver::World_Chunk::pointer_t chunk);
  };
}
