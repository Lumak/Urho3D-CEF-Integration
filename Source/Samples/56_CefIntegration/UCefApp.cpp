// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Mutex.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/BorderImage.h>
#include <SDL/SDL_log.h>

#include "UCefApp.h"
#include "UBrowserImage.h"
#include "cefsimple/simple_app.h"

#include <Urho3D/DebugNew.h>

//=============================================================================
//=============================================================================
UCefApp::UCefApp(Context *context)
    : Object(context)
    , uBrowserImage_(NULL)
    , uCefRenderHandler_(NULL)
{
}

UCefApp::~UCefApp()
{
    uBrowserImage_ = NULL;
}

int UCefApp::CreateAppBrowser()
{
    UI* ui = GetSubsystem<UI>();
    uBrowserImage_ = new UBrowserImage(context_);
    ui->GetRoot()->AddChild(uBrowserImage_);

    uCefRenderHandler_ = new UCefRenderHandle(CEFBUF_WIDTH, CEFBUF_HEIGHT, CEFBUF_COMPONENTS);
    uBrowserImage_->Init(uCefRenderHandler_, BROWSER_RENDER_WIDTH, BROWSER_RENDER_HEIGTH);

    CefMainArgs main_args(NULL);

    // Specify CEF global settings here.
    CefSettings settings;
    settings.multi_threaded_message_loop = true;
    settings.windowless_rendering_enabled = true;

    CefRefPtr<SimpleHandler> simpHandler = new SimpleHandler((CefRenderHandler *)uCefRenderHandler_);

    // SimpleApp implements application-level callbacks for the browser process.
    // It will create the first browser instance in OnContextInitialized() after
    // CEF has initialized.
    CefRefPtr<SimpleApp> sApp = new SimpleApp(simpHandler);

    // Initialize CEF.
    CefInitialize(main_args, settings, sApp.get(), NULL);

    return 0;
}

void UCefApp::DestroyAppBrowser()
{
    // flag the handler from copying its buffer
    if ( uCefRenderHandler_ )
    {
        uCefRenderHandler_->Shutdown();
    }

    // reference from: cef_life_span_handler.h
    // An application should handle top-level owner window close notifications by
    // calling CefBrowserHost::TryCloseBrowser() or
    // CefBrowserHost::CloseBrowser(false) instead of allowing the window to close
    // immediately (see the examples below). This gives CEF an opportunity to
    // process the 'onbeforeunload' event and optionally cancel the close before
    // DoClose() is called.
    // 1.  User clicks the window close button which sends a close notification to
    //     the application's top-level window.
    // . . . 
    // 9.  Application's top-level window is destroyed.
    // 10. Application's OnBeforeClose() handler is called and the browser object is destroyed.

    SimpleHandler::GetInstance()->CloseAllBrowsers(false);

    int itr = 0;
    while ( !SimpleHandler::GetInstance()->OnBeforeCloseWasCalled() )
    {
        itr++;
        Time::Sleep(10);
    }

    SDL_Log( "onBeforeClose itr = %d", itr );

    if ( uBrowserImage_ )
    {
        uBrowserImage_->ClearCefHandler();
        uBrowserImage_->Remove();
        uBrowserImage_ = NULL;
    }

    Time::Sleep(10);
}



