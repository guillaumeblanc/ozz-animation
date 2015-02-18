//============================================================================//
//                                                                            //
// ozz-animation, 3d skeletal animation libraries and tools.                  //
// https://code.google.com/p/ozz-animation/                                   //
//                                                                            //
//----------------------------------------------------------------------------//
//                                                                            //
// Copyright (c) 2012-2015 Guillaume Blanc                                    //
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

#ifndef OZZ_SAMPLES_FRAMEWORK_APPLICATION_H_
#define OZZ_SAMPLES_FRAMEWORK_APPLICATION_H_

#include <cstddef>
#include "ozz/base/containers/string.h"

namespace ozz {
namespace math {struct Box;}
namespace sample {

class ImGui;
class Renderer;
class Record;

namespace internal {
class RendererImpl;
class Camera;
class Shooter;
}  // internal

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
  int Run(int _argc, const char** _argv,
          const char* _version, const char* _title);

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
  // Argument _dt is the elapsed time (in seconds) since the last update.
  // OnUpdate can return false which will in turn stop the loop and exit the
  // application with EXIT_FAILURE. Note that OnDestroy is called in any case.
  virtual bool OnUpdate(float _dt) = 0;

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

  // Requires the inheriting application to provide scene bounds. It is used by
  // the camera to frame all the scene.
  // This function is never called before a first OnUpdate.
  virtual void GetSceneBounds(math::Box* _bound) const = 0;

  // Implements framework internal loop function.
  bool Loop();

  // This callback has to forward loop call to OneLoop private function.
  friend void OneLoopCbk(void*);

  // Implements framework internal one iteration loop function.
  enum LoopStatus {
    kContinue,  // Can continue with next loop.
    kBreak,  // Should stop looping (ex: exit).
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

  // Get README for content to display it in the help ui.
  void ParseReadme();

  // Disallow copy and assignment.
  Application(const Application& _application);
  void operator = (const Application& _application);

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

  // Last time the idle function was called, in seconds.
  // This is a double value in order to maintain enough accuracy when the
  // application is running since a long time.
  double last_idle_time_;

  // The camera object used by the application.
  internal::Camera* camera_;

  // The screen shooter object used by the application.
  internal::Shooter* shooter_;

  // Set to true to display help.
  bool show_help_;

  // Capture settings.
  bool capture_video_;
  bool capture_screenshot_;

  // The renderer utility object used by the application.
  internal::RendererImpl* renderer_;

  // Immediate mode gui interface.
  ImGui* im_gui_;

  // Timing records.
  Record* fps_;
  Record* update_time_;
  Record* render_time_;

  // Current screen resolution.
  Resolution resolution_;

  // Help message.
  ozz::String::Std help_;
};
}  // sample
}  // ozz
#endif  // OZZ_SAMPLES_FRAMEWORK_APPLICATION_H_
