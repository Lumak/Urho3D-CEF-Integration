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

#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/BorderImage.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>
#include <fstream>
#include <SDL/SDL_log.h>

#include "UBrowserImage.h"
#include "UCefApp.h"

#include <Urho3D/DebugNew.h>

//=============================================================================
//=============================================================================
#define FRAME_RATE_MS           32
#define MOUSE_WHEEL_MULTIPLYER  30.0f

//=============================================================================
//=============================================================================
UCefRenderHandle::UCefRenderHandle(int width, int height, unsigned components)
    : width_(width)
    , height_(height)
    , components_(components)
    , bufferUpdated_(false)
    , browser_(NULL)
    , isShuttingDown_(false)
{
    copyBuffer_ = new unsigned char[width*height*components];
}

UCefRenderHandle::~UCefRenderHandle()
{
    copyBuffer_ = NULL;
    browser_ = NULL;
}

bool UCefRenderHandle::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect)
{
    rect = CefRect(0, 0, width_, height_);
    return true;
}

void UCefRenderHandle::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType ttype, 
                               const RectList& dirtyRects, const void* buffer, int width, int height)
{
    if ( IsShuttingDown() )
    {
        return;
    }

    if ( copyTimer_.GetMSec(false) < FRAME_RATE_MS )
    {
        return;
    }

    if ( width != width_ || height != height_ )
    {
        MutexLock lock(copyMutex_);

        width_ = width;
        height_ = height;
        copyBuffer_ = new unsigned char[width_*height_*components_];
    }

    if ( browser_ == NULL )
    {
        browser_ = browser;
    }

    CopyBuffer(copyBuffer_.Get(), (void*)buffer, width_*height_*components_);

    copyTimer_.Reset();
}

void UCefRenderHandle::Resize(int width, int height)
{
    if ( width != width_ || height != height_ )
    {
        MutexLock lock(copyMutex_);

        width_ = width;
        height_ = height;
        copyBuffer_ = new unsigned char[width_*height_*components_];
    }
}

void UCefRenderHandle::CopyBuffer(void *dst, void *src, unsigned usize)
{
    MutexLock lock(copyMutex_);
    //HiresTimer htimer;

    struct CefColor
    {
        unsigned char r_, g_, b_, a_;
    };

    #ifdef INDIVIDUAL_RGBA_CPY
    CefColor *fsrc = (CefColor*)src;
    CefColor *fdst = (CefColor*)dst;

    for ( unsigned i = 0; i < usize/sizeof(CefColor); ++i )
    {
        fdst[i].r_ = fsrc[i].b_;
        fdst[i].g_ = fsrc[i].g_;
        fdst[i].b_ = fsrc[i].r_;
        fdst[i].a_ = fsrc[i].a_;
    }
    //SDL_Log("rgba cp = %I64d", htimer.GetUSec(false) );

    #else
    // doing mempcy first then doing the r-b swap in shared ptr 
    // array is about 20%-40% faster than individual rgba cpy
    // -sample output data (in usecs):
    //INFO: rgba cp = 12581
    //INFO: rgba cp = 16775
    //INFO: rgba cp = 18151
    //INFO: rgba cp = 19887
    //INFO: memcpy - rb swap = 11272
    //INFO: memcpy - rb swap = 12205
    //INFO: memcpy - rb swap = 14512
    //INFO: memcpy - rb swap = 7541

    memcpy(dst, src, usize);

    CefColor *fdst = (CefColor*)dst;

    for ( unsigned i = 0; i < usize/sizeof(CefColor); ++i )
    {
        unsigned char tmp = fdst[i].r_;
        fdst[i].r_ = fdst[i].b_;
        fdst[i].b_ = tmp;
    }
    //SDL_Log("memcpy - rb swap = %I64d", htimer.GetUSec(false) );
    #endif

    bufferUpdated_ = true;
}

void UCefRenderHandle::CopyToTexture(Texture2D *texture)
{
    MutexLock lock(copyMutex_);

    if ( bufferUpdated_ )
    {
        texture->SetData(0, 0, 0, width_, height_, copyBuffer_.Get());
        bufferUpdated_ = false;
    }
}

void UCefRenderHandle::Shutdown()
{ 
    isShuttingDown_ = true; 
}

bool UCefRenderHandle::IsShuttingDown()
{
    return isShuttingDown_;
}

//=============================================================================
//=============================================================================
UBrowserImage::UBrowserImage(Context *context)
    : BorderImage(context)
    , cefBrowser_(NULL)
    , cefRendererHandle_(NULL)
{
}

UBrowserImage::~UBrowserImage()
{
    cefRendererHandle_ = NULL;
    cefBrowser_ = NULL;
}

void UBrowserImage::ClearCefHandler()
{
    if ( cefRendererHandle_ )
    {
        cefRendererHandle_->Shutdown();
        cefRendererHandle_->browser_ = NULL;
    }

    if ( cefBrowser_ )
    {
        cefBrowser_ = NULL;
    }
}

void UBrowserImage::Init(UCefRenderHandle *cefRenderHandler, int width, int height)
{
    InitTexture(width, height);

    cefRendererHandle_ = cefRenderHandler;

    RegisterHandlers();
}

void UBrowserImage::InitTexture(int width, int height)
{
    texture_ = new Texture2D(context_);
    
    // set texture format
    texture_->SetMipsToSkip(QUALITY_LOW, 0);
    texture_->SetNumLevels(1);
    texture_->SetSize(CEFBUF_WIDTH, CEFBUF_HEIGHT, Graphics::GetRGBAFormat());
    
    // set modes
    texture_->SetFilterMode(FILTER_BILINEAR);
    texture_->SetAddressMode(COORD_U, ADDRESS_CLAMP);
    texture_->SetAddressMode(COORD_V, ADDRESS_CLAMP);

    width_ = width;
    height_ = height;

    scaleDiff_ = Vector2( (float)CEFBUF_WIDTH/(float)width_, (float)CEFBUF_HEIGHT/(float)height_ );
    initalOffset_ = IntVector2(20, 20);

    // init 
    SetVisible(false);
    SetPosition(initalOffset_);
    SetTexture(texture_);
    SetSize(width_, height_);
    SetEnabled(true);
    SetOpacity(0.95f);
}

void UBrowserImage::UpdateBuffer()
{
    // check timer before appready
    if ( copyTimer_.GetMSec(false) < FRAME_RATE_MS || !IsAppReady())
    {
        return;
    }

    // copy buffer
    cefRendererHandle_->CopyToTexture( texture_ );
    copyTimer_.Reset();

    cefBrowser_ = cefRendererHandle_->browser_;

    if ( !IsVisible() )
    {
        SetVisible(true);
    }
}

bool UBrowserImage::IsAppReady() const
{
    return ( cefRendererHandle_ && cefRendererHandle_->IsUpdated() );
}

void UBrowserImage::RegisterHandlers()
{
    // timer
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(UBrowserImage, HandleUpdate));

    // screen resize and renderer
    SubscribeToEvent(E_SCREENMODE, URHO3D_HANDLER(UBrowserImage, HandleScreenMode));

    // mouse
    SubscribeToEvent(E_FOCUSCHANGED, URHO3D_HANDLER(UBrowserImage, HandleFocusChanged));
    SubscribeToEvent(E_MOUSEWHEEL, URHO3D_HANDLER(UBrowserImage, HandleMouseWheel));

    // keys
    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(UBrowserImage, HandleKeyDown));
    SubscribeToEvent(E_KEYUP, URHO3D_HANDLER(UBrowserImage, HandleKeyUp));
    SubscribeToEvent(E_TEXTINPUT, URHO3D_HANDLER(UBrowserImage, HandleTextInput));
}

//=============================================================================
//=============================================================================
void UBrowserImage::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    UpdateBuffer();
}

void UBrowserImage::HandleFocusChanged(StringHash eventType, VariantMap& eventData)
{
    if ( cefBrowser_ )
    {
        cefBrowser_->GetHost()->SendFocusEvent(IsHovering());
    }
}

void UBrowserImage::OnHover(const IntVector2& position, const IntVector2& screenPosition, 
                            int buttons, int qualifiers, Cursor* cursor)
{
    if ( cefBrowser_ )
    {
        lastMousePos_ = position;
        CefMouseEvent cevent = GetCefMoustEvent(qualifiers);

        cefBrowser_->GetHost()->SendMouseMoveEvent(cevent, false);
    }
}

void UBrowserImage::OnClickBegin(const IntVector2& position, const IntVector2& screenPosition, 
                                 int button, int buttons, int qualifiers, Cursor* cursor)
{
    if ( cefBrowser_ )
    {
        lastMousePos_ = position;
        CefMouseEvent cevent = GetCefMoustEvent(qualifiers);

        bool mouseUp = false;
        CefBrowserHost::MouseButtonType btnType = MBT_LEFT;
        cefBrowser_->GetHost()->SendMouseClickEvent(cevent, btnType, mouseUp, 1);
    }
}

void UBrowserImage::OnClickEnd(const IntVector2& position, const IntVector2& screenPosition, 
                               int button, int buttons, int qualifiers, Cursor* cursor, UIElement* beginElement)
{
    if ( cefBrowser_ )
    {
        lastMousePos_ = position;
        CefMouseEvent cevent = GetCefMoustEvent(qualifiers);

        bool mouseUp = true;
        CefBrowserHost::MouseButtonType btnType = MBT_LEFT;
        cefBrowser_->GetHost()->SendMouseClickEvent(cevent, btnType, mouseUp, 1);
    }
}

//=============================================================================
//=============================================================================
void UBrowserImage::HandleMouseWheel(StringHash eventType, VariantMap& eventData)
{
    using namespace MouseWheel;

    int qualifiers = eventData[P_QUALIFIERS].GetInt();
    int delta = eventData[P_WHEEL].GetInt();

    if ( cefBrowser_ )
    {
        CefMouseEvent cevent = GetCefMoustEvent(qualifiers);

        delta = (int)((float)delta * scaleDiff_.y_ * MOUSE_WHEEL_MULTIPLYER);

        cefBrowser_->GetHost()->SendMouseWheelEvent(cevent, 0, delta);
    }
}

//=============================================================================
//=============================================================================
void UBrowserImage::HandleScreenMode(StringHash eventType, VariantMap& eventData)
{
    using namespace ScreenMode;

    //cefBrowser_->GetHost()->WasResized();
}

//=============================================================================
//=============================================================================
void UBrowserImage::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
    using namespace KeyDown;

    int qualifiers = eventData[P_QUALIFIERS].GetInt();
    int key = eventData[P_KEY].GetInt();

    if ( cefBrowser_ )
    {
        CefKeyEvent cevent;
        cevent.type             = KEYEVENT_KEYDOWN;
        cevent.modifiers        = GetKeyModifiers(qualifiers);
        cevent.windows_key_code = key;

        cefBrowser_->GetHost()->SendKeyEvent(cevent);
    }
}

//=============================================================================
//=============================================================================
void UBrowserImage::HandleKeyUp(StringHash eventType, VariantMap& eventData)
{
    using namespace KeyUp;

    int qualifiers = eventData[P_QUALIFIERS].GetInt();
    int key = eventData[P_KEY].GetInt();

    if ( cefBrowser_ )
    {
        CefKeyEvent cevent;
        cevent.type             = KEYEVENT_KEYUP;
        cevent.modifiers        = GetKeyModifiers(qualifiers);
        cevent.windows_key_code = key;

        cefBrowser_->GetHost()->SendKeyEvent(cevent);
    }
}

//=============================================================================
//=============================================================================
void UBrowserImage::HandleTextInput(StringHash eventType, VariantMap& eventData)
{
    using namespace TextInput;

    int qualifiers = eventData[P_QUALIFIERS].GetInt();
    int key = (int)eventData[ P_TEXT ].GetString().CString()[ 0 ];

    if ( cefBrowser_ )
    {
        CefKeyEvent cevent;
        cevent.type             = KEYEVENT_CHAR;
        cevent.modifiers        = GetKeyModifiers(qualifiers);
        cevent.windows_key_code = key;

        cefBrowser_->GetHost()->SendKeyEvent(cevent);
    }
}

CefMouseEvent UBrowserImage::GetCefMoustEvent(int qualifiers)
{
    CefMouseEvent cevent;

    cevent.x = (int)((float)lastMousePos_.x_ * scaleDiff_.x_);
    cevent.y = (int)((float)lastMousePos_.y_ * scaleDiff_.y_);
    cevent.modifiers = GetKeyModifiers(qualifiers);

    return cevent;
}

unsigned UBrowserImage::GetKeyModifiers(int qualifiers)
{
    unsigned modifier = 0;

    if ( qualifiers & QUAL_SHIFT ) modifier  = EVENTFLAG_SHIFT_DOWN;
    if ( qualifiers & QUAL_CTRL  ) modifier |= EVENTFLAG_CONTROL_DOWN;
    if ( qualifiers & QUAL_ALT   ) modifier |= EVENTFLAG_ALT_DOWN;

    Input* input = GetSubsystem<Input>();
    if ( input->GetKeyDown(KEY_CAPSLOCK) ) modifier |= EVENTFLAG_CAPS_LOCK_ON;

    return modifier;
}








