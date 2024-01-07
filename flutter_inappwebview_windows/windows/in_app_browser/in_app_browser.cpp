#include <Windows.h>
#include "in_app_browser.h"

#include <cstdint>
#include <memory>

#include "flutter/event_channel.h"
#include "flutter/plugin_registrar.h"
#include "flutter/plugin_registrar_windows.h"
#include "flutter/method_channel.h"
#include "flutter/encodable_value.h"

#include <stdlib.h>
#include <string>
#include <tchar.h>
#include <wil/wrl.h>
#include <wil/com.h>
#include "../utils/strconv.h"
#include "../utils/util.h"

#include <WebView2.h>
#include <WebView2EnvironmentOptions.h>

namespace flutter_inappwebview_plugin
{
	using namespace Microsoft::WRL;

	const wchar_t* CLASS_NAME = L"InAppBrowser";

	InAppBrowser::InAppBrowser(FlutterInappwebviewWindowsBasePlugin* plugin, const InAppBrowserCreationParams& params)
		: plugin(plugin),		
		m_hInstance(GetModuleHandle(nullptr)),
		id(params.id),
		initialUrlRequest(params.urlRequest),
		channelDelegate(std::make_unique<InAppBrowserChannelDelegate>(id, plugin->registrar->messenger()))
	{
		WNDCLASS wndClass = {};
		wndClass.lpszClassName = CLASS_NAME;
		wndClass.hInstance = m_hInstance;
		wndClass.hIcon = LoadIcon(NULL, IDI_WINLOGO);
		wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
		wndClass.lpfnWndProc = InAppBrowser::WndProc;

		RegisterClass(&wndClass);

		m_hWnd = CreateWindowEx(
			0,                              // Optional window styles.
			CLASS_NAME,                     // Window class
			L"Learn to Program Windows",    // Window text
			WS_OVERLAPPEDWINDOW,            // Window style

			// Size and position
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

			NULL,       // Parent window    
			NULL,       // Menu
			m_hInstance,// Instance handle
			this        // Additional application data
		);

		ShowWindow(m_hWnd, SW_SHOW);

		webView = std::make_unique<InAppWebView>(plugin, id, m_hWnd, InAppBrowser::METHOD_CHANNEL_NAME_PREFIX + id, [this]() -> void {
			if (channelDelegate) {
				channelDelegate->onBrowserCreated();
			}

			if (initialUrlRequest.has_value()) {
				webView->loadUrl(initialUrlRequest.value());
			}
		});

		// <-- WebView2 sample code starts here -->
		// Step 3 - Create a single WebView within the parent window
		// Locate the browser and set up the environment for WebView
		//CreateCoreWebView2EnvironmentWithOptions(nullptr, nullptr, nullptr,
		//	Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
		//		[hWnd = m_hWnd, inAppBrowser = this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

		//			// Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
		//			env->CreateCoreWebView2Controller(hWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
		//				[hWnd, inAppBrowser](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
		//					if (controller != nullptr) {
		//						inAppBrowser->webviewController = controller;
		//						inAppBrowser->webviewController->get_CoreWebView2(&inAppBrowser->webview);
		//					}

		//					// Add a few settings for the webview
		//					// The demo step is redundant since the values are the default settings
		//					wil::com_ptr<ICoreWebView2Settings> settings;
		//					inAppBrowser->webview->get_Settings(&settings);
		//					settings->put_IsScriptEnabled(TRUE);
		//					settings->put_AreDefaultScriptDialogsEnabled(TRUE);
		//					settings->put_IsWebMessageEnabled(TRUE);

		//					// Resize WebView to fit the bounds of the parent window
		//					RECT bounds;
		//					GetClientRect(hWnd, &bounds);
		//					inAppBrowser->webviewController->put_Bounds(bounds);

		//					auto url = inAppBrowser->initialUrlRequest.value().url.value();
		//					std::wstring stemp = ansi_to_wide(url);

		//					// Schedule an async task to navigate to Bing
		//					inAppBrowser->webview->Navigate(stemp.c_str());

		//					// <NavigationEvents>
		//					// Step 4 - Navigation events
		//					// register an ICoreWebView2NavigationStartingEventHandler to cancel any non-https navigation
		//					EventRegistrationToken token;
		//					inAppBrowser->webview->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
		//						[](ICoreWebView2* webview, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
		//							wil::unique_cotaskmem_string uri;
		//							args->get_Uri(&uri);
		//							std::wstring source(uri.get());
		//							if (source.substr(0, 5) != L"https") {
		//								args->put_Cancel(true);
		//							}
		//							return S_OK;
		//						}).Get(), &token);
		//					// </NavigationEvents>

		//					// <Scripting>
		//					// Step 5 - Scripting
		//					// Schedule an async task to add initialization script that freezes the Object object
		//					inAppBrowser->webview->AddScriptToExecuteOnDocumentCreated(L"Object.freeze(Object);", nullptr);
		//					// Schedule an async task to get the document URL
		//					inAppBrowser->webview->ExecuteScript(L"window.document.URL;", Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
		//						[](HRESULT errorCode, LPCWSTR resultObjectAsJson) -> HRESULT {
		//							LPCWSTR URL = resultObjectAsJson;
		//							OutputDebugStringW(URL);
		//							//doSomethingWithURL(URL);
		//							return S_OK;
		//						}).Get());
		//					// </Scripting>

		//					// <CommunicationHostWeb>
		//					// Step 6 - Communication between host and web content
		//					// Set an event handler for the host to return received message back to the web content
		//					inAppBrowser->webview->add_WebMessageReceived(Callback<ICoreWebView2WebMessageReceivedEventHandler>(
		//						[](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* args) -> HRESULT {
		//							wil::unique_cotaskmem_string message;
		//							args->TryGetWebMessageAsString(&message);
		//							// processMessage(&message);
		//							webview->PostWebMessageAsString(message.get());
		//							return S_OK;
		//						}).Get(), &token);

		//					// Schedule an async task to add initialization script that
		//					// 1) Add an listener to print message from the host
		//					// 2) Post document URL to the host
		//					inAppBrowser->webview->AddScriptToExecuteOnDocumentCreated(
		//						L"window.chrome.webview.addEventListener(\'message\', event => alert(event.data));" \
		//						L"window.chrome.webview.postMessage(window.document.URL);",
		//						nullptr);
		//					// </CommunicationHostWeb>

		//					return S_OK;
		//				}).Get());
		//			return S_OK;
		//		}).Get());
	}

	LRESULT CALLBACK InAppBrowser::WndProc(
			HWND window,
			UINT message,
			WPARAM wparam,
			LPARAM lparam
		) noexcept {
		if (message == WM_NCCREATE) {
			auto window_struct = reinterpret_cast<CREATESTRUCT*>(lparam);
			SetWindowLongPtr(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window_struct->lpCreateParams));
		}
		else if (InAppBrowser* that = GetThisFromHandle(window)) {
			return that->MessageHandler(window, message, wparam, lparam);
		}

		return DefWindowProc(window, message, wparam, lparam);
	}

	LRESULT InAppBrowser::MessageHandler(
			HWND hwnd,
			UINT message,
			WPARAM wparam,
			LPARAM lparam
		) noexcept {
		switch (message) {
		case WM_DESTROY: {
			webView.reset();

			// might receive multiple WM_DESTROY messages.
			if (!destroyed_) {
				destroyed_ = true;

				if (channelDelegate) {
					channelDelegate->onExit();
				}
			}
			return 0;
		}
		case WM_DPICHANGED: {
			auto newRectSize = reinterpret_cast<RECT*>(lparam);
			LONG newWidth = newRectSize->right - newRectSize->left;
			LONG newHeight = newRectSize->bottom - newRectSize->top;

			SetWindowPos(hwnd, nullptr, newRectSize->left, newRectSize->top, newWidth,
				newHeight, SWP_NOZORDER | SWP_NOACTIVATE);
			return 0;
		}
		case WM_SIZE: {
			RECT bounds;
			GetClientRect(hwnd, &bounds);
			if (webView) {
				webView->webViewController->put_Bounds(bounds);
			}
			return 0;
		}
		case WM_ACTIVATE: {
			return 0;
		}
		}

		return DefWindowProc(hwnd, message, wparam, lparam);
	}

	InAppBrowser* InAppBrowser::GetThisFromHandle(HWND const window) noexcept {
		return reinterpret_cast<InAppBrowser*>(
			GetWindowLongPtr(window, GWLP_USERDATA));
	}

	InAppBrowser::~InAppBrowser()
	{
		debugLog("dealloc InAppBrowser");
		webView.reset();
		SetWindowLongPtr(m_hWnd, GWLP_USERDATA, 0);
		plugin = nullptr;
	}

}