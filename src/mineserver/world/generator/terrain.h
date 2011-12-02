/*This will generate all the terrain for
the map. This includes mountains and flats
ect. I am going to use libnoise for now.*/

#include <boost/shared_ptr.hpp>

namespace Mineserver
{
  class terrainGen
  {
    long noiseSeed;
    public:
    typedef boost::shared_ptr<Mineserver::terrainGen> pointer_t;

    bool init(long tempSeed);
  };
}
