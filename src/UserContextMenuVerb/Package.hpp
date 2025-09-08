#pragma once

#ifdef _DEBUG
#define PACKAGE_NAME L"Flipeador.UserContextMenu.Dev_jtjzc90v003vw"
#else
#define PACKAGE_NAME L"Flipeador.UserContextMenu_jtjzc90v003vw"
#endif

// Verb ID | COM Class ID (Package.appxmanifest)
#define PACKAGE_COM_CLSID "4529C759-9140-4FF5-A577-C05357BA9508"

Json LoadPackageCommands();
