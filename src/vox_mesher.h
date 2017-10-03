#pragma once
#ifndef _VOX_MESHER_H_
#define _VOX_MESHER_H_

#include "vox_render.h"
#include "vox_world.h"

void meshVanillaNaive(Chunk *chunk, ChunkMesh *mesh);
void meshVanillaCull(Chunk *chunk, ChunkMesh *mesh);
void meshVanillaGreedy(Chunk *chunk, ChunkMesh *mesh);
void addGreedyJob(Chunk *chunk, ChunkMesh *mesh);

// TODO: GROSS
ChunkMesh **finishedMeshes = 0;
int numFinishedMeshes = 0;

#endif // _VOX_MESHER_H_

