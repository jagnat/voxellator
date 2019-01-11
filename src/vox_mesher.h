#pragma once
#ifndef _VOX_MESHER_H_
#define _VOX_MESHER_H_

#include "vox_render.h"
#include "vox_world.h"

void meshVanillaNaive(Chunk *chunk);
void meshVanillaCull(Chunk *chunk);
void meshVanillaGreedy(Chunk *chunk);
void addGreedyJob(Chunk *chunk);

#endif // _VOX_MESHER_H_
