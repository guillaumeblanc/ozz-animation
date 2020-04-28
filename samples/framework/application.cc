//----------------------------------------------------------------------------//
//                                                                            //
// ozz-animation is hosted at http://github.com/guillaumeblanc/ozz-animation  //
// and distributed under the MIT License (MIT).                               //
//                                                                            //
// Copyright (c) Guillaume Blanc                                              //
//                                                                            //
// Permission is hereby granted, free of charge, to any person obtaining a    //
// copy of this software and associated documentation files (the "Software"), //
// to deal in the Software without restriction, including without limitation  //
// the rights to use, copy, modify, merge, publish, distribute, sublicense,   //
// and/or sell copies of the Software, and to permit persons to whom the      //
// Software is furnished to do so, subject to the following conditions:       //
//                                                                            //
// The above copyright notice and this permission notice shall be included in //
// all copies or substantial portions of the Software.                        //
//                                                                            //
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR //
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   //
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    //
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER //
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    //
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        //
// DEALINGS IN THE SOFTWARE.                                                  //
//                                                                            //
//----------------------------------------------------------------------------//

#define OZZ_INCLUDE_PRIVATE_HEADER  // Allows to include private headers.

#include "framework/application.h"

#include <cassert>
#include <cstdlib>
#include <cstring>

#ifdef __APPLE__
#include <unistd.h>
#endif  // __APPLE__

#if EMSCRIPTEN
#include <emscripten.h>
#include <emscripten/html5.h>
#endif  // EMSCRIPTEN

#include "framework/image.h"
#include "framework/internal/camera.h"
#include "framework/internal/imgui_impl.h"
#include "framework/internal/renderer_impl.h"
#include "framework/internal/shooter.h"
#include "framework/profile.h"
#include "framework/renderer.h"
#include "ozz/base/io/stream.h"
#include "ozz/base/log.h"
#include "ozz/base/maths/box.h"
#include "ozz/base/memory/allocator.h"
#include "ozz/options/options.h"

OZZ_OPTIONS_DECLARE_INT(
    max_idle_loops,
    "The maximum number of idle loops the sample application can perform."
    " Application automatically exit when this number of loops is reached."
    " A negative value disables this feature.",
    -1, false);

OZZ_OPTIONS_DECLARE_BOOL(render, "Enables sample redering.", true, false);

namespace {
// Screen resolution presets.
const ozz::sample::Resolution resolution_presets[] = {
    {640, 360},   {640, 480},  {800, 450},  {800, 600},   {1024, 576},
    {1024, 768},  {1280, 720}, {1280, 800}, {1280, 960},  {1280, 1024},
    {1400, 1050}, {1440, 900}, {1600, 900}, {1600, 1200}, {1680, 1050},
    {1920, 1080}, {1920, 1200}};
const int kNumPresets = OZZ_ARRAY_SIZE(resolution_presets);
}  // namespace

// Check resolution argument is within 0 - kNumPresets
static bool ResolutionCheck(const ozz::options::Option& _option,
                            int /*_argc*/) {
  const ozz::options::IntOption& option =
      static_cast<const ozz::options::IntOption&>(_option);
  return option >= 0 && option < kNumPresets;
}

OZZ_OPTIONS_DECLARE_INT_FN(resolution, "Resolution index (0 to 17).", 5, false,
                           &ResolutionCheck);

namespace ozz {
namespace sample {
Application* Application::application_ = nullptr;

Application::Application()
    : exit_(false),
      freeze_(false),
      fix_update_rate(false),
      fixed_update_rate(60.f),
      time_factor_(1.f),
      time_(0.f),
      last_idle_time_(0.),
      show_help_(false),
      show_grid_(true),
      show_axes_(true),
      capture_video_(false),
      capture_screenshot_(false),
      fps_(New<Record>(128)),
      update_time_(New<Record>(128)),
      render_time_(New<Record>(128)),
      resolution_(resolution_presets[0]) {
#ifndef NDEBUG
  // Assert presets are correctly sorted.
  for (int i = 1; i < kNumPresets; ++i) {
    const Resolution& preset_m1 = resolution_presets[i - 1];
    const Resolution& preset = resolution_presets[i];
    assert(preset.width > preset_m1.width || preset.height > preset_m1.height);
  }
#endif  //  NDEBUG
}

Application::~Application() {}

int Application::Run(int _argc, const char** _argv, const char* _version,
                     const char* _title) {
  // Only one application at a time can be ran.
  if (application_) {
    return EXIT_FAILURE;
  }
  application_ = this;

  // Starting application
  log::Out() << "Starting sample \"" << _title << "\" version \"" << _version
             << "\"" << std::endl;
  log::Out() << "Ozz libraries were built with \""
             << math::SimdImplementationName() << "\" SIMD math implementation."
             << std::endl;

  // Parse command line arguments.
  const char* usage =
      "Ozz animation sample. See README.md file for more details.";
  ozz::options::ParseResult result =
      ozz::options::ParseCommandLine(_argc, _argv, _version, usage);
  if (result != ozz::options::kSuccess) {
    exit_ = true;
    return result == ozz::options::kExitSuccess ? EXIT_SUCCESS : EXIT_FAILURE;
  }

  // Fetch initial resolution.
  resolution_ = resolution_presets[OPTIONS_resolution];

#ifdef __APPLE__
  // On OSX, when run from Finder, working path is the root path. This does not
  // allow to load resources from relative path.
  // The workaround is to change the working directory to application directory.
  // The proper solution would probably be to use bundles and load data from
  // resource folder.
  chdir(ozz::options::ParsedExecutablePath().c_str());
#endif  // __APPLE__

  // Initialize help.
  ParseReadme();

  // Open an OpenGL window
  bool success = true;
  if (OPTIONS_render) {
    // Initialize GLFW
    if (!glfwInit()) {
      application_ = nullptr;
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

    // Initializes rendering before looping.
    if (!glfwOpenWindow(resolution_.width, resolution_.height, 8, 8, 8, 8, 32,
                        0, GLFW_WINDOW)) {
      log::Err() << "Failed to open OpenGL window. Required OpenGL version is "
                 << gl_version_major << "." << gl_version_minor << "."
                 << std::endl;
      success = false;
    } else {
      log::Out() << "Successfully opened OpenGL window version \""
                 << glGetString(GL_VERSION) << "\"." << std::endl;

      // Allocates and initializes camera
      camera_ = make_unique<internal::Camera>();
      math::Float3 camera_center;
      math::Float2 camera_angles;
      float distance;
      if (GetCameraInitialSetup(&camera_center, &camera_angles, &distance)) {
        camera_->Reset(camera_center, camera_angles, distance);
      }

      // Allocates and initializes renderer.
      renderer_ = make_unique<internal::RendererImpl>(camera_.get());
      success = renderer_->Initialize();

      if (success) {
        shooter_ = make_unique<internal::Shooter>();
        im_gui_ = make_unique<internal::ImGuiImpl>();

#ifndef EMSCRIPTEN  // Better not rename web page.
        glfwSetWindowTitle(_title);
#endif  // EMSCRIPTEN

        // Setup the window and installs callbacks.
        glfwSwapInterval(1);  // Enables vertical sync by default.
        glfwSetWindowSizeCallback(&ResizeCbk);
        glfwSetWindowCloseCallback(&CloseCbk);

        // Loop the sample.
        success = Loop();
        shooter_.reset();
        im_gui_.reset();
      }
      renderer_.reset();
      camera_.reset();
    }

    // Closes window and terminates GLFW.
    glfwTerminate();

  } else {
    // Loops without any rendering initialization.
    success = Loop();
  }

  // Notifies that an error occurred.
  if (!success) {
    log::Err() << "An error occurred during sample execution." << std::endl;
  }

  application_ = nullptr;

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}

// Helper function to detecte key pressed and released.
template <int _Key>
bool KeyPressed() {
  static int previous_key = glfwGetKey(_Key);
  const int key = glfwGetKey(_Key);
  const bool pressed = previous_key == GLFW_PRESS && key == GLFW_RELEASE;
  previous_key = key;
  return pressed;
}

Application::LoopStatus Application::OneLoop(int _loops) {
  Profiler profile(fps_.get());  // Profiles frame.

  // Tests for a manual exit request.
  if (exit_ || glfwGetKey(GLFW_KEY_ESC) == GLFW_PRESS) {
    return kBreak;
  }

  // Test for an exit request.
  if (OPTIONS_max_idle_loops > 0 && _loops > OPTIONS_max_idle_loops) {
    return kBreak;
  }

// Don't overload the cpu if the window is not active.
#ifndef EMSCRIPTEN
  if (OPTIONS_render && !glfwGetWindowParam(GLFW_ACTIVE)) {
    glfwWaitEvents();  // Wait...

    // Reset last update time in order to stop the time while the app isn't
    // active.
    last_idle_time_ = glfwGetTime();

    return kContinue;  // ...but don't do anything.
  }
#else
  int width, height;
  if (emscripten_get_canvas_element_size(nullptr, &width, &height) !=
      EMSCRIPTEN_RESULT_SUCCESS) {
    return kBreakFailure;
  }
  if (width != resolution_.width || height != resolution_.height) {
    ResizeCbk(width, height);
  }
#endif  // EMSCRIPTEN

  // Enable/disable help on F1 key.
  show_help_ = show_help_ ^ KeyPressed<GLFW_KEY_F1>();

  // Capture screenshot or video.
  capture_screenshot_ = KeyPressed<'S'>();
  capture_video_ = capture_video_ ^ KeyPressed<'V'>();

  // Do the main loop.
  if (!Idle(_loops == 0)) {
    return kBreakFailure;
  }

  // Skips display if "no_render" option is enabled.
  if (OPTIONS_render) {
    if (!Display()) {
      return kBreakFailure;
    }
  }

  return kContinue;
}

void OneLoopCbk(void* _arg) {
  Application* app = reinterpret_cast<Application*>(_arg);
  static int loops = 0;
  app->OneLoop(loops++);
}

bool Application::Loop() {
  // Initialize sample.
  bool success = OnInitialize();

// Emscripten requires to manage the main loop on their own, as browsers don't
// like infinite blocking functions.
#ifdef EMSCRIPTEN
  emscripten_set_main_loop_arg(OneLoopCbk, this, 0, 1);
#else   // EMSCRIPTEN
  // Loops.
  for (int loops = 0; success; ++loops) {
    const LoopStatus status = OneLoop(loops);
    success = status != kBreakFailure;
    if (status != kContinue) {
      break;
    }
  }
#endif  // EMSCRIPTEN

  // De-initialize sample, even in case of initialization failure.
  OnDestroy();

  return success;
}

bool Application::Display() {
  assert(OPTIONS_render);

  bool success = true;

  {  // Profiles rendering excluding GUI.
    Profiler profile(render_time_.get());

    GL(ClearDepth(1.f));
    GL(ClearColor(.4f, .42f, .38f, 1.f));
    GL(Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    // Setup default states
    GL(Enable(GL_CULL_FACE));
    GL(CullFace(GL_BACK));
    GL(Enable(GL_DEPTH_TEST));
    GL(DepthMask(GL_TRUE));
    GL(DepthFunc(GL_LEQUAL));

    // Bind 3D camera matrices.
    camera_->Bind3D();

    // Forwards display event to the inheriting application.
    if (success) {
      success = OnDisplay(renderer_.get());
    }
  }  // Ends profiling.

  // Renders grid and axes at the end as they are transparent.
  if (show_grid_) {
    renderer_->DrawGrid(20, 1.f);
  }
  if (show_axes_) {
    renderer_->DrawAxes(ozz::math::Float4x4::identity());
  }

  // Bind 2D camera matrices.
  camera_->Bind2D();

  // Forwards gui event to the inheriting application.
  if (success) {
    success = Gui();
  }

  // Capture back buffer.
  if (capture_screenshot_ || capture_video_) {
    shooter_->Capture(GL_BACK);
    capture_screenshot_ = false;
  }

  // Swaps current window.
  glfwSwapBuffers();

  return success;
}

bool Application::Idle(bool _first_frame) {
  // Early out if displaying help.
  if (show_help_) {
    last_idle_time_ = glfwGetTime();
    return true;
  }

  // Compute elapsed time since last idle, and delta time.
  float delta;
  double time = glfwGetTime();
  if (_first_frame ||  // Don't take into account time spent initializing.
      time == 0.) {    // Means glfw isn't initialized (rendering's disabled).
    delta = 1.f / 60.f;
  } else {
    delta = static_cast<float>(time - last_idle_time_);
  }
  last_idle_time_ = time;

  // Update dt, can be scaled, fixed, freezed...
  float update_delta;
  if (freeze_) {
    update_delta = 0.f;
  } else {
    if (fix_update_rate) {
      update_delta = time_factor_ / fixed_update_rate;
    } else {
      update_delta = delta * time_factor_;
    }
  }

  // Increment current application time
  time_ += update_delta;

  // Forwards update event to the inheriting application.
  bool update_result;
  {  // Profiles update scope.
    Profiler profile(update_time_.get());
    update_result = OnUpdate(update_delta, time_);
  }

  // Updates screen shooter object.
  if (shooter_) {
    shooter_->Update();
  }

  // Update camera model-view matrix.
  if (camera_) {
    math::Box scene_bounds;
    GetSceneBounds(&scene_bounds);

    math::Float4x4 camera_transform;
    if (GetCameraOverride(&camera_transform)) {
      camera_->Update(camera_transform, scene_bounds, delta, _first_frame);
    } else {
      camera_->Update(scene_bounds, delta, _first_frame);
    }
  }

  return update_result;
}

bool Application::Gui() {
  bool success = true;
  const float kFormWidth = 200.f;
  const float kHelpMargin = 16.f;

  // Finds gui area.
  const float kGuiMargin = 2.f;
  ozz::math::RectInt window_rect(0, 0, resolution_.width, resolution_.height);

  // Fills ImGui's input structure.
  internal::ImGuiImpl::Inputs input;
  int mouse_y;
  glfwGetMousePos(&input.mouse_x, &mouse_y);
  input.mouse_y = window_rect.height - mouse_y;
  input.lmb_pressed = glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

  // Starts frame
  im_gui_->BeginFrame(input, window_rect, renderer_.get());

  // Downcast to public imgui.
  ImGui* im_gui = im_gui_.get();
  // Help gui.
  {
    math::RectFloat rect(kGuiMargin, kGuiMargin,
                         window_rect.width - kGuiMargin * 2.f,
                         window_rect.height - kGuiMargin * 2.f);
    // Doesn't constrain form is it's opened, so it covers all screen.
    ImGui::Form form(im_gui, "Show help", rect, &show_help_, !show_help_);
    if (show_help_) {
      im_gui->DoLabel(help_.c_str(), ImGui::kLeft, false);
    }
  }

  // Do framework gui.
  if (!show_help_ && success &&
      window_rect.width > (kGuiMargin + kFormWidth) * 2.f) {
    static bool open = true;
    math::RectFloat rect(kGuiMargin, kGuiMargin, kFormWidth,
                         window_rect.height - kGuiMargin * 2.f - kHelpMargin);
    ImGui::Form form(im_gui, "Framework", rect, &open, true);
    if (open) {
      success = FrameworkGui();
    }
  }

  // Do sample gui.
  if (!show_help_ && success && window_rect.width > kGuiMargin + kFormWidth) {
    static bool open = true;
    math::RectFloat rect(window_rect.width - kFormWidth - kGuiMargin,
                         kGuiMargin, kFormWidth,
                         window_rect.height - kGuiMargin * 2 - kHelpMargin);
    ImGui::Form form(im_gui, "Sample", rect, &open, true);
    if (open) {
      // Forwards event to the inherited application.
      success = OnGui(im_gui);
    }
  }

  // Ends frame
  im_gui_->EndFrame();

  return success;
}

bool Application::FrameworkGui() {
  // Downcast to public imgui.
  ImGui* im_gui = im_gui_.get();
  {  // Render statistics
    static bool open = true;
    ImGui::OpenClose stat_oc(im_gui, "Statistics", &open);
    if (open) {
      char szLabel[64];
      {  // FPS
        Record::Statistics statistics = fps_->GetStatistics();
        std::sprintf(szLabel, "FPS: %.0f",
                     statistics.mean == 0.f ? 0.f : 1000.f / statistics.mean);
        static bool fps_open = false;
        ImGui::OpenClose stats(im_gui, szLabel, &fps_open);
        if (fps_open) {
          std::sprintf(szLabel, "Frame: %.2f ms", statistics.mean);
          im_gui->DoGraph(szLabel, 0.f, statistics.max, statistics.latest,
                          fps_->cursor(), fps_->record_begin(),
                          fps_->record_end());
        }
      }
      {  // Update time
        Record::Statistics statistics = update_time_->GetStatistics();
        std::sprintf(szLabel, "Update: %.2f ms", statistics.mean);
        static bool update_open = true;  // This is the most relevant for ozz.
        ImGui::OpenClose stats(im_gui, szLabel, &update_open);
        if (update_open) {
          im_gui->DoGraph(nullptr, 0.f, statistics.max, statistics.latest,
                          update_time_->cursor(), update_time_->record_begin(),
                          update_time_->record_end());
        }
      }
      {  // Render time
        Record::Statistics statistics = render_time_->GetStatistics();
        std::sprintf(szLabel, "Render: %.2f ms", statistics.mean);
        static bool render_open = false;
        ImGui::OpenClose stats(im_gui, szLabel, &render_open);
        if (render_open) {
          im_gui->DoGraph(nullptr, 0.f, statistics.max, statistics.latest,
                          render_time_->cursor(), render_time_->record_begin(),
                          render_time_->record_end());
        }
      }
    }
  }

  {  // Time control
    static bool open = false;
    ImGui::OpenClose stats(im_gui, "Time control", &open);
    if (open) {
      im_gui->DoButton("Freeze", true, &freeze_);
      im_gui->DoCheckBox("Fix update rate", &fix_update_rate, true);
      if (!fix_update_rate) {
        char sz_factor[64];
        std::sprintf(sz_factor, "Time factor: %.2f", time_factor_);
        im_gui->DoSlider(sz_factor, -5.f, 5.f, &time_factor_);
        if (im_gui->DoButton("Reset time factor", time_factor_ != 1.f)) {
          time_factor_ = 1.f;
        }
      } else {
        char sz_fixed_update_rate[64];
        std::sprintf(sz_fixed_update_rate, "Update rate: %.0f fps",
                     fixed_update_rate);
        im_gui->DoSlider(sz_fixed_update_rate, 1.f, 200.f, &fixed_update_rate,
                         .5f, true);
        if (im_gui->DoButton("Reset update rate", fixed_update_rate != 60.f)) {
          fixed_update_rate = 60.f;
        }
      }
    }
  }

  {  // Rendering options
    static bool open = false;
    ImGui::OpenClose options(im_gui, "Options", &open);
    if (open) {
      // Multi-sampling.
      static bool fsaa_available = glfwGetWindowParam(GLFW_FSAA_SAMPLES) != 0;
      static bool fsaa_enabled = fsaa_available;
      if (im_gui->DoCheckBox("Anti-aliasing", &fsaa_enabled, fsaa_available)) {
        if (fsaa_enabled) {
          GL(Enable(GL_MULTISAMPLE));
        } else {
          GL(Disable(GL_MULTISAMPLE));
        }
      }
      // Vertical sync
      static bool vertical_sync_ = true;  // On by default.
      if (im_gui->DoCheckBox("Vertical sync", &vertical_sync_, true)) {
        glfwSwapInterval(vertical_sync_ ? 1 : 0);
      }

      im_gui->DoCheckBox("Show grid", &show_grid_, true);
      im_gui->DoCheckBox("Show axes", &show_axes_, true);
    }

    // Searches for matching resolution settings.
    int preset_lookup = 0;
    for (; preset_lookup < kNumPresets - 1; ++preset_lookup) {
      const Resolution& preset = resolution_presets[preset_lookup];
      if (preset.width > resolution_.width) {
        break;
      } else if (preset.width == resolution_.width) {
        if (preset.height >= resolution_.height) {
          break;
        }
      }
    }

    char szResolution[64];
    std::sprintf(szResolution, "Resolution: %dx%d", resolution_.width,
                 resolution_.height);
    if (im_gui->DoSlider(szResolution, 0, kNumPresets - 1, &preset_lookup)) {
      // Resolution changed.
      resolution_ = resolution_presets[preset_lookup];
      glfwSetWindowSize(resolution_.width, resolution_.height);
    }
  }

  {  // Capture
    static bool open = false;
    ImGui::OpenClose controls(im_gui, "Capture", &open);
    if (open) {
      im_gui->DoButton("Capture video", true, &capture_video_);
      capture_screenshot_ |= im_gui->DoButton(
          "Capture screenshot", !capture_video_, &capture_screenshot_);
    }
  }

  {  // Controls
    static bool open = false;
    ImGui::OpenClose controls(im_gui, "Camera controls", &open);
    if (open) {
      camera_->OnGui(im_gui);
    }
  }
  return true;
}

bool Application::GetCameraInitialSetup(math::Float3* _center,
                                        math::Float2* _angles,
                                        float* _distance) const {
  (void)_center;
  (void)_angles;
  (void)_distance;
  return false;
}

// Default implementation doesn't override camera location.
bool Application::GetCameraOverride(math::Float4x4* _transform) const {
  (void)_transform;
  assert(_transform);
  return false;
}

void Application::ResizeCbk(int _width, int _height) {
  // Stores new resolution settings.
  application_->resolution_.width = _width;
  application_->resolution_.height = _height;

  // Uses the full viewport.
  GL(Viewport(0, 0, _width, _height));

  // Forwards screen size to camera and shooter.
  application_->camera_->Resize(_width, _height);
  application_->shooter_->Resize(_width, _height);
}

int Application::CloseCbk() {
  application_->exit_ = true;
  return GL_FALSE;  // The window will be closed while exiting the main loop.
}

void Application::ParseReadme() {
  const char* error_message = "Unable to find README.md help file.";

  // Get README file, opens as binary to avoid conversions.
  ozz::io::File file("README.md", "rb");
  if (!file.opened()) {
    help_ = error_message;
    return;
  }

  // Allocate enough space to store the whole file.
  const size_t read_length = file.Size();
  ozz::memory::Allocator* allocator = ozz::memory::default_allocator();
  char* content = reinterpret_cast<char*>(allocator->Allocate(read_length, 4));

  // Read the content
  if (file.Read(content, read_length) == read_length) {
    help_ = ozz::string(content, content + read_length);
  } else {
    help_ = error_message;
  }

  // Deallocate temporary buffer;
  allocator->Deallocate(content);
}
}  // namespace sample
}  // namespace ozz
