//
// PROJECT:         Aspia
// FILE:            host/clipboard.cc
// LICENSE:         GNU Lesser General Public License 2.1
// PROGRAMMERS:     Dmitry Chapyshev (dmitry@aspia.ru)
//

#include "host/clipboard.h"

#include <QDebug>

#include "base/strings/string_util.h"
#include "base/strings/unicode.h"
#include "base/win/scoped_clipboard.h"
#include "base/win/scoped_hglobal.h"
#include "base/system_error_code.h"

namespace aspia {

namespace {

constexpr char kMimeTypeTextUtf8[] = "text/plain; charset=UTF-8";

} // namespace

Clipboard::~Clipboard()
{
    Stop();
}

bool Clipboard::Start(ClipboardEventCallback clipboard_event_callback)
{
    clipboard_event_callback_ = std::move(clipboard_event_callback);

    window_ = std::make_unique<MessageWindow>();

    if (!window_->Create(std::bind(&Clipboard::OnMessage,
                                   this,
                                   std::placeholders::_1, std::placeholders::_2,
                                   std::placeholders::_3, std::placeholders::_4)))
    {
        qWarning("Couldn't create clipboard window.");
        return false;
    }

    if (!AddClipboardFormatListener(window_->hwnd()))
    {
        qWarning() << "AddClipboardFormatListener failed: " << lastSystemErrorString();
        return false;
    }

    return true;
}

void Clipboard::Stop()
{
    if (window_)
    {
        RemoveClipboardFormatListener(window_->hwnd());

        window_.reset();

        last_mime_type_.clear();
        last_data_.clear();

        clipboard_event_callback_ = nullptr;
    }
}

void Clipboard::OnClipboardUpdate()
{
    if (IsClipboardFormatAvailable(CF_UNICODETEXT))
    {
        std::string data;

        // Add a scope, so that we keep the clipboard open for as short a time
        // as possible.
        {
            ScopedClipboard clipboard;

            if (!clipboard.Init(window_->hwnd()))
            {
                qWarning() << "Couldn't open the clipboard: " << lastSystemErrorString();
                return;
            }

            HGLOBAL text_global = clipboard.GetData(CF_UNICODETEXT);
            if (!text_global)
            {
                qWarning() << "Couldn't get data from the clipboard: " << lastSystemErrorString();
                return;
            }

            {
                ScopedHGlobal<wchar_t> text_lock(text_global);
                if (!text_lock.Get())
                {
                    qWarning() << "Couldn't lock clipboard data: " << lastSystemErrorString();
                    return;
                }

                if (!UNICODEtoUTF8(text_lock.Get(), data))
                {
                    qWarning("Couldn't convert data to utf8");
                    return;
                }
            }
        }

        if (!data.empty())
        {
            data = ReplaceCrLfByLf(data);

            if (last_mime_type_ != kMimeTypeTextUtf8 || last_data_ != data)
            {
                proto::desktop::ClipboardEvent event;

                event.set_mime_type(kMimeTypeTextUtf8);
                event.set_data(data);

                clipboard_event_callback_(event);
            }
        }
    }
}

bool Clipboard::OnMessage(UINT message, WPARAM /* wParam */, LPARAM /* lParam */, LRESULT& result)
{
    switch (message)
    {
        case WM_CLIPBOARDUPDATE:
            OnClipboardUpdate();
            break;

        default:
            return false;
    }

    result = 0;
    return true;
}

void Clipboard::InjectClipboardEvent(const proto::desktop::ClipboardEvent& clipboard_event)
{
    if (!window_)
        return;

    // Currently we only handle UTF-8 text.
    if (clipboard_event.mime_type() != kMimeTypeTextUtf8)
    {
        qWarning() << "Unsupported mime type: "
                   << QString::fromStdString(clipboard_event.mime_type());
        return;
    }

    if (!IsStringUTF8(clipboard_event.data()))
    {
        qWarning("Clipboard data is not UTF-8 encoded");
        return;
    }

    // Store last injected data.
    last_mime_type_ = clipboard_event.mime_type();
    last_data_ = clipboard_event.data();

    std::wstring text;

    if (!UTF8toUNICODE(ReplaceLfByCrLf(last_data_), text))
    {
        qWarning("Couldn't convert data to unicode");
        return;
    }

    ScopedClipboard clipboard;

    if (!clipboard.Init(window_->hwnd()))
    {
        qWarning("Couldn't open the clipboard");
        return;
    }

    clipboard.Empty();

    HGLOBAL text_global = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(wchar_t));
    if (!text_global)
    {
        qWarning() << "GlobalAlloc failed: " << lastSystemErrorString();
        return;
    }

    LPWSTR text_global_locked = reinterpret_cast<LPWSTR>(GlobalLock(text_global));
    if (!text_global_locked)
    {
        qWarning() << "GlobalLock failed: " << lastSystemErrorString();
        GlobalFree(text_global);
        return;
    }

    memcpy(text_global_locked, text.data(), text.size() * sizeof(wchar_t));
    text_global_locked[text.size()] = 0;

    GlobalUnlock(text_global);

    clipboard.SetData(CF_UNICODETEXT, text_global);
}

} // namespace aspia