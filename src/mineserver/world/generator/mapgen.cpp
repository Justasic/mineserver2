/*This is the main file for generate maps
the generation will start from here and it
should keep everything together

I will soon also add flatlands back
and have it optional.

Also i hope to have this generation multithreaded
*/

#include <mineserver/world/chunk.h>
#include <mineserver/world/generator/mapgen.h>

using namespace Mineserver;

void mapGen::initialize(long tempSeed)
{
  seed = tempSeed;
  long realSeed = tempSeed;
}

long mapGen::randFromSeed()
{
  seed = (214013 * seed + 2531011);
  return (seed >> 16) & 0x7FFF;
}

bool mapGen::processChunk(Mineserver::World_Chunk::pointer_t chunk)
{
  uint8_t blockType;

  for (uint8_t y=0;y<=127;++y) {
    switch (y)
    {
      case 0:
        blockType = 0x07; // Bedrock
        break;
      case 1:
        blockType = 0x01; // Stone
        break;
      case 54:
        blockType = 0x03; // Dirt
        break;
      case 59:
        blockType = 0x02; // Grass
        break;
      case 60:
        blockType = 0x00; // Air
        break;
    }

    for (uint8_t x=0;x<=15;++x) {
      for (uint8_t z=0;z<=15;++z) {
        chunk->setBlockType(x, y, z, blockType);
        chunk->setBlockMeta(x, y, z, 0);
      }
    }

  }
  return true;

}
