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

#ifndef OZZ_SAMPLES_FRAMEWORK_APPLICATION_H_
#define OZZ_SAMPLES_FRAMEWORK_APPLICATION_H_

#include <cstddef>
#include "ozz/base/containers/string.h"
#include "ozz/base/memory/unique_ptr.h"

namespace ozz {
namespace math {
struct Box;
struct Float2;
struct Float3;
struct Float4x4;
}  // namespace math
namespace sample {

class ImGui;
class Renderer;
class Record;

namespace internal {
class ImGuiImpl;
class RendererImpl;
class Camera;
class Shooter;
}  // namespace internal

// Screen resolution settings.
struct Resolution {
  int width;
  int height;
};

class Application {
 public:
  // Creates a window and initialize GL context.
  // Any failure during initialization or loop execution will be silently
  // handled, until the call to ::Run that will return EXIT_FAILURE.
  Application();

  // Destroys the application. Cleans up everything.
  virtual ~Application();

  // Runs application main loop.
  // Caller must provide main function arguments, as well as application version
  // and usage strings.
  // Returns EXIT_SUCCESS if the application exits due to user request, or
  // EXIT_FAILURE if an error occurred during initialization or the main loop.
  // Only one application can be run at a time, otherwise EXIT_FAILURE is
  // returned.
  int Run(int _argc, const char** _argv, const char* _version,
          const char* _title);

 private:
  // Provides initialization event to the inheriting application. Called while
  // the help screen is being displayed.
  // OnInitialize can return false which will in turn skip the display loop and
  // exit the application with EXIT_FAILURE. Note that OnDestroy is called in
  // any case.
  virtual bool OnInitialize() = 0;

  // Provides de-initialization event to the inheriting application.
  // OnDestroy is called even if OnInitialize failed and returned an error.
  virtual void OnDestroy() = 0;

  // Provides update event to the inheriting application.
  // _dt is the elapsed time (in seconds) since the last update.
  // _time is application time including scaling (aka accumulated _dt).
  // OnUpdate can return false which will in turn stop the loop and exit the
  // application with EXIT_FAILURE. Note that OnDestroy is called in any case.
  virtual bool OnUpdate(float _dt, float _time) = 0;

  // Provides immediate mode gui display event to the inheriting application.
  // This function is called in between the OnDisplay and swap functions.
  // OnGui can return false which will in turn stop the loop and exit the
  // application with EXIT_FAILURE. Note that OnDestroy is called in any case.
  virtual bool OnGui(ImGui* _im_gui) = 0;

  // Provides display event to the inheriting application.
  // This function is called in between the clear and swap functions.
  // OnDisplay can return false which will in turn stop the loop and exit the
  // application with EXIT_FAILURE. Note that OnDestroy is called in any case.
  virtual bool OnDisplay(Renderer* _renderer) = 0;

  // Initial camera values. These will only be considered if function returns
  // true;
  virtual bool GetCameraInitialSetup(math::Float3* _center,
                                     math::Float2* _angles,
                                     float* _distance) const;

  // Allows the inheriting application to override camera location.
  // Application should return true (false by default) if it wants to override
  // Camera location, and fills in this case _transform matrix.
  // This function is never called before a first OnUpdate.
  virtual bool GetCameraOverride(math::Float4x4* _transform) const;

  // Requires the inheriting application to provide scene bounds. It is used by
  // the camera to frame all the scene.
  // This function is never called before a first OnUpdate.
  // If _bound is set to "invalid", then camera won't be updated.
  virtual void GetSceneBounds(math::Box* _bound) const = 0;

  // Implements framework internal loop function.
  bool Loop();

  // This callback has to forward loop call to OneLoop private function.
  friend void OneLoopCbk(void*);

  // Implements framework internal one iteration loop function.
  enum LoopStatus {
    kContinue,      // Can continue with next loop.
    kBreak,         // Should stop looping (ex: exit).
    kBreakFailure,  // // Should stop looping beacause something went wrong.
  };
  LoopStatus OneLoop(int _loops);

  // Implements framework internal idle function.
  // Returns the value returned by OnIdle or from an internal issue.
  bool Idle(bool _first_frame);

  // Implements framework internal display callback.
  // Returns the value returned by OnDisplay or from an internal issue.
  bool Display();

  // Implements framework internal gui callback.
  // Returns the value returned by OnGui or from an internal error.
  bool Gui();

  // Implements framework gui rendering.
  bool FrameworkGui();

  // Implements framework glfw window reshape callback.
  static void ResizeCbk(int _width, int _height);

  // Implements framework glfw window close callback.
  static int CloseCbk();

  // Get README.md for content to display it in the help ui.
  void ParseReadme();

  // Disallow copy and assignment.
  Application(const Application& _application);
  void operator=(const Application& _application);

  // A pointer to the current, and only, running application.
  static Application* application_;

  // Application exit request.
  bool exit_;

  // Update time freeze state.
  bool freeze_;

  // Fixes update rat to a fixed value, instead of real_time.
  bool fix_update_rate;

  // Fixed update rate, only applies to application update dt, not the real fps.
  float fixed_update_rate;

  // Update time scale factor.
  float time_factor_;

  // Current application time, including scaling and freezes..
  float time_;

  // Last time the idle function was called, in seconds.
  // This is a double value in order to maintain enough accuracy when the
  // application is running since a long time.
  double last_idle_time_;

  // The camera object used by the application.
  unique_ptr<internal::Camera> camera_;

  // The screen shooter object used by the application.
  unique_ptr<internal::Shooter> shooter_;

  // Set to true to display help.
  bool show_help_;

  // Grid display settings.
  bool show_grid_;
  bool show_axes_;

  // Capture settings.
  bool capture_video_;
  bool capture_screenshot_;

  // The renderer utility object used by the application.
  unique_ptr<internal::RendererImpl> renderer_;

  // Immediate mode gui interface.
  unique_ptr<internal::ImGuiImpl> im_gui_;

  // Timing records.
  unique_ptr<Record> fps_;
  unique_ptr<Record> update_time_;
  unique_ptr<Record> render_time_;

  // Current screen resolution.
  Resolution resolution_;

  // Help message.
  ozz::string help_;
};
}  // namespace sample
}  // namespace ozz
#endif  // OZZ_SAMPLES_FRAMEWORK_APPLICATION_H_
