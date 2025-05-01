#pragma once

#include "opengl/mesh.h"
#include "opengl/canvas.h"
#include "core/randomization.h"
#include "generation/fractals.h"

void GenerateLeaf(Canvas2D& leafCanvas, GLTriangleMesh& leafMesh);
void GenerateFlower(Canvas2D& flowerCanvas, GLTriangleMesh& flowerMesh);
void GenerateNewTree(GLLine& skeletonLines, GLTriangleMesh& branchMeshes, GLTriangleMesh& crownLeavesMeshes, GLTriangleMesh& crownFlowersMeshes, const GLTriangleMesh& leafMesh, const GLTriangleMesh& flowerMesh, UniformRandomGenerator& uniformGenerator, int treeIterations, int treeSubdivisions, bool showFlowers);