// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "cefsimple/simple_app.h"

#include <string>

#include "cefsimple/simple_handler.h"
#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"

#if 0
namespace {
// When using the Views framework this object provides the delegate
// implementation for the CefWindow that hosts the Views-based browser.
class SimpleWindowDelegate : public CefWindowDelegate {
 public:
  explicit SimpleWindowDelegate(CefRefPtr<CefBrowserView> browser_view)
      : browser_view_(browser_view) {
  }

  void OnWindowCreated(CefRefPtr<CefWindow> window) OVERRIDE {
    // Add the browser view and show the window.
    window->AddChildView(browser_view_);
    window->Show();

    // Give keyboard focus to the browser view.
    browser_view_->RequestFocus();
  }

  void OnWindowDestroyed(CefRefPtr<CefWindow> window) OVERRIDE {
    browser_view_ = NULL;
  }

  bool CanClose(CefRefPtr<CefWindow> window) OVERRIDE {
    // Allow the window to close if the browser says it's OK.
    CefRefPtr<CefBrowser> browser = browser_view_->GetBrowser();
    if (browser)
      return browser->GetHost()->TryCloseBrowser();
    return true;
  }

 private:
  CefRefPtr<CefBrowserView> browser_view_;

  IMPLEMENT_REFCOUNTING(SimpleWindowDelegate);
  DISALLOW_COPY_AND_ASSIGN(SimpleWindowDelegate);
};

}  // namespace
#endif

SimpleApp::SimpleApp(SimpleHandler *simpleHandler) 
    : simpHandler_(simpleHandler)
    , bSyncBrowser_(false)
{
}

SimpleApp::~SimpleApp()
{
    simpHandler_ = NULL;
}

void SimpleApp::OnBeforeCommandLineProcessing(const CefString& process_type, CefRefPtr<CefCommandLine> command_line)
{
  // Pass additional command-line flags to the browser process.
  if (process_type.empty()) 
  {
    // Pass additional command-line flags when off-screen rendering is enabled.
      if (!command_line->HasSwitch("off-screen-rendering-enabled")) 
          command_line->AppendSwitch("off-screen-rendering-enabled");
      if (!command_line->HasSwitch("disable-extensions") )
          command_line->AppendSwitch("disable-extensions");

    if (command_line->HasSwitch("off-screen-rendering-enabled")) 
    {
      // If the PDF extension is enabled then cc Surfaces must be disabled for
      // PDFs to render correctly.
      // See https://bitbucket.org/chromiumembedded/cef/issues/1689 for details.
      if (!command_line->HasSwitch("disable-extensions") &&
          !command_line->HasSwitch("disable-pdf-extension")) {
        command_line->AppendSwitch("disable-surfaces");
      }

      // Use software rendering and compositing (disable GPU) for increased FPS
      // and decreased CPU usage. This will also disable WebGL so remove these
      // switches if you need that capability.
      // See https://bitbucket.org/chromiumembedded/cef/issues/1257 for details.
      if (!command_line->HasSwitch("enable-gpu")) 
      {
        command_line->AppendSwitch("disable-gpu");
        command_line->AppendSwitch("disable-gpu-compositing");
      }

      // Synchronize the frame rate between all processes. This results in
      // decreased CPU usage by avoiding the generation of extra frames that
      // would otherwise be discarded. The frame rate can be set at browser
      // creation time via CefBrowserSettings.windowless_frame_rate or changed
      // dynamically using CefBrowserHost::SetWindowlessFrameRate. In cefclient
      // it can be set via the command-line using `--off-screen-frame-rate=XX`.
      // See https://bitbucket.org/chromiumembedded/cef/issues/1368 for details.
      command_line->AppendSwitch("enable-begin-frame-scheduling");
    }

    if (command_line->HasSwitch("use-views") &&
        !command_line->HasSwitch("top-chrome-md")) {
      // Use non-material mode on all platforms by default. Among other things
      // this causes menu buttons to show hover state. See usage of
      // MaterialDesignController::IsModeMaterial() in Chromium code.
      command_line->AppendSwitchWithValue("top-chrome-md", "non-material");
    }
  }
}

void SimpleApp::OnContextInitialized() 
{
    CEF_REQUIRE_UI_THREAD();

    CefRefPtr<CefCommandLine> command_line =
        CefCommandLine::GetGlobalCommandLine();

    // Specify CEF browser settings here.
    CefBrowserSettings browser_settings;

    std::string url;

    // Check if a "--url=" value was provided via the command-line. If so, use
    // that instead of the default URL.
    url = command_line->GetSwitchValue("url");
    if (url.empty())
    {
        //url = "http://www.google.com";
        //url = "https://www.youtube.com/watch?v=-fmCoUjOMXU";
        url = "https://www.youtube.com/watch?v=2u5ReExUPas&index=1";
    }

    // Information used when creating the native window.
    CefWindowInfo window_info;

    // LUMAK: change to windowless and browser sync
    window_info.SetAsWindowless(NULL, false);

    // Async browser creation works from any thread (CefBrowserHost.CreateBrowser).
    // Sync browser creation can be performed only on browser's UI thread (CEF UI thread).
    if ( bSyncBrowser_ )
    {
        syncBrowser_ = CefBrowserHost::CreateBrowserSync(window_info, simpHandler_, url, browser_settings, NULL);
    }
    else
    {
        CefBrowserHost::CreateBrowser(window_info, simpHandler_, url, browser_settings, NULL);
    }
}

void SimpleApp::AppCloseBrowser()
{
    //syncBrowser_->CloseBrowser(true);
}

