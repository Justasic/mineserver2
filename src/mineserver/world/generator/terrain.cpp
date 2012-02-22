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

/*This will generate all the terrain for
the map. This includes mountains and flats
ect. I am going to use libnoise for now.*/

#include <mineserver/world/generator/terrain.h>

bool worldGeneration::terrainGen::noiseInit(long seed)
{
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
  flatTerrain.SetScale(0.125);
  flatTerrain.SetBias(0.05);

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
