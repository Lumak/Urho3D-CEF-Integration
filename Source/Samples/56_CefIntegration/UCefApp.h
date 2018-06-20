// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include <Urho3D/Core/Object.h>
#include "cefsimple/simple_app.h"

using namespace Urho3D;

class UCefRenderHandle;
class UBrowserImage;

//=============================================================================
//=============================================================================
class UCefApp : public Object
{
    URHO3D_OBJECT(UCefApp, Object);
public:
    UCefApp(Context *context);
    virtual ~UCefApp();

    int CreateAppBrowser();
    void DestroyAppBrowser();

protected:
    UCefRenderHandle         *uCefRenderHandler_;
    SharedPtr<UBrowserImage> uBrowserImage_;
};

