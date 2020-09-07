// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#if defined( SC_USE_WEBENGINE )
#include <QtWebEngine/QtWebEngine>
#include <QtWebEngineWidgets/QtWebEngineWidgets>
#else
#include <QtWebKit/QtWebKit>
#include <QtWebKitWidgets/QtWebKitWidgets>
#endif

#if defined( SC_USE_WEBENGINE )
using SC_WebEngineView = QWebEngineView;
using SC_WebEnginePage = QWebEnginePage ;
#else
using SC_WebEngineView = QWebView;

using SC_WebEnginePage = QWebPage;
#endif