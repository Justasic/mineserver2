/*
 * Copyright (c) 2011-2012, The Mineserver Project
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright
 *  notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 *  notice, this list of conditions and the following disclaimer in the
 *  documentation and/or other materials provided with the distribution.
 * Neither the name of the The Mineserver Project nor the
 *  names of its contributors may be used to endorse or promote products
 *  derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
