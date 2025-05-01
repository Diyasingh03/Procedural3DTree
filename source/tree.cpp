#include "tree.h"

void GenerateLeaf(Canvas2D & leafCanvas, GLTriangleMesh& leafMesh)
{
	/*
		Leaf texture
	*/
	int leafTextureSize = leafCanvas.GetTexture()->width;
	Color leafFillColor{ 0,200,0,0 };
	Color leafLineColor{ 0,100,0,255 };

	leafCanvas.Fill(leafFillColor);
	std::vector<glm::fvec3> leafHull;
	DrawFractalLeaf(
		leafHull,
		leafCanvas,
		leafLineColor,
		6,
		1.0f,
		glm::fvec2(leafTextureSize*0.5, leafTextureSize),
		90
	);

	glm::fvec2 previous = leafHull[0];
	for (int i = 1; i < leafHull.size(); i++)
	{
		leafCanvas.DrawLine(leafHull[i - 1], leafHull[i], leafLineColor);
	}
	leafCanvas.DrawLine(leafHull.back(), leafHull[0], leafLineColor);
	leafCanvas.GetTexture()->CopyToGPU();


	/*
		Create leaf mesh by converting turtle graphics to vertices and UV coordinates.
	*/
	glm::fvec3 leafNormal{ 0.0f, 0.0f, 1.0f };

	// Normalize leafHull dimensions to [0, 1.0]
	for (auto& h : leafHull)
	{
		h = h / float(leafTextureSize);
	}
	for (glm::fvec3& p : leafHull)
	{
		leafMesh.AddVertex(
			{ p.x - 0.5f, p.z, 1.0f - p.y }, // convert to world coordinate system (tip points towards x+, face towards z+)
			leafNormal,
			{ 1.0f, 0.0f, 0.0f, 1.0f }, // vertex color
			{ p.x, p.y, 0.0f, 0.0f }	// texture coordinate
		);
	}
	for (int i = 1; i < leafHull.size() - 1; i++)
	{
		leafMesh.DefineNewTriangle(0, i, i + 1);
	}
	leafMesh.ApplyMatrix(glm::scale(glm::mat4{ 1.0f }, glm::fvec3{ 0.5f }));
	leafMesh.SendToGPU();
}

void GenerateFlower(Canvas2D& flowerCanvas, GLTriangleMesh& flowerMesh)
{
	/*
		Flower texture
	*/
	int flowerTextureSize = flowerCanvas.GetTexture()->width;
	Color flowerFillColor{ 255, 255, 255, 255 };
	Color flowerLineColor{ 255, 255, 255, 255 };

	flowerCanvas.Fill(flowerFillColor);
	
	// Draw flower petals
	int numPetals = 8;  // Increased number of petals
	float petalLength = flowerTextureSize * 0.4f;
	glm::fvec2 center(flowerTextureSize * 0.5f, flowerTextureSize * 0.5f);
	
	// Create petal shape using hull points
	std::vector<glm::fvec3> petalHull;
	petalHull.push_back(glm::fvec3(0.0f, 0.0f, 0.0f));  // base
	petalHull.push_back(glm::fvec3(-0.2f, 0.3f, 0.0f));  // left control
	petalHull.push_back(glm::fvec3(0.0f, 0.8f, 0.0f));   // tip
	petalHull.push_back(glm::fvec3(0.2f, 0.3f, 0.0f));   // right control
	
	// Scale petal points
	for (auto& p : petalHull) {
		p *= petalLength;
		p.x += flowerTextureSize * 0.5f;
		p.y = flowerTextureSize - p.y;  // Flip Y coordinate
	}

	// Draw the petal outline
	for (int i = 0; i < petalHull.size(); i++) {
		int next = (i + 1) % petalHull.size();
		flowerCanvas.DrawLine(petalHull[i], petalHull[next], flowerLineColor);
	}

	flowerCanvas.GetTexture()->CopyToGPU();

	/*
		Create flower mesh with 3D petal arrangement
	*/
	glm::fvec3 flowerNormal{ 0.0f, 0.0f, 1.0f };
	
	// Normalize petal hull dimensions to [0, 1.0]
	std::vector<glm::fvec3> normalizedHull = petalHull;
	for (auto& p : normalizedHull) {
		p = p / float(flowerTextureSize);
	}

	// Create a single petal
	GLTriangleMesh basePetal;
	for (glm::fvec3& p : normalizedHull) {
		float gradient = p.y; // Assuming Y is normalized to [0, 1]
		glm::fvec4 vertexColor = glm::mix(
			glm::fvec4(0.5f, 0.2f, 0.2f, 1.0f), // Darker color for the base
			glm::fvec4(1.0f, 1.0f, 1.0f, 1.0f), // Lighter color for the tip
			gradient
		);
		basePetal.AddVertex(
			{ p.x - 0.5f, p.z, 1.0f - p.y },  // convert to world coordinate system
			flowerNormal,
			vertexColor,  // vertex color
			{ p.x, p.y, 0.0f, 0.0f }      // texture coordinate
		);
	}

	// Create triangles for the petal
	for (int i = 1; i < normalizedHull.size() - 1; i++) {
		basePetal.DefineNewTriangle(0, i, i + 1);
	}

	// Create multiple layers of petals with varying orientations
	for (int layer = 0; layer < 3; layer++) {
		float layerScale = 1.0f - (layer * 0.15f);  // Each layer is slightly smaller
		float layerHeight = layer * 0.05f;  // Each layer is slightly higher
		
		for (int i = 0; i < numPetals; i++) {
			float angle = (360.0f / numPetals) * i;
			
			// Create a more 3D arrangement by varying the petal orientation
			glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, layerHeight, 0.0f));
			
			// Rotate around Y axis for petal position
			transform = glm::rotate(transform, glm::radians(angle), glm::vec3(0.0f, 1.0f, 0.0f));
			
			// Tilt petals outward based on layer and position
			float tiltAngle = 30.0f + (layer * 15.0f);  // Base tilt plus layer-based tilt
			float tiltVariation = 10.0f * sin(angle * 2.0f);  // Add some variation
			transform = glm::rotate(transform, glm::radians(tiltAngle + tiltVariation), glm::vec3(1.0f, 0.0f, 0.0f));
			
			// Scale the petal
			transform = glm::scale(transform, glm::vec3(layerScale));
			
			flowerMesh.AppendMeshTransformed(basePetal, transform);
		}
	}

	// Scale the entire flower
	flowerMesh.ApplyMatrix(glm::scale(glm::mat4{ 1.0f }, glm::fvec3{ 0.5f }));
	flowerMesh.SendToGPU();
}

void GenerateNewTree(GLLine& skeletonLines, GLTriangleMesh& branchMeshes, GLTriangleMesh& crownLeavesMeshes, GLTriangleMesh& crownFlowersMeshes, const GLTriangleMesh& leafMesh, const GLTriangleMesh& flowerMesh, UniformRandomGenerator& uniformGenerator, int treeIterations, int treeSubdivisions, bool showFlowers)
{
	skeletonLines.Clear();
	branchMeshes.Clear();
	crownLeavesMeshes.Clear();
	crownFlowersMeshes.Clear();

	/*
		Tree branch propertes
	*/
	float trunkThickness = 0.5f * powf(1.3f, float(treeIterations));
	float branchScalar = 0.4f;											// how the branch thickness relates to the parent
	float depthScalar = powf(0.75f, 1.0f / float(treeSubdivisions));	// how much the branch shrinks in thickness the farther from the root it goes (the pow is to counter the subdiv growth)
	const int trunkCylinderDivisions = 32;

	/*
		Leaf generation properties
	*/
	float leafMinScale = 0.25f;
	float leafMaxScale = 1.5f;
	float growthCurve = treeIterations / (1.0f + float(treeIterations));
	float pruningChance = growthCurve * 2.0f - 1.0f;// Random chance to remove a leaf (chance increases by the number of iterations)

	int leavesPerBranch = 25 - int(20 * (growthCurve * 2.0f - 1.0f));
	leavesPerBranch = (leavesPerBranch == 0) ? 1 : leavesPerBranch;

	/*
		Helper functions
	*/
	auto& getBranchThickness = [&](int branchDepth, int nodeDepth) -> float
	{
		return trunkThickness * powf(branchScalar, float(branchDepth)) * powf(depthScalar, float(nodeDepth));
	};

	auto& getCylinderDivisions = [&](int branchDepth) -> int
	{
		int cylinderDivisions = int(trunkCylinderDivisions / pow(2, branchDepth));
		return (cylinderDivisions < 4) ? 6 : cylinderDivisions;
	};

	int branchCount = 0;
	GenerateFractalTree3D(
		TreeStyle::Default,
		uniformGenerator,
		treeIterations,
		treeSubdivisions,
		1.0f, // applyRandomness
		[&](Bone<FractalTree3DProps>* root, std::vector<FractalBranch>& branches) -> void
	{
		if (!root) return;
		using TBone = Bone<FractalTree3DProps>;

		for (int b = 0; b < branches.size(); b++)
		{
			int cylinderDivisions = getCylinderDivisions(branches[b].depth);
			GLTriangleMesh newBranchMesh{ false };

			/*
				Vertex
				Positions, Normals, Texture Coordinates
			*/
			// Create vertex rings around each bone
			auto& branchNodes = branches[b].nodes;
			float rootLength = branchNodes[0]->length;
			float texU = 0.0f; // Texture coordinate along branch, it varies depending on the bone length and must be tracked
			for (int depth = 0; depth < branchNodes.size(); depth++)
			{
				auto& bone = branchNodes[depth];
				float thickness = getBranchThickness(branches[b].depth, bone->nodeDepth);
				float circumference = 2.0f*PI_f*thickness;
				texU += bone->length / circumference;

				skeletonLines.AddLine(bone->transform.position, bone->tipPosition(), glm::fvec4(0.0f, 1.0f, 0.0f, 1.0f));
				skeletonLines.AddLine(bone->transform.position, bone->transform.position+bone->transform.up*0.2f, glm::fvec4(1.0f, 0.0f, 0.0f, 1.0f));

				glm::fvec3 localX = bone->transform.up;
				glm::fvec3 localY = bone->transform.forward;

				// Make the branch root blend into its parent a bit. (this makes the branches appear less angular)
				auto& t = bone->transform;
				glm::fvec3 position = t.position;
				if (depth < (treeSubdivisions - 1) && branchNodes[0]->parent)
				{
					auto& parent = branchNodes[0]->parent;
					float blendAlpha = depth / float(treeSubdivisions);

					glm::fvec3 u = parent->transform.forward;
					glm::fvec3 v = bone->transform.position - parent->transform.position;
					float length = glm::length(v);
					v /= length;
					glm::fvec3 projectionOnParent = parent->transform.position + glm::dot(u, v) * u * length * blendAlpha;

					position = glm::mix(projectionOnParent, t.position, 0.5f + 0.5f*blendAlpha);
					thickness = glm::mix(thickness / branchScalar, thickness, 0.4f + 0.6f*blendAlpha);

					// Blend orientation of cylinder ring to give a spline
					auto& parentForward = parent->transform.forward;
					auto& boneForward = bone->transform.forward;
					localY = glm::normalize(glm::mix(parentForward, boneForward, blendAlpha));
					glm::fvec3 rotationVector = glm::normalize(glm::cross(boneForward, localY));
					float angle = glm::acos(glm::dot(boneForward, localY));
					localX = glm::rotate(glm::mat4(1.0f), angle, rotationVector) * glm::fvec4(localX, 0.0f);
				}

				// Generate the cylinder ring
				float angleStep = 360.0f / float(cylinderDivisions);
				for (int i = 0; i < cylinderDivisions; i++)
				{
					float angle = angleStep * i;
					glm::mat4 rot = glm::rotate(glm::mat4{ 1.0f }, glm::radians(angle), localY);
					glm::fvec3 normal = rot * glm::fvec4(localX, 0.0f);

					newBranchMesh.AddVertex(
						position + normal * thickness,
						normal,
						glm::fvec4{ 1.0f },
						glm::fvec4{ texU, i / float(cylinderDivisions), 1.0f, 1.0f }
					);
				}

				// Add extra set of vertices for the UV seam
				newBranchMesh.AddVertex(
					position + localX * thickness,
					localX,
					glm::fvec4{ 1.0f },
					glm::fvec4{ texU, 1.0f, 1.0f, 1.0f }
				);
			}

			// Add tip for branch
			auto& lastBone = branchNodes.back();
			newBranchMesh.AddVertex(
				lastBone->tipPosition(),
				lastBone->transform.forward,
				glm::fvec4{ 1.0f },
				glm::fvec4{ texU + lastBone->length, 0.5f, 1.0f, 1.0f }
			);

			/*
				Triangle Indices
			*/
			// Generate indices for cylinders
			int ringStep = cylinderDivisions + 1; // +1 because of UV seam
			for (int depth = 1; depth < branchNodes.size(); depth++)
			{
				int uStart = depth * ringStep;
				int lStart = uStart - ringStep;

				for (int i = 0; i < cylinderDivisions; i++)
				{
					int u = uStart + i;
					int l = lStart + i;

					newBranchMesh.DefineNewTriangle(l, l + 1, u + 1);
					newBranchMesh.DefineNewTriangle(u + 1, u, l);
				}
			}

			// Generate indices for tip
			int tipIndex = int(newBranchMesh.positions.size()) - 1;
			int lastRing = ringStep * (int(branchNodes.size()) - 1);
			for (int i = 1; i < ringStep; i++)
			{
				int ringId = lastRing + i;
				newBranchMesh.DefineNewTriangle(ringId - 1, ringId, tipIndex);
			}

			branchMeshes.AppendMesh(newBranchMesh);
		}

		/*
			Generate leaves
		*/
		int maxBranchDepth = 0;
		for (auto& branch : branches)
		{
			maxBranchDepth = (branch.depth > maxBranchDepth) ? branch.depth : maxBranchDepth;
		}

		int startDepth = maxBranchDepth - 2;
		startDepth = (startDepth > 2) ? startDepth : 2;

		for (auto& branch : branches)
		{
			if (branch.depth < startDepth) continue;
			auto& branchNodes = branch.nodes;
			int lastIndex = int(branchNodes.size() - 1);
			int startIndex = int(round(0.25f * lastIndex));
			for (int i = startIndex; i <= lastIndex; ++i)
			{
				auto& leafNode = branchNodes[i];

				glm::fvec3 nodeBegin = leafNode->transform.position;
				glm::fvec3 nodeEnd = leafNode->tipPosition();
				glm::fvec3 nodeDirection = leafNode->transform.forward;
				glm::fvec3 nodeNormal = leafNode->transform.up;

				float thickness = getBranchThickness(branch.depth, leafNode->nodeDepth);
				float circumference = 2.0f*PI_f*thickness;

				int leafId = leavesPerBranch;
				float stepSize = leafNode->length / leavesPerBranch;
				glm::fvec3 position, direction, normal;
				while (leafId > 0)
				{
					leafId--;
					if (uniformGenerator.RandomFloat() < pruningChance) continue;

					// Compute the leaf placement
					position = nodeBegin + nodeDirection * (stepSize*leafId + uniformGenerator.RandomFloat(0.0f, stepSize / 2.0f));	// spread along branch
					float angle = uniformGenerator.RandomFloat(0.0f, 2.0f*PI_f);
					direction = glm::rotate(glm::mat4{ 1.0f }, angle, nodeDirection) * glm::fvec4{ nodeNormal, 1.0f };				// random direction
					position += direction * thickness;																				// push leaf so that it starts on the branch and not inside it
					direction = glm::normalize(glm::mix(direction, nodeDirection, uniformGenerator.RandomFloat(0.3f, 0.8f)));		// blend how much the leaf is angled along the branch
					angle = uniformGenerator.RandomFloat(0.0f, 22.0f* PI_f);
					normal = glm::rotate(glm::mat4{ 1.0f }, angle, direction) * glm::fvec4{ nodeDirection, 1.0f };					// random twist

					// Insert the leaf
					crownLeavesMeshes.AppendMeshTransformed(
						leafMesh,
						glm::inverse(glm::lookAt(position, position - direction, -normal)) * glm::scale(glm::mat4{ 1.0f }, glm::fvec3{ uniformGenerator.RandomFloat(leafMinScale, leafMaxScale) })
					);
				}

				// Put a leaf at the tip of the branch
				if (i == lastIndex)
				{
					crownLeavesMeshes.AppendMeshTransformed(
						leafMesh,
						glm::inverse(glm::lookAt(nodeEnd, nodeEnd - nodeDirection, -nodeNormal)) * glm::scale(glm::mat4{ 1.0f }, glm::fvec3{ uniformGenerator.RandomFloat(leafMinScale, leafMaxScale) })
					);
				}
			}
		}

		// Add flowers to the tree
		if (showFlowers)
		{
			// Add flowers to some of the branch tips
			for (int b = 0; b < branches.size(); b++)
			{
				if (branches[b].depth > 1)
				{
					auto& branchNodes = branches[b].nodes;
					int lastIndex = int(branchNodes.size() - 1);
					
					// Calculate how many flowers to add based on branch depth and length
					int numFlowers = 1 + int(branches[b].depth * 0.5f);  // More flowers for deeper branches
					
					// Add flowers starting from the last node and moving backwards
					for (int i = 0; i < numFlowers; i++)
					{
						// Only add flowers if random chance succeeds
						if (uniformGenerator.RandomFloat() < 0.4f)
						{
							// Calculate position along the branch
							int nodeIndex = lastIndex - i;
							if (nodeIndex < 0) break;  // Stop if we've gone too far back
							
							auto& node = branchNodes[nodeIndex];
							
							// Calculate position slightly away from the branch
							glm::vec3 branchDirection = node->transform.forward;
							glm::vec3 branchNormal = node->transform.up;
							glm::vec3 offsetDirection = glm::normalize(glm::cross(branchDirection, branchNormal));
							
							// Position the flower slightly away from the branch
							//glm::vec3 flowerPosition = node->tipPosition() + offsetDirection * 0.1f;  // 0.1 units away from branch
							glm::vec3 flowerPosition = node->tipPosition();

							/*glm::mat4 flowerTransform = glm::translate(glm::mat4(1.0f), flowerPosition);
							
							// Add a single flower with random orientation
							float randomAngle = uniformGenerator.RandomFloat(-30.0f, 30.0f);
							flowerTransform = glm::rotate(flowerTransform, glm::radians(randomAngle), glm::vec3(0.0f, 1.0f, 0.0f));
							
							// Add slight random offset to position
							float offset = 0.05f * uniformGenerator.RandomFloat(-1.0f, 1.0f);
							flowerTransform = glm::translate(flowerTransform, glm::vec3(offset, 0.0f, offset));
							*/
							// Apply a random tilt angle (up or down)
							float tiltAngle = uniformGenerator.RandomFloat(-30.0f, 30.0f); // Random tilt between -30° and 30°
							glm::mat4 tiltTransform = glm::rotate(glm::mat4(1.0f), glm::radians(tiltAngle), branchNormal);

							// Create the transformation matrix for the flower
							glm::mat4 flowerTransform = glm::inverse(glm::lookAt(
								flowerPosition,                          // Flower position
								flowerPosition + branchDirection,        // Look in the direction of the branch
								branchNormal                             // Use the branch's normal as the "up" vector
							));

							// Apply the tilt transformation
							flowerTransform = flowerTransform * tiltTransform;

							// Create a transformation matrix to orient the flower
							/*glm::mat4 flowerTransform = glm::inverse(glm::lookAt(
								flowerPosition,                          // Flower position
								flowerPosition + branchDirection,        // Look in the direction of the branch
								branchNormal                             // Use the branch's normal as the "up" vector
							));

							// Add slight random rotation around the branch's forward direction
							float randomAngle = uniformGenerator.RandomFloat(0.0f, 360.0f);
							flowerTransform = glm::rotate(flowerTransform, glm::radians(randomAngle), branchDirection);

							// Add slight random offset to position
							float offset = 0.05f * uniformGenerator.RandomFloat(-1.0f, 1.0f);
							flowerTransform = glm::translate(flowerTransform, glm::vec3(offset, 0.0f, offset));*/
							crownFlowersMeshes.AppendMeshTransformed(flowerMesh, flowerTransform);
						}
					}
				}
			}
		}
	});

	skeletonLines.SendToGPU();
	branchMeshes.SendToGPU();
	crownLeavesMeshes.SendToGPU();
	crownFlowersMeshes.SendToGPU();
}

