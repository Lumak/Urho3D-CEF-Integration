// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include <Urho3D/Core/Timer.h>
using namespace Urho3D;

#include "cefsimple/simple_handler.h"

#include <sstream>
#include <string>

#include "include/base/cef_bind.h"
#include "include/cef_app.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"

#include <SDL/SDL_log.h>

namespace {

SimpleHandler* g_instance = NULL;

}  // namespace

SimpleHandler::SimpleHandler(CefRenderHandler *cefRenderHandler)
    : cefRenderHandler_(cefRenderHandler)
    , use_views_(false)
    , is_closing_(false)
    , onLoadEnded_(false)
    , messageLoopStarted_(false)
    , onBeforeCloseCalled_(false)
{
  DCHECK(!g_instance);
  g_instance = this;
}

SimpleHandler::~SimpleHandler() 
{
  cefRenderHandler_ = NULL;
  g_instance = NULL;
}

// static
SimpleHandler* SimpleHandler::GetInstance() 
{
  return g_instance;
}
void SimpleHandler::ClearInstance()
{
    g_instance = NULL;
}
void SimpleHandler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) 
{
    CEF_REQUIRE_UI_THREAD();

    // Set the title of the window using platform APIs.
    PlatformTitleChange(browser, title);
}

void SimpleHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) 
{
    CEF_REQUIRE_UI_THREAD();

    // Add to the list of existing browsers.
    browser_list_.push_back(browser);
}

bool SimpleHandler::DoClose(CefRefPtr<CefBrowser> browser) 
{
    CEF_REQUIRE_UI_THREAD();

    // Closing the main window requires special handling. See the DoClose()
    // documentation in the CEF header for a detailed destription of this
    // process.
    if (browser_list_.size() == 1) 
    {
        // Set a flag to indicate that the window close should be allowed.
        is_closing_ = true;
    }

    // Allow the close. For windowed browsers this will result in the OS close
    // event being sent.
    return false;
}

void SimpleHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) 
{
    CEF_REQUIRE_UI_THREAD();

    // Remove from the list of existing browsers.
    BrowserList::iterator bit = browser_list_.begin();
    for (; bit != browser_list_.end(); ++bit) 
    {
        if ((*bit)->IsSame(browser)) 
        {
            browser_list_.erase(bit);
            break;
        }
    }

    if (browser_list_.empty()) 
    {
        onBeforeCloseCalled_ = true;

        // All browser windows have closed. Quit the application message loop.
        if ( messageLoopStarted_ )
            CefQuitMessageLoop();
    }
}

bool SimpleHandler::OnBeforeCloseWasCalled()
{
    return onBeforeCloseCalled_;
}

void SimpleHandler::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString& errorText,
                                const CefString& failedUrl) 
{
    CEF_REQUIRE_UI_THREAD();

    // Don't display an error for downloaded files.
    if (errorCode == ERR_ABORTED)
        return;

    // Display a load error message.
    std::stringstream ss;
        ss << "<html><body bgcolor=\"white\">"
            "<h2>Failed to load URL " << std::string(failedUrl) <<
            " with error " << std::string(errorText) << " (" << errorCode <<
            ").</h2></body></html>";
    frame->LoadString(ss.str(), failedUrl);
}

void SimpleHandler::OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode)
{
    onLoadEnded_ = true;
}

void SimpleHandler::CloseAllBrowsers(bool force_close) 
{
    if (!CefCurrentlyOn(TID_UI))
    {
        // Execute on the UI thread.
        CefPostTask(TID_UI, base::Bind(&SimpleHandler::CloseAllBrowsers, this, force_close));
        return;
    }

    if (browser_list_.empty())
        return;

    // copy the browser list to a temp list
    BrowserList tmpList;
    tmpList.clear();
    BrowserList::const_iterator it1 = browser_list_.begin();
    for ( ; it1 != browser_list_.end(); ++it1)
        tmpList.push_back(*it1);


    BrowserList::const_iterator it = tmpList.begin();
    for ( ; it != tmpList.end(); ++it)
        (*it)->GetHost()->CloseBrowser(force_close);
}

bool SimpleHandler::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser,
                                             CefProcessId source_process,
                                             CefRefPtr<CefProcessMessage> message) 
{
    std::string strname = message->GetName().ToString();
    SDL_Log("SH:onProcMsgRcv - %s", strname.c_str() );
    return false;
}

