//============================================================================//
// Copyright (c) <2012> <Guillaume Blanc>                                     //
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
//============================================================================//

// This file is allowed to include private headers.
#define OZZ_INCLUDE_PRIVATE_HEADER

#include "framework/application.h"

#include <GL/glfw.h>
#include <GL/glext.h>

#include <cstdlib>
#include <cassert>
#include <cstring>

#include "ozz/base/log.h"
#include "ozz/base/maths/box.h"
#include "ozz/base/memory/allocator.h"
#include "ozz/options/options.h"
#include "framework/renderer.h"
#include "framework/profile.h"
#include "framework/internal/camera.h"
#include "framework/internal/imgui_impl.h"
#include "framework/internal/renderer_impl.h"

OZZ_OPTIONS_DECLARE_FLOAT(
  auto_exit_time,
  "The time before application automatically exits."
  " A negative value disables this feature.",
  -1.f,
  false);

namespace ozz {
namespace demo {

Application* Application::application_ = NULL;

Application::Application()
    : freeze_(false),
      exit_(false),
      time_factor_(1.f),
      last_idle_time_(0.),
      camera_(NULL),
      auto_framing_(false),
      renderer_(NULL),
      im_gui_(NULL),
      fps_(memory::default_allocator().New<Record>(128)),
      update_time_(memory::default_allocator().New<Record>(128)),
      render_time_(memory::default_allocator().New<Record>(128)) {
}

Application::~Application() {
  memory::default_allocator().Delete(fps_);
  memory::default_allocator().Delete(update_time_);
  memory::default_allocator().Delete(render_time_);
}

int Application::Run(int _argc, const char** _argv,
                     const char* _version, const char* _usage) {
  // Only one application at a time can be ran.
  if (application_) {
    return EXIT_FAILURE;
  }
  application_ = this;

  // Parse comand line arguments.
  ozz::options::ParseResult result =
    ozz::options::ParseCommandLine(_argc, _argv, _version, _usage);
  if (result != ozz::options::kSuccess) {
    exit_ = true;
    return result == ozz::options::kExitSuccess ? EXIT_SUCCESS : EXIT_FAILURE;
  }

  // Initialize GLFW
  if (!glfwInit()) {
    application_ = NULL;
    return EXIT_FAILURE;
  }

  // Setup GL context.
  const int gl_version_major = 2, gl_version_minor = 0;
  glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, gl_version_major);
  glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, gl_version_minor);
  glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 4);
#ifndef NDEBUG
  glfwOpenWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif  // NDEBUG

  // Open an OpenGL window
  bool success = true;
  if (!glfwOpenWindow(1024, 576, 8, 8, 8, 8, 32, 0, GLFW_WINDOW)) {
    log::Err() << "Failed to open OpenGL window. Required OpenGL version is " <<
      gl_version_major << "." << gl_version_minor << "." << std::endl;
    success = false;
  } else {
    log::Err() << "Successfully opened OpenGL window version \"" <<
      glGetString(GL_VERSION) << "\"." << std::endl;

    // Allocates and initializes internal objects.
    camera_ = memory::default_allocator().New<internal::Camera>();
    im_gui_ = memory::default_allocator().New<internal::ImGuiImpl>();
    renderer_ = memory::default_allocator().New<internal::RendererImpl>();
    success = renderer_->Initialize();

    // Setup the window and installs callbacks.
    glfwSwapInterval(1);  // Enables vertical sync by default.
    glfwSetWindowTitle(GetTitle());
    glfwSetWindowSizeCallback(&ResizeCbk);
    glfwSetWindowCloseCallback(&CloseCbk);

    // Initialize demo.
    if (success) {
      success = OnInitialize();
    }

     // Loop if initialization succeeded.
    if (success) {
      success = Loop();
    }

    // Notifies that an error occurred.
    if (!success) {
      log::Err() << "An error occurred during demo execution." <<
        std::endl;
    }

    // De-initialize demo, even in case of initialization failure.
    OnDestroy();

    // Delete the camera
    memory::default_allocator().Delete(camera_);
    camera_ = NULL;
    memory::default_allocator().Delete(im_gui_);
    im_gui_ = NULL;
    memory::default_allocator().Delete(renderer_);
    renderer_ = NULL;
  }

  // Closes window and terminates GLFW.
  glfwTerminate();
  application_ = NULL;

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

bool Application::Loop() {
  bool success = true;
  const double first_loop_time = glfwGetTime();
  for (bool first_loop = true; ; first_loop = false) {
    Profiler profile(fps_);  // Profiles frame.

    // Tests for a manual exit request.
    if (exit_ || glfwGetKey(GLFW_KEY_ESC)) {
      break;
    }

    // Test for an exit request.
    if (OPTIONS_auto_exit_time > 0.f &&
        glfwGetTime() > first_loop_time + OPTIONS_auto_exit_time) {
      break;
    }

    // Don't overload the cpu if the window is not active.
    if (!glfwGetWindowParam(GLFW_ACTIVE)) {
      glfwWaitEvents();  // Wait...

      // Reset last update time in order to stop the time while the app isn't
      // active.
      last_idle_time_ = glfwGetTime();

      continue;  // ...but don't do anything.
    }

    // Do the main loop.
    success = Idle();
    if (!success) {
      break;
    }

    // Test for camera framing requests.
    bool f_key = glfwGetKey('F') == GLFW_PRESS;
    if (first_loop || auto_framing_  || f_key) {
      // Note the GetSceneBounds must not be called before the first update.
      math::Box scene_bounds;
      if (GetSceneBounds(&scene_bounds)) {
        camera_->FrameAll(scene_bounds, first_loop);
      }
    }

    success = Display();
    if (!success) {
      break;
    }
  }
  return success;
}

bool Application::Display() {
  bool success = true;

  { // Profiles rendering excluding GUI.
    Profiler profile(render_time_);

    GL(ClearColor(.33f, .333f, .315f, 0.f));
    GL(Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    // Setup default states
    GL(ShadeModel(GL_SMOOTH));
    GL(Enable(GL_CULL_FACE));
    GL(CullFace(GL_BACK));
    GL(Enable(GL_DEPTH_TEST));
    GL(DepthMask(GL_TRUE));
    GL(DepthFunc(GL_LEQUAL));

    // Bind camera modelview matrix.
    camera_->Bind();

    // Forwards display event to the inheriting application.
    if (success) {
      success = OnDisplay(renderer_);
    }
  }  // Ends profiling.

  // Renders grid and axes at the end as they are transparent.
  GL(DepthMask(GL_FALSE));
  renderer_->DrawGrid(10, 1.f);
  GL(DepthMask(GL_TRUE));
  renderer_->DrawAxes(1.f);

  // Forwards gui event to the inheriting application.
  if (success) {
    success = Gui();
  }

  // Swaps current window.
  glfwSwapBuffers();

  return success;
}

bool Application::Idle() {
  // Compute elapsed time since last idle.
  const double time = glfwGetTime();
  const float delta = static_cast<float>(time - last_idle_time_);
  const float scaled_delta = freeze_ ? 0.f : delta * time_factor_;
  last_idle_time_ = time;

  // Update camera model-view matrix.
  camera_->Update(delta);

  // Forwards update event to the inheriting application.
  Profiler profile(update_time_);  // Profiles update.
  return OnUpdate(scaled_delta);
}

bool Application::Gui() {
  bool success = true;
  const int kFormWidth = 190;

  // Finds gui area.
  const int kGuiMargin = 2;
  ozz::math::RectInt window_rect(0, 0, 0, 0);
  glfwGetWindowSize(&window_rect.width, &window_rect.height);

  // Fills ImGui's input structure.
  internal::ImGuiImpl::Inputs input;
  int mouse_y;
  glfwGetMousePos(&input.mouse_x, &mouse_y);
  input.mouse_y = window_rect.height - mouse_y;
  input.lmb_pressed = glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

  // Starts frame
  static_cast<internal::ImGuiImpl*>(im_gui_)->BeginFrame(input, window_rect);

  // Do framework gui.
  if (success &&
      window_rect.width > (kGuiMargin + kFormWidth) * 2) {
    static bool open = true;
    math::RectInt rect(kGuiMargin,
                       kGuiMargin,
                       kFormWidth,
                       window_rect.height - kGuiMargin * 2);
    ImGui::Form form(im_gui_, "Framework", rect, &open);
    if (open) {
      success = FrameworkGui();
    }
  }

  // Do demo gui.
  if (success &&
      window_rect.width > kGuiMargin + kFormWidth) {  // Can't render anything.
    static bool open = true;
    math::RectInt rect(window_rect.width - kFormWidth - kGuiMargin,
                       kGuiMargin,
                       kFormWidth,
                       window_rect.height - kGuiMargin * 2);
    ImGui::Form form(im_gui_, "Demo", rect, &open);
    if (open) {
      { // Displays description message.
        static bool open_desc = true;
        ImGui::OpenClose controls(im_gui_, "Description", &open_desc);
        if (open_desc) {
          im_gui_->DoLabel(
            ozz::options::ParsedExecutableUsage(), ImGui::kLeft, false);
        }
      }
      // Forwards event to the inherited application.
      success = OnGui(im_gui_);
    }
  }

  // Ends frame
  static_cast<internal::ImGuiImpl*>(im_gui_)->EndFrame();

  return success;
}

bool Application::FrameworkGui() {
  { // Render statistics
    static bool open = true;
    ImGui::OpenClose stats(im_gui_, "Statistics", &open);
    if (open) {
      char szLabel[64];
      float min, max, mean;
      { // FPS
        fps_->Statistics(&min, &max, &mean);
        std::sprintf(szLabel, "FPS: %.0f", 1000.f / mean);
        static bool fps_open = true;
        ImGui::OpenClose stats(im_gui_, szLabel, &fps_open);
        if (fps_open) {
          std::sprintf(szLabel, "Frame: %.2f ms", mean);
          im_gui_->DoGraph(
            szLabel, 0.f, 20.f, mean,
            fps_->cursor(),
            fps_->record_begin(), fps_->record_end());
        }
      }
      { // Update time
        update_time_->Statistics(&min, &max, &mean);
        std::sprintf(szLabel, "Update: %.2f ms", mean);
        static bool update_open = false;
        ImGui::OpenClose stats(im_gui_, szLabel, &update_open);
        if (update_open) {
          im_gui_->DoGraph(
            NULL, 0.f, 1.f, mean,
            update_time_->cursor(),
            update_time_->record_begin(), update_time_->record_end());
        }
      }
      { // Render time
        render_time_->Statistics(&min, &max, &mean);
        std::sprintf(szLabel, "Render: %.2f ms", mean);
        static bool render_open = false;
        ImGui::OpenClose stats(im_gui_, szLabel, &render_open);
        if (render_open) {
          im_gui_->DoGraph(
            NULL, 0.f, 1.f, mean,
            render_time_->cursor(),
            render_time_->record_begin(), render_time_->record_end());
        }
      }
    }
  }

  { // Time control
    static bool open = true;
    ImGui::OpenClose stats(im_gui_, "Time control", &open);
    if (open) {
      im_gui_->DoCheckBox("Freeze", &freeze_, true);
      char szFactor[64];
      std::sprintf(szFactor, "Time factor: %.2f", time_factor_);
      im_gui_->DoSlider(szFactor, 0.f, 10.f, &time_factor_, .5f, true);
      if (im_gui_->DoButton("Reset time factor", time_factor_ != 1.f)) {
        time_factor_ = 1.f;
      }
    }
  }

  { // Rendering options
    static bool open = true;
    ImGui::OpenClose options(im_gui_, "Options", &open);
    if (open) {
      // Multi-sampling.
      static bool fsaa_available = glfwGetWindowParam(GLFW_FSAA_SAMPLES) != 0;
      static bool fsaa_enabled = fsaa_available;
      if (im_gui_->DoCheckBox("Multisampling", &fsaa_enabled, fsaa_available)) {
        if (fsaa_enabled) {
          GL(Enable(GL_MULTISAMPLE));
        } else {
          GL(Disable(GL_MULTISAMPLE));
        }
      }
      // Vertical sync
      static bool vertical_sync_ = true;  // On by default.
      if (im_gui_->DoCheckBox("Vertical sync", &vertical_sync_, true)) {
        glfwSwapInterval(vertical_sync_ ? 1 : 0);
      }
    }
  }

  { // Controls
    static bool open = true;
    ImGui::OpenClose controls(im_gui_, "Camera controls", &open);
    if (open) {
      im_gui_->DoCheckBox("Automatic framing", &auto_framing_);
      static bool help = true;
      ImGui::OpenClose controls(im_gui_, "Help", &help);
      const char* controls_label =
        "-F: Frame all\n"
        "-RMB: Rotate\n"
        "-Ctrl + RMB: Zoom\n"
        "-Shift + RMB: Pan\n"
        "-MMB: Center\n";
      im_gui_->DoLabel(controls_label, ImGui::kLeft, false);
    }
  }
  return true;
}

void Application::ResizeCbk(int _width, int _height) {
  // Uses a full viewport.
  GL(Viewport(0, 0, _width, _height));

  // Forwards screen size to the camera.
  application_->camera_->Resize(_width, _height);
}

int Application::CloseCbk() {
  application_->exit_ = true;
  return GL_FALSE;  // The window will be closed while exiting the main loop.
}
}  // demo
}  // ozz
