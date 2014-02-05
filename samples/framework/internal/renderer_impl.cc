//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2014 Guillaume Blanc                                    //
//                                                                            //
// This software is provided 'as-is', without any express or implied          //
// warranty. In no event will the authors be held liable for any damages      //
// arising from the use of this software.                                     //
//                                                                            //
// Permission is granted to anyone to use this software for any purpose,      //
// including commercial applications, and to alter it and redistribute it     //
// freely, subject to the following restrictions:                             //
//                                                                            //
// 1. The origin of this software must not be misrepresented; you must not    //
// claim that you wrote the original software. If you use this software       //
// in a product, an acknowledgment in the product documentation would be      //
// appreciated but is not required.                                           //
//                                                                            //
// 2. Altered source versions must be plainly marked as such, and must not be //
// misrepresented as being the original software.                             //
//                                                                            //
// 3. This notice may not be removed or altered from any source               //
// distribution.                                                              //
//                                                                            //
//============================================================================//

#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.

#include "framework/internal/renderer_impl.h"

#include "ozz/animation/runtime/local_to_model_job.h"
#include "ozz/animation/runtime/skeleton.h"

#include "ozz/base/log.h"

#include "ozz/base/platform.h"

#include "ozz/base/maths/math_ex.h"
#include "ozz/base/maths/simd_math.h"
#include "ozz/base/maths/vec_float.h"
#include "ozz/base/maths/box.h"

#include "ozz/base/memory/allocator.h"

namespace ozz {
namespace sample {
namespace internal {

namespace {
// A vertex made of positions and normals.
struct VertexPNC {
  math::Float3 pos;
  math::Float3 normal;
  Renderer::Color color;
};
}  // namespace

RendererImpl::Model::Model()
    : vbo(0),
      mode(GL_POINTS),
      count(0),
      shader(NULL) {
}

RendererImpl::Model::~Model() {
  if (vbo) {
    GL(DeleteBuffers(1, &vbo));
    vbo = 0;
  }
  if (shader) {
    memory::default_allocator()->Delete(shader);
    shader = NULL;
  }
}

RendererImpl::RendererImpl()
    : max_skeleton_pieces_(animation::Skeleton::kMaxJoints * 2),
      prealloc_uniforms_(NULL),
      joint_instance_vbo_(0) {
  prealloc_uniforms_ = reinterpret_cast<float*>(
    memory::default_allocator()->Allocate(
      max_skeleton_pieces_ * 16 * sizeof(float),
      AlignOf<math::SimdFloat4>::value));
}

RendererImpl::~RendererImpl() {
  memory::default_allocator()->Deallocate(prealloc_models_);

  GL(DeleteBuffers(1, &joint_instance_vbo_));
  joint_instance_vbo_ = 0;

  memory::default_allocator()->Deallocate(prealloc_uniforms_);
  prealloc_uniforms_ = NULL;
}

bool RendererImpl::Initialize() {
  if (!InitOpenGLExtensions()) {
    return false;
  }
  if (!InitPostureRendering()) {
    return false;
  }

  // Builds the dynamic vbo
  GL(GenBuffers(1, &joint_instance_vbo_));

  return true;
}

namespace {

const char* shader_uber_vs =
  "varying vec3 world_normal;\n"
  "varying vec4 vertex_color;\n"
  "void main() {\n"
  "  mat4 world_matrix = GetWorldMatrix();\n"
  "  vec4 vertex = vec4(gl_Vertex.xyz, 1.);\n"
  "  gl_Position = gl_ModelViewProjectionMatrix * world_matrix * vertex;\n"
  "  mat3 cross_matrix = mat3(\n"
  "    cross(world_matrix[1].xyz, world_matrix[2].xyz),\n"
  "    cross(world_matrix[2].xyz, world_matrix[0].xyz),\n"
  "    cross(world_matrix[0].xyz, world_matrix[1].xyz));\n"
  "  float invdet = 1.0 / dot(cross_matrix[2], world_matrix[2].xyz);\n"
  "  mat3 normal_matrix = cross_matrix * invdet;\n"
  "  world_normal = normal_matrix * gl_Normal;\n"
  "  vertex_color = gl_Color;\n"
  "}\n";
const char* shader_ambient_fs =
  "vec3 lerp(in vec3 alpha, in vec3 a, in vec3 b) {\n"
  "  return a + alpha * (b - a);\n"
  "}\n"
  "vec4 lerp(in vec4 alpha, in vec4 a, in vec4 b) {\n"
  "  return a + alpha * (b - a);\n"
  "}\n"
  "varying vec3 world_normal;\n"
  "varying vec4 vertex_color;\n"
  "void main() {\n"
  "  vec3 normal = normalize(world_normal);\n"
  "  vec3 alpha = (normal + 1.) * .5;\n"
  "  vec4 bt = lerp(\n"
  "    alpha.xzxz, vec4(.3, .3, .7, .7), vec4(.4, .4, .8, .8));\n"
  "  gl_FragColor = vec4(\n"
  "     lerp(alpha.yyy, vec3(bt.x, .3, bt.y), vec3(bt.z, .8, bt.w)), 1.);\n"
  "  gl_FragColor *= vertex_color;\n"
  "}\n";

Shader* InitJointShading() {
  // Builds a world matrix from joint uniforms, a joint matrix scaled by bone
  // length.
  const char* vs_joint_to_world_matrix =
    "mat4 GetWorldMatrix() {\n"
    "  // Rebuilds joint matrix.\n"
    "  mat4 joint_matrix;\n"
    "  joint_matrix[0] = vec4(normalize(joint[0].xyz), 0.);\n"
    "  joint_matrix[1] = vec4(normalize(joint[1].xyz), 0.);\n"
    "  joint_matrix[2] = vec4(normalize(joint[2].xyz), 0.);\n"
    "  joint_matrix[3] = vec4(joint[3].xyz, 1.);\n"

    "  // Rebuilds bone properties.\n"
    "  vec3 bone_dir = vec3(joint[0].w, joint[1].w, joint[2].w);\n"
    "  float bone_len = length(bone_dir);\n"

    "  // Setup rendering world matrix.\n"
    "  mat4 world_matrix;\n"
    "  world_matrix[0] = joint_matrix[0] * bone_len;\n"
    "  world_matrix[1] = joint_matrix[1] * bone_len;\n"
    "  world_matrix[2] = joint_matrix[2] * bone_len;\n"
    "  world_matrix[3] = joint_matrix[3];\n"
    "  return world_matrix;\n"
    "}\n";
  const char* vs[] = {
    "#version 110\n",
    GL_ARB_instanced_arrays ?
      "attribute mat4 joint;\n" :
      "uniform mat4 joint;\n",  // Simplest fall back.
    vs_joint_to_world_matrix,
    shader_uber_vs};
  const char* fs[] = {
    "#version 110\n",
    shader_ambient_fs};

  Shader* shader = Shader::Build(OZZ_ARRAY_SIZE(vs), vs,
                                 OZZ_ARRAY_SIZE(fs), fs);
  if (!shader) {
    return NULL;
  }
  if (GL_ARB_instanced_arrays &&
      !shader->BindAttrib("joint")) {
    return NULL;
  } else if (!GL_ARB_instanced_arrays &&
             !shader->BindUniform("joint")) {
    return NULL;
  }
  return shader;
}

Shader* InitBoneShading() {
  // Builds a world matrix from joint uniforms, sticking bone model between
  // parent and child joints.
  const char* vs_joint_to_world_matrix =
    "mat4 GetWorldMatrix() {\n"
    "  // Rebuilds bone properties.\n"
    "  // Bone length is set to zero to disable leaf rendering.\n"
    "  float is_bone = joint[3].w;\n"
    "  vec3 bone_dir = vec3(joint[0].w, joint[1].w, joint[2].w) * is_bone;\n"
    "  float bone_len = length(bone_dir);\n"

    "  // Setup rendering world matrix.\n"
    "  float dot = dot(joint[2].xyz, bone_dir);\n"
    "  vec3 binormal = abs(dot) < .01 ? joint[2].xyz : joint[1].xyz;\n"

    "  mat4 world_matrix;\n"
    "  world_matrix[0] = vec4(bone_dir, 0.);\n"
    "  world_matrix[1] = \n"
    "    vec4(bone_len * normalize(cross(binormal, bone_dir)), 0.);\n"
    "  world_matrix[2] =\n"
    "    vec4(bone_len * normalize(cross(bone_dir, world_matrix[1].xyz)), 0.);\n"
    "  world_matrix[3] = vec4(joint[3].xyz, 1.);\n"
    "  return world_matrix;\n"
    "}\n";
  const char* vs[] = {
    "#version 110\n",
    GL_ARB_instanced_arrays ?
      "attribute mat4 joint;\n" :
      "uniform mat4 joint;\n",  // Simplest fall back.
    vs_joint_to_world_matrix,
    shader_uber_vs};
  const char* fs[] = {
    "#version 110\n",
    shader_ambient_fs};

  Shader* shader = Shader::Build(OZZ_ARRAY_SIZE(vs), vs,
                                 OZZ_ARRAY_SIZE(fs), fs);
  if (!shader) {
    return NULL;
  }
  if (GL_ARB_instanced_arrays &&
      !shader->BindAttrib("joint")) {
    return NULL;
  } else if (!GL_ARB_instanced_arrays &&
             !shader->BindUniform("joint")) {
    return NULL;
  }
  return shader;
}
}  // namespace

void RendererImpl::DrawAxes(const ozz::math::Float4x4& _transform) {

  glPushMatrix();
  OZZ_ALIGN(16) float matrix[16];
  ozz::math::StorePtr(_transform.cols[0], matrix + 0);
  ozz::math::StorePtr(_transform.cols[1], matrix + 4);
  ozz::math::StorePtr(_transform.cols[2], matrix + 8);
  ozz::math::StorePtr(_transform.cols[3], matrix + 12);
  glMultMatrixf(matrix);

  glBegin(GL_LINES);
    glColor4ub(0xff, 0, 0, 0xff);  // X axis (red).
    glVertex3f(0.f, 0.f, 0.f);
    glVertex3f(1.f, 0.f, 0.f);
    glColor4ub(0, 0xff, 0, 0xff);  // Y axis (green).
    glVertex3f(0.f, 0.f, 0.f);
    glVertex3f(0.f, 1.f, 0.f);
    glColor4ub(0, 0, 0xff, 0xff);  // Z axis (blue).
    glVertex3f(0.f, 0.f, 0.f);
    glVertex3f(0.f, 0.f, 1.f);
  GL(End());

  glPopMatrix();
}

void RendererImpl::DrawGrid(int _cell_count, float _cell_size) {
  const float extent = _cell_count * _cell_size;
  const float half_extent = extent * 0.5f;
  const ozz::math::Float3 corner(-half_extent, 0, -half_extent);

  GL(Enable(GL_BLEND));
  GL(BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
  GL(Disable(GL_CULL_FACE));
  glBegin(GL_QUADS);
  glColor4ub(0x80, 0xc0, 0xd0, 0xb0);
  glVertex3f(corner.x, corner.y, corner.z);
  glVertex3f(corner.x, corner.y, corner.z + extent);
  glVertex3f(corner.x + extent, corner.y, corner.z + extent);
  glVertex3f(corner.x + extent, corner.y, corner.z);
  GL(End());
  GL(Disable(GL_BLEND));
  GL(Enable(GL_CULL_FACE));

  glBegin(GL_LINES);
  glColor4ub(0xb0, 0xb0, 0xb0, 0xff);
  // Renders lines along X axis.
  ozz::math::Float3 x_line_begin = corner;
  ozz::math::Float3 x_line_end(corner.x + extent, corner.y, corner.z);
  for (int i = 0; i < _cell_count + 1; i++) {
    glVertex3fv(&x_line_begin.x);
    glVertex3fv(&x_line_end.x);
    x_line_begin.z += _cell_size;
    x_line_end.z += _cell_size;
  }
  // Renders lines along Z axis.
  ozz::math::Float3 z_line_begin = corner;
  ozz::math::Float3 z_line_end(corner.x, corner.y, corner.z + extent);
  for (int i = 0; i < _cell_count + 1; i++) {
    glVertex3fv(&z_line_begin.x);
    glVertex3fv(&z_line_end.x);
    z_line_begin.x += _cell_size;
    z_line_end.x += _cell_size;
  }
  GL(End());
}

// Computes the model space bind pose and renders it.
bool RendererImpl::DrawSkeleton(const ozz::animation::Skeleton& _skeleton,
                                const ozz::math::Float4x4& _transform,
                                bool _draw_joints) {
  using ozz::math::Float4x4;

  const int num_joints = _skeleton.num_joints();
  if (!num_joints) {
    return true;
  }

  // Reallocate matrix array if necessary.
  if (prealloc_models_.Size() < num_joints) {
    ozz::memory::Allocator* allocator = ozz::memory::default_allocator();
    prealloc_models_ =
      allocator->AllocateRange<ozz::math::Float4x4>(_skeleton.num_joints());
  }
  if (!prealloc_models_.begin) {
    return false;
  }
  
  // Compute model space bind pose.
  ozz::animation::LocalToModelJob job;
  job.input = _skeleton.bind_pose();
  job.output = prealloc_models_;
  job.skeleton = &_skeleton;
  if (!job.Run()) {
    return false;
  }

  // Forwards to rendering.
  return DrawPosture(_skeleton, prealloc_models_, _transform, _draw_joints);
}

bool RendererImpl::InitPostureRendering() {
  const float kInter = .15f;
  {  // Prepares bone mesh.
    const math::Float3 pos[6] = {
      math::Float3(1.f, 0.f, 0.f),
      math::Float3(kInter, .1f, .1f),
      math::Float3(kInter, .1f, -.1f),
      math::Float3(kInter, -.1f, -.1f),
      math::Float3(kInter, -.1f, .1f),
      math::Float3(0.f, 0.f, 0.f)};
    const math::Float3 normals[8] = {
      Normalize(Cross(pos[2] - pos[1], pos[2] - pos[0])),
      Normalize(Cross(pos[1] - pos[2], pos[1] - pos[5])),
      Normalize(Cross(pos[3] - pos[2], pos[3] - pos[0])),
      Normalize(Cross(pos[2] - pos[3], pos[2] - pos[5])),
      Normalize(Cross(pos[4] - pos[3], pos[4] - pos[0])),
      Normalize(Cross(pos[3] - pos[4], pos[3] - pos[5])),
      Normalize(Cross(pos[1] - pos[4], pos[1] - pos[0])),
      Normalize(Cross(pos[4] - pos[1], pos[4] - pos[5]))};
    const Color white = {0xff, 0xff, 0xff, 0xff};
    const VertexPNC bones[24] = {
      {pos[0], normals[0], white}, {pos[2], normals[0], white},
      {pos[1], normals[0], white}, {pos[5], normals[1], white},
      {pos[1], normals[1], white}, {pos[2], normals[1], white},
      {pos[0], normals[2], white}, {pos[3], normals[2], white},
      {pos[2], normals[2], white}, {pos[5], normals[3], white},
      {pos[2], normals[3], white}, {pos[3], normals[3], white},
      {pos[0], normals[4], white}, {pos[4], normals[4], white},
      {pos[3], normals[4], white}, {pos[5], normals[5], white},
      {pos[3], normals[5], white}, {pos[4], normals[5], white},
      {pos[0], normals[6], white}, {pos[1], normals[6], white},
      {pos[4], normals[6], white}, {pos[5], normals[7], white},
      {pos[4], normals[7], white}, {pos[1], normals[7], white}};

    // Builds and fills the vbo.
    Model& bone = models_[0];
    bone.mode = GL_TRIANGLES;
    bone.count = OZZ_ARRAY_SIZE(bones);
    GL(GenBuffers(1, &bone.vbo));
    GL(BindBuffer(GL_ARRAY_BUFFER, bone.vbo));
    GL(BufferData(GL_ARRAY_BUFFER, sizeof(bones), bones, GL_STATIC_DRAW));
    GL(BindBuffer(GL_ARRAY_BUFFER, 0));  // Unbinds.

    // Init bone shader.
    bone.shader = InitBoneShading();
    if (!bone.shader) {
      return false;
    }
  }

  {  // Prepares joint mesh.
    const int kNumSlices = 20;
    const int kNumPointsPerCircle = kNumSlices + 1;
    const int kNumPointsYZ = kNumPointsPerCircle;
    const int kNumPointsXY = kNumPointsPerCircle + kNumPointsPerCircle / 4;
    const int kNumPointsXZ = kNumPointsPerCircle;
    const int kNumPoints = kNumPointsXY + kNumPointsXZ + kNumPointsYZ;
    const float kRadius = kInter;  // Radius multiplier.
    const Color red = {0xff, 0xc0, 0xc0, 0xff};
    const Color green = {0xc0, 0xff, 0xc0, 0xff};
    const Color blue = {0xc0, 0xc0, 0xff, 0xff};
    VertexPNC joints[kNumPoints];

    // Fills vertices.
    int index = 0;
    for (int j = 0; j < kNumPointsYZ; j++) { // YZ plan.
      float angle = j * math::k2Pi / kNumSlices;
      float s = sinf(angle), c = cosf(angle);
      VertexPNC& vertex = joints[index++];
      vertex.pos = math::Float3(0.f, c * kRadius, s * kRadius);
      vertex.normal = math::Float3(0.f, c, s);
      vertex.color = red;
    }
    for (int j = 0; j < kNumPointsXY; j++) { // XY plan.
      float angle = j * math::k2Pi / kNumSlices;
      float s = sinf(angle), c = cosf(angle);
      VertexPNC& vertex = joints[index++];
      vertex.pos = math::Float3(s * kRadius, c * kRadius, 0.f);
      vertex.normal = math::Float3(s, c, 0.f);
      vertex.color = blue;
    }
    for (int j = 0; j < kNumPointsXZ; j++) { // XZ plan.
        float angle = j * math::k2Pi / kNumSlices;
        float s = sinf(angle), c = cosf(angle);
        VertexPNC& vertex = joints[index++];
        vertex.pos = math::Float3(c * kRadius, 0.f, -s * kRadius);
        vertex.normal = math::Float3(c, 0.f, -s);
        vertex.color = green;
    }
    assert(index == kNumPoints);

    // Builds and fills the vbo.
    Model& joint = models_[1];
    joint.mode = GL_LINE_STRIP;
    joint.count = OZZ_ARRAY_SIZE(joints);
    GL(GenBuffers(1, &joint.vbo));
    GL(BindBuffer(GL_ARRAY_BUFFER, joint.vbo));
    GL(BufferData(GL_ARRAY_BUFFER, sizeof(joints), joints, GL_STATIC_DRAW));
    GL(BindBuffer(GL_ARRAY_BUFFER, 0));  // Unbinds.

    // Init joint shader.
    joint.shader = InitJointShading();
    if (!joint.shader) {
      return false;
    }
  }

  return true;
}

namespace {
int DrawPosture_FillUniforms(const ozz::animation::Skeleton& _skeleton,
                             ozz::Range<const ozz::math::Float4x4> _matrices,
                             float* _uniforms,
                             int _max_instances) {
    assert(math::IsAligned(_uniforms, AlignOf<math::SimdFloat4>::value));

    // Prepares computation constants.
    const int num_joints = _skeleton.num_joints();
    const ozz::animation::Skeleton::JointProperties* properties =
      _skeleton.joint_properties().begin;

    int instances = 0;
    for (int i = 0; i < num_joints && instances < _max_instances; i++) {

      // Selects parent matrix, which is identity in case of a root.
      const int parent_id = properties[i].parent;
      if (parent_id == ozz::animation::Skeleton::kNoParentIndex) {
        continue;
      }

      // Selects joint matrices.
      const math::Float4x4& parent = _matrices.begin[parent_id];
      const math::Float4x4& current = _matrices.begin[i];

      OZZ_ALIGN(16) float bone_dir[4];
      {
        // Copy parent joint's raw matrix, to render a bone between the parent
        // and current matrix.
        float* uniform = _uniforms + instances * 16;
        math::StorePtr(parent.cols[0], uniform + 0);
        math::StorePtr(parent.cols[1], uniform + 4);
        math::StorePtr(parent.cols[2], uniform + 8);
        math::StorePtr(parent.cols[3], uniform + 12);

        // Set bone direction (bone_dir). The shader expects to find it at index
        // [3,7,11] of the matrix.
        // Index 15 is used to store whether a bone should be rendered,
        // otherwise it's a leaf.
        math::StorePtr(current.cols[3] - parent.cols[3], bone_dir);
        uniform[3] = bone_dir[0];
        uniform[7] = bone_dir[1];
        uniform[11] = bone_dir[2];
        uniform[15] = 1.f;  // Enables bone rendering.
        instances++;
      }

      // Only the joint is rendered for leaves, the bone model isn't.
      if (properties[i].is_leaf) {
        // Copy current joint's raw matrix.
        float* uniform = _uniforms + instances * 16;
        math::StorePtr(current.cols[0], uniform + 0);
        math::StorePtr(current.cols[1], uniform + 4);
        math::StorePtr(current.cols[2], uniform + 8);
        math::StorePtr(current.cols[3], uniform + 12);

        // Re-use bone_dir to fix the size of the leaf (same as previous bone).
        // The shader expects to find it at index [3,7,11] of the matrix.
        uniform[3] = bone_dir[0];
        uniform[7] = bone_dir[1];
        uniform[11] = bone_dir[2];
        uniform[15] = 0.f;  // Disables bone rendering.
        instances++;
      }
    }

    return instances;
}
}  // namespace

// Draw posture internal non-instanced rendering fall back implementation.
void RendererImpl::DrawPosture_Impl(int _instance_count, bool _draw_joints) {
  // Loops through models and instances.
  for (int i = 0; i < (_draw_joints ? 2 : 1); i++) {
    const Model& model = models_[i];

    // Switches to model's shader.
    GL(UseProgram(model.shader->program()));

    // Setup model vertex data.
    GL(BindBuffer(GL_ARRAY_BUFFER, model.vbo));
    GL(EnableClientState(GL_VERTEX_ARRAY));
    GL(VertexPointer(3, GL_FLOAT, sizeof(VertexPNC), 0));
    GL(EnableClientState(GL_NORMAL_ARRAY));
    GL(NormalPointer(GL_FLOAT, sizeof(VertexPNC), GL_OFFSET(12)));
    GL(EnableClientState(GL_COLOR_ARRAY));
    GL(ColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexPNC), GL_OFFSET(24)));
    GL(BindBuffer(GL_ARRAY_BUFFER, 0));

    const GLint uniform_slot = model.shader->uniform(0);
    for (int i = 0; i < _instance_count; i++) {
      GL(UniformMatrix4fv(uniform_slot, 1, false, prealloc_uniforms_ + 16 * i));
      GL(DrawArrays(model.mode, 0, model.count));
    }
  }

  GL(DisableClientState(GL_VERTEX_ARRAY));
  GL(DisableClientState(GL_NORMAL_ARRAY));
  GL(DisableClientState(GL_COLOR_ARRAY));

  // Restores fixed pipeline.
  GL(UseProgram(0));
}

// "Draw posture" internal instanced rendering implementation.
void RendererImpl::DrawPosture_InstancedImpl(int _instance_count,
                                             bool _draw_joints) {
  // Maps the dynamic buffer and update it.
  GL(BindBuffer(GL_ARRAY_BUFFER, joint_instance_vbo_));
  const std::size_t vbo_size = _instance_count * 16 * sizeof(float);
  GL(BufferData(GL_ARRAY_BUFFER, vbo_size, prealloc_uniforms_, GL_STREAM_DRAW));
  GL(BindBuffer(GL_ARRAY_BUFFER, 0));

  // Renders models.
  for (int i = 0; i < (_draw_joints ? 2 : 1); i++) {
    const Model& model = models_[i];

    // Setup model vertex data.
    GL(BindBuffer(GL_ARRAY_BUFFER, model.vbo));
    GL(EnableClientState(GL_VERTEX_ARRAY));
    GL(VertexPointer(3, GL_FLOAT, sizeof(VertexPNC), 0));
    GL(EnableClientState(GL_NORMAL_ARRAY));
    GL(NormalPointer(GL_FLOAT, sizeof(VertexPNC), GL_OFFSET(12)));
    GL(EnableClientState(GL_COLOR_ARRAY));
    GL(ColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexPNC), GL_OFFSET(24)));
    GL(BindBuffer(GL_ARRAY_BUFFER, 0));

    // Switches to model's shader.
    GL(UseProgram(model.shader->program()));

    // Setup instanced GL context.
    const GLint joint_attrib = model.shader->attrib(0);
    GL(BindBuffer(GL_ARRAY_BUFFER, joint_instance_vbo_));
    GL(EnableVertexAttribArray(joint_attrib + 0));
    GL(EnableVertexAttribArray(joint_attrib + 1));
    GL(EnableVertexAttribArray(joint_attrib + 2));
    GL(EnableVertexAttribArray(joint_attrib + 3));
    GL(VertexAttribDivisorARB(joint_attrib + 0, 1));
    GL(VertexAttribDivisorARB(joint_attrib + 1, 1));
    GL(VertexAttribDivisorARB(joint_attrib + 2, 1));
    GL(VertexAttribDivisorARB(joint_attrib + 3, 1));
    GL(VertexAttribPointer(joint_attrib + 0, 4, GL_FLOAT, GL_FALSE,
                           sizeof(math::Float4x4), GL_OFFSET(0)));
    GL(VertexAttribPointer(joint_attrib + 1, 4, GL_FLOAT, GL_FALSE,
                           sizeof(math::Float4x4), GL_OFFSET(16)));
    GL(VertexAttribPointer(joint_attrib + 2, 4, GL_FLOAT, GL_FALSE,
                           sizeof(math::Float4x4), GL_OFFSET(32)));
    GL(VertexAttribPointer(joint_attrib + 3, 4, GL_FLOAT, GL_FALSE,
                           sizeof(math::Float4x4), GL_OFFSET(48)));
    GL(BindBuffer(GL_ARRAY_BUFFER, 0));

    GL(DrawArraysInstancedARB(model.mode, 0, model.count, _instance_count));

    GL(DisableVertexAttribArray(joint_attrib + 0));
    GL(DisableVertexAttribArray(joint_attrib + 1));
    GL(DisableVertexAttribArray(joint_attrib + 2));
    GL(DisableVertexAttribArray(joint_attrib + 3));
    GL(VertexAttribDivisorARB(joint_attrib + 0, 0));
    GL(VertexAttribDivisorARB(joint_attrib + 1, 0));
    GL(VertexAttribDivisorARB(joint_attrib + 2, 0));
    GL(VertexAttribDivisorARB(joint_attrib + 3, 0));
  }

  GL(DisableClientState(GL_VERTEX_ARRAY));
  GL(DisableClientState(GL_NORMAL_ARRAY));
  GL(DisableClientState(GL_COLOR_ARRAY));

  // Restores fixed pipeline.
  GL(UseProgram(0));
}

// Uses GL_ARB_instanced_arrays as a first choice to render the whole skeleton
// in a single draw call.
// Does a draw call per joint if no extension can help.
bool RendererImpl::DrawPosture(const ozz::animation::Skeleton& _skeleton,
                               ozz::Range<const ozz::math::Float4x4> _matrices,
                               const ozz::math::Float4x4& _transform,
                               bool _draw_joints) {
  if (!_matrices.begin || !_matrices.end) {
    return false;
  }
  if (_matrices.end - _matrices.begin < _skeleton.num_joints()) {
    return false;
  }

  // Convert matrices to uniforms.
  const int instance_count = DrawPosture_FillUniforms(
      _skeleton, _matrices, prealloc_uniforms_, max_skeleton_pieces_);
  assert(instance_count <= max_skeleton_pieces_);

  glPushMatrix();
  OZZ_ALIGN(16) float matrix[16];
  ozz::math::StorePtr(_transform.cols[0], matrix + 0);
  ozz::math::StorePtr(_transform.cols[1], matrix + 4);
  ozz::math::StorePtr(_transform.cols[2], matrix + 8);
  ozz::math::StorePtr(_transform.cols[3], matrix + 12);
  glMultMatrixf(matrix);

  if (GL_ARB_instanced_arrays) {
    DrawPosture_InstancedImpl(instance_count, _draw_joints);
  } else {
    DrawPosture_Impl(instance_count, _draw_joints);
  }

  glPopMatrix();

  return true;
}

bool RendererImpl::DrawBox(const ozz::math::Box& _box,
                           const ozz::math::Float4x4& _transform,
                           const Color _colors[2]) {
  glPushMatrix();
  OZZ_ALIGN(16) float matrix[16];
  ozz::math::StorePtr(_transform.cols[0], matrix + 0);
  ozz::math::StorePtr(_transform.cols[1], matrix + 4);
  ozz::math::StorePtr(_transform.cols[2], matrix + 8);
  ozz::math::StorePtr(_transform.cols[3], matrix + 12);
  glMultMatrixf(matrix);

  GL(PushAttrib(GL_POLYGON_BIT));
  GL(PolygonMode(GL_FRONT, GL_FILL));
  for (int i = 0; i < 2; i++) {
    glBegin(GL_QUADS);
      glColor4f(_colors[i].r, _colors[i].g, _colors[i].b, _colors[i].a);
      glVertex3f(_box.min.x, _box.min.y, _box.min.z);
      glVertex3f(_box.min.x, _box.max.y, _box.min.z);
      glVertex3f(_box.max.x, _box.max.y, _box.min.z);
      glVertex3f(_box.max.x, _box.min.y, _box.min.z);
      glVertex3f(_box.max.x, _box.min.y, _box.min.z);
      glVertex3f(_box.max.x, _box.max.y, _box.min.z);
      glVertex3f(_box.max.x, _box.max.y, _box.max.z);
      glVertex3f(_box.max.x, _box.min.y, _box.max.z);
      glVertex3f(_box.min.x, _box.max.y, _box.min.z);
      glVertex3f(_box.min.x, _box.max.y, _box.max.z);
      glVertex3f(_box.max.x, _box.max.y, _box.max.z);
      glVertex3f(_box.max.x, _box.max.y, _box.min.z);
      glVertex3f(_box.min.x, _box.min.y, _box.max.z);
      glVertex3f(_box.max.x, _box.min.y, _box.max.z);
      glVertex3f(_box.max.x, _box.max.y, _box.max.z);
      glVertex3f(_box.min.x, _box.max.y, _box.max.z);
      glVertex3f(_box.min.x, _box.min.y, _box.min.z);
      glVertex3f(_box.min.x, _box.min.y, _box.max.z);
      glVertex3f(_box.min.x, _box.max.y, _box.max.z);
      glVertex3f(_box.min.x, _box.max.y, _box.min.z);
      glVertex3f(_box.min.x, _box.min.y, _box.min.z);
      glVertex3f(_box.max.x, _box.min.y, _box.min.z);
      glVertex3f(_box.max.x, _box.min.y, _box.max.z);
      glVertex3f(_box.min.x, _box.min.y, _box.max.z);
    GL(End());
    GL(PolygonMode(GL_FRONT, GL_LINE)); // Next loop will be with lines.
  }
  GL(PopAttrib());

  glPopMatrix();

  return true;
}

// Helper macro used to initialize extension function pointer.
#define OZZ_INIT_GL_EXT(_fct, _fct_type, _success) \
do {\
  _fct = reinterpret_cast<_fct_type>(glfwGetProcAddress(#_fct)); \
  if (_fct == NULL) {\
    log::Err() << "Unable to install " #_fct " function." << std::endl;\
    _success &= false;\
  }\
} while (void(0), 0)

bool RendererImpl::InitOpenGLExtensions() {
  bool success = true;
  OZZ_INIT_GL_EXT(glBindBuffer, PFNGLBINDBUFFERPROC, success);
  OZZ_INIT_GL_EXT(glDeleteBuffers, PFNGLDELETEBUFFERSPROC, success);
  OZZ_INIT_GL_EXT(glGenBuffers, PFNGLGENBUFFERSPROC, success);
  OZZ_INIT_GL_EXT(glIsBuffer, PFNGLISBUFFERPROC, success);
  OZZ_INIT_GL_EXT(glBufferData, PFNGLBUFFERDATAPROC, success);
  OZZ_INIT_GL_EXT(glBufferSubData, PFNGLBUFFERSUBDATAPROC, success);
  OZZ_INIT_GL_EXT(glGetBufferSubData, PFNGLGETBUFFERSUBDATAPROC, success);
  OZZ_INIT_GL_EXT(glMapBuffer, PFNGLMAPBUFFERPROC, success);
  OZZ_INIT_GL_EXT(glUnmapBuffer, PFNGLUNMAPBUFFERPROC, success);
  OZZ_INIT_GL_EXT(
    glGetBufferParameteriv, PFNGLGETBUFFERPARAMETERIVPROC, success);
  OZZ_INIT_GL_EXT(glGetBufferPointerv, PFNGLGETBUFFERPOINTERVPROC, success);

  OZZ_INIT_GL_EXT(glAttachShader, PFNGLATTACHSHADERPROC, success);
  OZZ_INIT_GL_EXT(glBindAttribLocation, PFNGLBINDATTRIBLOCATIONPROC, success);
  OZZ_INIT_GL_EXT(glCompileShader, PFNGLCOMPILESHADERPROC, success);
  OZZ_INIT_GL_EXT(glCreateProgram, PFNGLCREATEPROGRAMPROC, success);
  OZZ_INIT_GL_EXT(glCreateShader, PFNGLCREATESHADERPROC, success);
  OZZ_INIT_GL_EXT(glDeleteProgram, PFNGLDELETEPROGRAMPROC, success);
  OZZ_INIT_GL_EXT(glDeleteShader, PFNGLDELETESHADERPROC, success);
  OZZ_INIT_GL_EXT(glDetachShader, PFNGLDETACHSHADERPROC, success);
  OZZ_INIT_GL_EXT(
    glDisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAYPROC, success);
  OZZ_INIT_GL_EXT(
    glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC, success);
  OZZ_INIT_GL_EXT(glGetActiveAttrib, PFNGLGETACTIVEATTRIBPROC, success);
  OZZ_INIT_GL_EXT(glGetActiveUniform, PFNGLGETACTIVEUNIFORMPROC, success);
  OZZ_INIT_GL_EXT(glGetAttachedShaders, PFNGLGETATTACHEDSHADERSPROC, success);
  OZZ_INIT_GL_EXT(glGetAttribLocation, PFNGLGETATTRIBLOCATIONPROC, success);
  OZZ_INIT_GL_EXT(glGetProgramiv, PFNGLGETPROGRAMIVPROC, success);
  OZZ_INIT_GL_EXT(glGetProgramInfoLog, PFNGLGETPROGRAMINFOLOGPROC, success);
  OZZ_INIT_GL_EXT(glGetShaderiv, PFNGLGETSHADERIVPROC, success);
  OZZ_INIT_GL_EXT(glGetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC, success);
  OZZ_INIT_GL_EXT(glGetShaderSource, PFNGLGETSHADERSOURCEPROC, success);
  OZZ_INIT_GL_EXT(glGetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC, success);
  OZZ_INIT_GL_EXT(glGetUniformfv, PFNGLGETUNIFORMFVPROC, success);
  OZZ_INIT_GL_EXT(glGetUniformiv, PFNGLGETUNIFORMIVPROC, success);
  OZZ_INIT_GL_EXT(glGetVertexAttribdv, PFNGLGETVERTEXATTRIBDVPROC, success);
  OZZ_INIT_GL_EXT(glGetVertexAttribfv, PFNGLGETVERTEXATTRIBFVPROC, success);
  OZZ_INIT_GL_EXT(glGetVertexAttribiv, PFNGLGETVERTEXATTRIBIVPROC, success);
  OZZ_INIT_GL_EXT(
    glGetVertexAttribPointerv, PFNGLGETVERTEXATTRIBPOINTERVPROC, success);
  OZZ_INIT_GL_EXT(glIsProgram, PFNGLISPROGRAMPROC, success);
  OZZ_INIT_GL_EXT(glIsShader, PFNGLISSHADERPROC, success);
  OZZ_INIT_GL_EXT(glLinkProgram, PFNGLLINKPROGRAMPROC, success);
  OZZ_INIT_GL_EXT(glShaderSource, PFNGLSHADERSOURCEPROC, success);
  OZZ_INIT_GL_EXT(glUseProgram, PFNGLUSEPROGRAMPROC, success);
  OZZ_INIT_GL_EXT(glUniform1f, PFNGLUNIFORM1FPROC, success);
  OZZ_INIT_GL_EXT(glUniform2f, PFNGLUNIFORM2FPROC, success);
  OZZ_INIT_GL_EXT(glUniform3f, PFNGLUNIFORM3FPROC, success);
  OZZ_INIT_GL_EXT(glUniform4f, PFNGLUNIFORM4FPROC, success);
  OZZ_INIT_GL_EXT(glUniform1fv, PFNGLUNIFORM1FVPROC, success);
  OZZ_INIT_GL_EXT(glUniform2fv, PFNGLUNIFORM2FVPROC, success);
  OZZ_INIT_GL_EXT(glUniform3fv, PFNGLUNIFORM3FVPROC, success);
  OZZ_INIT_GL_EXT(glUniform4fv, PFNGLUNIFORM4FVPROC, success);
  OZZ_INIT_GL_EXT(glUniformMatrix2fv, PFNGLUNIFORMMATRIX2FVPROC, success);
  OZZ_INIT_GL_EXT(glUniformMatrix3fv, PFNGLUNIFORMMATRIX3FVPROC, success);
  OZZ_INIT_GL_EXT(glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FVPROC, success);
  OZZ_INIT_GL_EXT(glValidateProgram, PFNGLVALIDATEPROGRAMPROC, success);
  OZZ_INIT_GL_EXT(glVertexAttrib1f, PFNGLVERTEXATTRIB1FPROC, success);
  OZZ_INIT_GL_EXT(glVertexAttrib1fv, PFNGLVERTEXATTRIB1FVPROC, success);
  OZZ_INIT_GL_EXT(glVertexAttrib2f, PFNGLVERTEXATTRIB2FPROC, success);
  OZZ_INIT_GL_EXT(glVertexAttrib2fv, PFNGLVERTEXATTRIB2FVPROC, success);
  OZZ_INIT_GL_EXT(glVertexAttrib3f, PFNGLVERTEXATTRIB3FPROC, success);
  OZZ_INIT_GL_EXT(glVertexAttrib3fv, PFNGLVERTEXATTRIB3FVPROC, success);
  OZZ_INIT_GL_EXT(glVertexAttrib4f, PFNGLVERTEXATTRIB4FPROC, success);
  OZZ_INIT_GL_EXT(glVertexAttrib4fv, PFNGLVERTEXATTRIB4FVPROC, success);
  OZZ_INIT_GL_EXT(glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC, success);
  if (!success) {
    log::Err() << "Failed to initialize mandatory GL extensions." << std::endl;
    return false;
  }

  GL_ARB_instanced_arrays =
    glfwExtensionSupported("GL_ARB_instanced_arrays") != 0;
  if (GL_ARB_instanced_arrays) {
    log::Log() << "Optional GL_ARB_instanced_arrays extensions found." <<
      std::endl;
    success = true;
    OZZ_INIT_GL_EXT(
      glVertexAttribDivisorARB, PFNGLVERTEXATTRIBDIVISORARBPROC, success);
    OZZ_INIT_GL_EXT(
      glDrawArraysInstancedARB, PFNGLDRAWARRAYSINSTANCEDARBPROC, success);
    OZZ_INIT_GL_EXT(
      glDrawElementsInstancedARB, PFNGLDRAWELEMENTSINSTANCEDARBPROC, success);
    if (!success) {
      log::Err() <<
        "Failed to setup GL_ARB_instanced_arrays, feature is disabled."
        << std::endl;
      GL_ARB_instanced_arrays = false;
    }
  } else {
    log::Log() << "Optional GL_ARB_instanced_arrays extensions not found." <<
      std::endl;
  }
  return true;
}

Shader::Shader()
    : program_(0),
      vertex_(0),
      fragment_(0) {
}

Shader::~Shader() {
  if (vertex_) {
    GL(DetachShader(program_, vertex_));
    GL(DeleteShader(vertex_));
  }
  if (fragment_) {
    GL(DetachShader(program_, fragment_));
    GL(DeleteShader(fragment_));
  }
  if (program_) {
    GL(DeleteProgram(program_));
  }
}

Shader* Shader::Build(int _vertex_count, const char** _vertex,
                      int _fragment_count, const char** _fragment) {
  // Tries to compile shaders.
  GLuint vertex_shader = 0;
  if (_vertex) {
    vertex_shader =
      CompileShader(GL_VERTEX_SHADER, _vertex_count, _vertex);
    if (!vertex_shader) {
      return NULL;
    }
  }
  GLuint fragment_shader = 0;
  if (_fragment) {
    fragment_shader =
      CompileShader(GL_FRAGMENT_SHADER, _fragment_count, _fragment);
    if (!fragment_shader) {
      if (vertex_shader) {
        GL(DeleteShader(vertex_shader));
      }
      return NULL;
    }
  }

  // Shaders are compiled, builds program.
  Shader* shader = memory::default_allocator()->New<Shader>();
  shader->program_ = glCreateProgram();
  shader->vertex_ = vertex_shader;
  shader->fragment_ = fragment_shader;
  GL(AttachShader(shader->program_, vertex_shader));
  GL(AttachShader(shader->program_, fragment_shader));
  GL(LinkProgram(shader->program_));

  int infolog_length = 0;
  GL(GetProgramiv(shader->program_, GL_INFO_LOG_LENGTH, &infolog_length));
  if (infolog_length > 0) {
    char* info_log = memory::default_allocator()->Allocate<char>(infolog_length);
    int chars_written  = 0;
    glGetProgramInfoLog(
      shader->program_, infolog_length, &chars_written, info_log);
    log::Err() << info_log << std::endl;
    memory::default_allocator()->Deallocate(info_log);
  }

  return shader;
}

GLuint Shader::CompileShader(GLenum _type, int _count, const char** _src) {
  GLuint shader = glCreateShader(_type);
  GL(ShaderSource(shader, _count, _src, NULL));
  GL(CompileShader(shader));

  int infolog_length = 0;
  GL(GetShaderiv(shader, GL_INFO_LOG_LENGTH, &infolog_length));
  if (infolog_length > 0) {
    char* info_log = memory::default_allocator()->Allocate<char>(infolog_length);
    int chars_written  = 0;
    glGetShaderInfoLog(shader, infolog_length, &chars_written, info_log);
    log::Err() << info_log << std::endl;
    memory::default_allocator()->Deallocate(info_log);
  }

  int status;
  GL(GetShaderiv(shader, GL_COMPILE_STATUS, &status));
  if (status) {
    return shader;
  }

  GL(DeleteShader(shader));
  return 0;
}

bool Shader::BindUniform(const char* _semantic) {
  GLint location = glGetUniformLocation(program_, _semantic);
  if (location == -1) {  // _semantic not found.
    return false;
  }
  uniforms_.push_back(location);
  return true;
}

bool Shader::BindAttrib(const char* _semantic) {
  GLint location = glGetAttribLocation(program_, _semantic);
  if (location == -1) {  // _semantic not found.
    return false;
  }
  attribs_.push_back(location);
  return true;
}
}  // internal
}  // sample
}  // ozz

// Helper macro used to declare extension function pointer.
#define OZZ_DECL_GL_EXT(_fct, _fct_type) _fct_type _fct = NULL

OZZ_DECL_GL_EXT(glBindBuffer, PFNGLBINDBUFFERPROC);
OZZ_DECL_GL_EXT(glDeleteBuffers, PFNGLDELETEBUFFERSPROC);
OZZ_DECL_GL_EXT(glGenBuffers, PFNGLGENBUFFERSPROC);
OZZ_DECL_GL_EXT(glIsBuffer, PFNGLISBUFFERPROC);
OZZ_DECL_GL_EXT(glBufferData, PFNGLBUFFERDATAPROC);
OZZ_DECL_GL_EXT(glBufferSubData, PFNGLBUFFERSUBDATAPROC);
OZZ_DECL_GL_EXT(glGetBufferSubData, PFNGLGETBUFFERSUBDATAPROC);
OZZ_DECL_GL_EXT(glMapBuffer, PFNGLMAPBUFFERPROC);
OZZ_DECL_GL_EXT(glUnmapBuffer, PFNGLUNMAPBUFFERPROC);
OZZ_DECL_GL_EXT(glGetBufferParameteriv, PFNGLGETBUFFERPARAMETERIVPROC);
OZZ_DECL_GL_EXT(glGetBufferPointerv, PFNGLGETBUFFERPOINTERVPROC);

OZZ_DECL_GL_EXT(glAttachShader, PFNGLATTACHSHADERPROC);
OZZ_DECL_GL_EXT(glBindAttribLocation, PFNGLBINDATTRIBLOCATIONPROC);
OZZ_DECL_GL_EXT(glCompileShader, PFNGLCOMPILESHADERPROC);
OZZ_DECL_GL_EXT(glCreateProgram, PFNGLCREATEPROGRAMPROC);
OZZ_DECL_GL_EXT(glCreateShader, PFNGLCREATESHADERPROC);
OZZ_DECL_GL_EXT(glDeleteProgram, PFNGLDELETEPROGRAMPROC);
OZZ_DECL_GL_EXT(glDeleteShader, PFNGLDELETESHADERPROC);
OZZ_DECL_GL_EXT(glDetachShader, PFNGLDETACHSHADERPROC);
OZZ_DECL_GL_EXT(glDisableVertexAttribArray, PFNGLDISABLEVERTEXATTRIBARRAYPROC);
OZZ_DECL_GL_EXT(glEnableVertexAttribArray, PFNGLENABLEVERTEXATTRIBARRAYPROC);
OZZ_DECL_GL_EXT(glGetActiveAttrib, PFNGLGETACTIVEATTRIBPROC);
OZZ_DECL_GL_EXT(glGetActiveUniform, PFNGLGETACTIVEUNIFORMPROC);
OZZ_DECL_GL_EXT(glGetAttachedShaders, PFNGLGETATTACHEDSHADERSPROC);
OZZ_DECL_GL_EXT(glGetAttribLocation, PFNGLGETATTRIBLOCATIONPROC);
OZZ_DECL_GL_EXT(glGetProgramiv, PFNGLGETPROGRAMIVPROC);
OZZ_DECL_GL_EXT(glGetProgramInfoLog, PFNGLGETPROGRAMINFOLOGPROC);
OZZ_DECL_GL_EXT(glGetShaderiv, PFNGLGETSHADERIVPROC);
OZZ_DECL_GL_EXT(glGetShaderInfoLog, PFNGLGETSHADERINFOLOGPROC);
OZZ_DECL_GL_EXT(glGetShaderSource, PFNGLGETSHADERSOURCEPROC);
OZZ_DECL_GL_EXT(glGetUniformLocation, PFNGLGETUNIFORMLOCATIONPROC);
OZZ_DECL_GL_EXT(glGetUniformfv, PFNGLGETUNIFORMFVPROC);
OZZ_DECL_GL_EXT(glGetUniformiv, PFNGLGETUNIFORMIVPROC);
OZZ_DECL_GL_EXT(glGetVertexAttribdv, PFNGLGETVERTEXATTRIBDVPROC);
OZZ_DECL_GL_EXT(glGetVertexAttribfv, PFNGLGETVERTEXATTRIBFVPROC);
OZZ_DECL_GL_EXT(glGetVertexAttribiv, PFNGLGETVERTEXATTRIBIVPROC);
OZZ_DECL_GL_EXT(glGetVertexAttribPointerv, PFNGLGETVERTEXATTRIBPOINTERVPROC);
OZZ_DECL_GL_EXT(glIsProgram, PFNGLISPROGRAMPROC);
OZZ_DECL_GL_EXT(glIsShader, PFNGLISSHADERPROC);
OZZ_DECL_GL_EXT(glLinkProgram, PFNGLLINKPROGRAMPROC);
OZZ_DECL_GL_EXT(glShaderSource, PFNGLSHADERSOURCEPROC);
OZZ_DECL_GL_EXT(glUseProgram, PFNGLUSEPROGRAMPROC);
OZZ_DECL_GL_EXT(glUniform1f, PFNGLUNIFORM1FPROC);
OZZ_DECL_GL_EXT(glUniform2f, PFNGLUNIFORM2FPROC);
OZZ_DECL_GL_EXT(glUniform3f, PFNGLUNIFORM3FPROC);
OZZ_DECL_GL_EXT(glUniform4f, PFNGLUNIFORM4FPROC);
OZZ_DECL_GL_EXT(glUniform1fv, PFNGLUNIFORM1FVPROC);
OZZ_DECL_GL_EXT(glUniform2fv, PFNGLUNIFORM2FVPROC);
OZZ_DECL_GL_EXT(glUniform3fv, PFNGLUNIFORM3FVPROC);
OZZ_DECL_GL_EXT(glUniform4fv, PFNGLUNIFORM4FVPROC);
OZZ_DECL_GL_EXT(glUniformMatrix2fv, PFNGLUNIFORMMATRIX2FVPROC);
OZZ_DECL_GL_EXT(glUniformMatrix3fv, PFNGLUNIFORMMATRIX3FVPROC);
OZZ_DECL_GL_EXT(glUniformMatrix4fv, PFNGLUNIFORMMATRIX4FVPROC);
OZZ_DECL_GL_EXT(glValidateProgram, PFNGLVALIDATEPROGRAMPROC);
OZZ_DECL_GL_EXT(glVertexAttrib1f, PFNGLVERTEXATTRIB1FPROC);
OZZ_DECL_GL_EXT(glVertexAttrib1fv, PFNGLVERTEXATTRIB1FVPROC);
OZZ_DECL_GL_EXT(glVertexAttrib2f, PFNGLVERTEXATTRIB2FPROC);
OZZ_DECL_GL_EXT(glVertexAttrib2fv, PFNGLVERTEXATTRIB2FVPROC);
OZZ_DECL_GL_EXT(glVertexAttrib3f, PFNGLVERTEXATTRIB3FPROC);
OZZ_DECL_GL_EXT(glVertexAttrib3fv, PFNGLVERTEXATTRIB3FVPROC);
OZZ_DECL_GL_EXT(glVertexAttrib4f, PFNGLVERTEXATTRIB4FPROC);
OZZ_DECL_GL_EXT(glVertexAttrib4fv, PFNGLVERTEXATTRIB4FVPROC);
OZZ_DECL_GL_EXT(glVertexAttribPointer, PFNGLVERTEXATTRIBPOINTERPROC);

bool GL_ARB_instanced_arrays = false;
OZZ_DECL_GL_EXT(glVertexAttribDivisorARB, PFNGLVERTEXATTRIBDIVISORARBPROC);
OZZ_DECL_GL_EXT(glDrawArraysInstancedARB, PFNGLDRAWARRAYSINSTANCEDARBPROC);
OZZ_DECL_GL_EXT(glDrawElementsInstancedARB, PFNGLDRAWELEMENTSINSTANCEDARBPROC);
