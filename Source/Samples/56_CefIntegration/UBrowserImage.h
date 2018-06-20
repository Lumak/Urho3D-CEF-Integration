//
// Copyright (c) 2008-2016 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include <Urho3D/UI/Window.h>
#include <Urho3D/UI/BorderImage.h>
#include <Urho3D/Container/ArrayPtr.h>

#include <cef_render_handler.h>

namespace Urho3D
{
class Texture2D;
}

using namespace Urho3D;
class CefAppThread;

//=============================================================================
//=============================================================================
#define CEFBUF_WIDTH            1100
#define CEFBUF_HEIGHT           700
#define CEFBUF_COMPONENTS       4

#define BROWSER_RENDER_WIDTH    640
#define BROWSER_RENDER_HEIGTH   480

//=============================================================================
//=============================================================================
class UCefRenderHandle : public CefRenderHandler
{
    IMPLEMENT_REFCOUNTING(UCefRenderHandle);
public:

    UCefRenderHandle(int width, int height, unsigned components);
    virtual ~UCefRenderHandle();

    // cef virtual overrides
    virtual bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect);
    virtual void OnPaint(CefRefPtr<CefBrowser> browser,
                         PaintElementType ttype,
                         const RectList& dirtyRects,
                         const void* buffer,
                         int width, int height);

    void Resize(int width, int height);
    void CopyBuffer(void *dst, void *src, unsigned usize);
    void CopyToTexture(Texture2D *texture);
    bool IsUpdated()const   { return bufferUpdated_; }
    void Shutdown();
    bool IsShuttingDown();

    CefRefPtr<CefBrowser> browser_;

protected:
    SharedArrayPtr<unsigned char> copyBuffer_;

    int width_;
    int height_;
    unsigned components_;

    bool bufferUpdated_;
    bool isShuttingDown_;

    Mutex copyMutex_;

    // dbg for cpy
    HiresTimer htimer_;

    Timer copyTimer_;
};

//=============================================================================
//=============================================================================
class UBrowserImage : public BorderImage
{
    URHO3D_OBJECT(UBrowserImage, BorderImage);
public:

    UBrowserImage(Context *context);
    ~UBrowserImage();

    void Init(UCefRenderHandle *cefRenderHandler, int width, int height);
    void ClearCefHandler();
protected:
    void InitTexture(int width, int height);
    void UpdateBuffer();

    bool IsAppReady() const;
    void RegisterHandlers();
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    // renderer
    void HandleScreenMode(StringHash eventType, VariantMap& eventData);
    //void HandleBeginFrame(StringHash eventType, VariantMap& eventData);
    //void HandlePostUpdate(StringHash eventType, VariantMap& eventData);
    //void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);

    virtual void OnHover(const IntVector2& position, const IntVector2& screenPosition, int buttons, int qualifiers, Cursor* cursor);
    virtual void OnClickBegin(const IntVector2& position, const IntVector2& screenPosition, 
                              int button, int buttons, int qualifiers, Cursor* cursor);
    /// React to mouse click end.
    virtual void OnClickEnd(const IntVector2& position, const IntVector2& screenPosition, 
                            int button, int buttons, int qualifiers, Cursor* cursor, UIElement* beginElement);

    // inputs
    void HandleMouseWheel(StringHash eventType, VariantMap& eventData);
    void HandleFocusChanged(StringHash eventType, VariantMap& eventData);
    void HandleKeyDown(StringHash eventType, VariantMap& eventData);
    void HandleKeyUp(StringHash eventType, VariantMap& eventData);
    void HandleTextInput(StringHash eventType, VariantMap& eventData);

    CefMouseEvent GetCefMoustEvent(int qualifiers);
    unsigned GetKeyModifiers(int qualifiers);

protected:
    CefRefPtr<CefBrowser>       cefBrowser_;
    CefRefPtr<UCefRenderHandle> cefRendererHandle_;
    SharedPtr<Texture2D>        texture_;

    int width_;
    int height_;
    Timer copyTimer_;

    // interface
    IntVector2  lastMousePos_;
    IntVector2  initalOffset_;
    Vector2     scaleDiff_;

};












