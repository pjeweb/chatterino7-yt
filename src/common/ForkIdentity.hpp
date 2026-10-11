// SPDX-FileCopyrightText: 2026 Contributors to chatterino7-yt
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QtGlobal>

/// Fork packaging / runtime identity for coexistence with Chatterino and
/// chatterino7. Keep CMake desktop/bundle/exe names in sync with these values
/// (see src/CMakeLists.txt and resources desktop/appdata files).
namespace chatterino::fork {

inline constexpr char APPLICATION_NAME[] = "chatterino7-yt";
inline constexpr char DISPLAY_NAME[] = "chatterino7-yt";
inline constexpr char EXECUTABLE_NAME[] = "chatterino7-yt";
/// Desktop file ID without the `.desktop` suffix (also used as Linux icon name
/// and native-messaging host name).
inline constexpr char DESKTOP_FILE_ID[] = "com.chatterino7yt.chatterino7yt";
inline constexpr char MACOS_BUNDLE_ID[] = "app.chatterino7yt.chatterino7yt";

#ifdef Q_OS_WIN
inline constexpr wchar_t APP_USER_MODEL_ID[] = L"Chatterino7YT.Chatterino7YT";
#endif

}  // namespace chatterino::fork
