# Ozz-animation sample: Skinning

## Description

This sample adds skinning to the playback sample. It loads a skeleton, an animation and skinned meshes from ozz binary archives. It playbacks animation every frame and uses model-space matrices to build skinning matrices and render a skinned mesh.

## Concept

This sample is based on playback sample for the most part, reading and sampling an animation. In addition it loads an ozz binary mesh file which was generated with sample_fbx2mesh tool.
At every frame, once the animation is sampled and model-space matrices computed, skinning matrices are built. Skinning matrices are the composition (multiplication) of model-space and mesh inverse bind pose matrices. This step can be seen as allowing to transform vertices into joint's local-space, so they can be transformed by this joint.

## Sample usage

Animation playback parameters can be tuned from sample UI:
- Play/pause animation.
- Fix animation time.
- Set playback speed, which can be negative to go backward.

Rendering options are also exposed:
- Enabling skeleton display.
- Enabling mesh display.
- Enabling skinning stage.
- Display normals, tangent and binormals.

## Implementation

1. Load animation and skeleton, sample animation to get local-space transformations, and finally convert local-space transformations to model-space matrices. See Playback sample for more details about these steps.
2. Load meshes from ozz archive. There can be multiple meshes as import utility (aka fbx2mesh) maintains dcc file meshes split.
3. Computes and allocates skinning matrices. Number of skinning matrices might be less from the number of joints, as a mesh might be skinned by a subset of all skeleton joints only. Mesh::joint_remaps is used to know how to order skinning matrices, hence is size defines their number.   
4. Skinning matrices array is updated before rendering each mesh. A skinning matrix is the multiplication of the model-space and the mesh inverse bind pose matrix for a joint. Mesh::joint_remaps is used to index skeleton joints, so they match with the mesh.
5. Skinning is performed by ozz::geometry::SkinningJob. Please check sample framework DrawSkinnedMesh function for more details about how to setup that job.