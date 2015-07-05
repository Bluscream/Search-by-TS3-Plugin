/*
 * TeamSpeak 3 demo plugin
 *
 * Copyright (c) 2008-2014 TeamSpeak Systems GmbH
 */

#ifdef _WIN32
#pragma warning (disable : 4100)  /* Disable Unreferenced parameter warning */
#include <Windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "public_errors.h"
#include "public_errors_rare.h"
#include "public_definitions.h"
#include "public_rare_definitions.h"
#include "ts3_functions.h"
#include "plugin.h"

static struct TS3Functions ts3Functions;

#ifdef _WIN32
#define _strcpy(dest, destSize, src) strcpy_s(dest, destSize, src)
#define snprintf sprintf_s
#else
#define _strcpy(dest, destSize, src) { strncpy(dest, src, destSize-1); (dest)[destSize-1] = '\0'; }
#endif

#define PLUGIN_API_VERSION 20

#define PATH_BUFSIZE 512
#define COMMAND_BUFSIZE 128
#define INFODATA_BUFSIZE 128
#define SERVERINFO_BUFSIZE 256
#define CHANNELINFO_BUFSIZE 512
#define RETURNCODE_BUFSIZE 128

#define PLUGIN_NAME "Search By"
#define PLUGIN_AUTHOR "Bluscream"
#define PLUGIN_VERSION "1.0"
#define PLUGIN_CONTACT "admin@timo.de.vc"

static char* pluginID = NULL;

#ifdef _WIN32
/* Helper function to convert wchar_T to Utf-8 encoded strings on Windows */
static int wcharToUtf8(const wchar_t* str, char** result) {
	int outlen = WideCharToMultiByte(CP_UTF8, 0, str, -1, 0, 0, 0, 0);
	*result = (char*)malloc(outlen);
	if(WideCharToMultiByte(CP_UTF8, 0, str, -1, *result, outlen, 0, 0) == 0) {
		*result = NULL;
		return -1;
	}
	return 0;
}
#endif

/*********************************** Required functions ************************************/
/*
 * If any of these required functions is not implemented, TS3 will refuse to load the plugin
 */

/* Unique name identifying this plugin */
const char* ts3plugin_name() {
#ifdef _WIN32
	/* TeamSpeak expects UTF-8 encoded characters. Following demonstrates a possibility how to convert UTF-16 wchar_t into UTF-8. */
	static char* result = NULL;  /* Static variable so it's allocated only once */
	if(!result) {
		const wchar_t* name = L"Search by";
		if(wcharToUtf8(name, &result) == -1) {  /* Convert name into UTF-8 encoded result */
			result = "Search by";  /* Conversion failed, fallback here */
		}
	}
	return result;
#else
	return "Search by!";
#endif
}

/* Plugin version */
const char* ts3plugin_version() {
    return "1";
}

/* Plugin API version. Must be the same as the clients API major version, else the plugin fails to load. */
int ts3plugin_apiVersion() {
	return PLUGIN_API_VERSION;
}

/* Plugin author */
const char* ts3plugin_author() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
    return "Bluscream";
}

/* Plugin description */
const char* ts3plugin_description() {
	/* If you want to use wchar_t, see ts3plugin_name() on how to use */
	return "Gives you the ability to easy search for users and servers. Uses TSViewer, GameTracker, TS3Index and Google";
}

/* Set TeamSpeak 3 callback functions */
void ts3plugin_setFunctionPointers(const struct TS3Functions funcs) {
    ts3Functions = funcs;
}

/*
 * Custom code called right after loading the plugin. Returns 0 on success, 1 on failure.
 * If the function returns 1 on failure, the plugin will be unloaded again.
 */
int ts3plugin_init() {
    char appPath[PATH_BUFSIZE];
    char resourcesPath[PATH_BUFSIZE];
    char configPath[PATH_BUFSIZE];
	char pluginPath[PATH_BUFSIZE];

    /* Your plugin init code here */
    printf("PLUGIN: init\n");

    /* Example on how to query application, resources and configuration paths from client */
    /* Note: Console client returns empty string for app and resources path */
    ts3Functions.getAppPath(appPath, PATH_BUFSIZE);
    ts3Functions.getResourcesPath(resourcesPath, PATH_BUFSIZE);
    ts3Functions.getConfigPath(configPath, PATH_BUFSIZE);
	ts3Functions.getPluginPath(pluginPath, PATH_BUFSIZE);

	printf("PLUGIN: App path: %s\nResources path: %s\nConfig path: %s\nPlugin path: %s\n", appPath, resourcesPath, configPath, pluginPath);

    return 0;  /* 0 = success, 1 = failure, -2 = failure but client will not show a "failed to load" warning */
	/* -2 is a very special case and should only be used if a plugin displays a dialog (e.g. overlay) asking the user to disable
	 * the plugin again, avoiding the show another dialog by the client telling the user the plugin failed to load.
	 * For normal case, if a plugin really failed to load because of an error, the correct return value is 1. */
}

/* Custom code called right before the plugin is unloaded */
void ts3plugin_shutdown() {
    /* Your plugin cleanup code here */
    printf("PLUGIN: shutdown\n");

	/*
	 * Note:
	 * If your plugin implements a settings dialog, it must be closed and deleted here, else the
	 * TeamSpeak client will most likely crash (DLL removed but dialog from DLL code still open).
	 */

	/* Free pluginID if we registered it */
	if(pluginID) {
		free(pluginID);
		pluginID = NULL;
	}
}

void ts3plugin_registerPluginID(const char* id) {
	const size_t sz = strlen(id) + 1;
	pluginID = (char*)malloc(sz * sizeof(char));
	_strcpy(pluginID, sz, id);  /* The id buffer will invalidate after exiting this function */
	printf("PLUGIN: registerPluginID: %s\n", pluginID);
}

void ts3plugin_freeMemory(void* data) {
	free(data);
}
//
 /*
 * Plugin requests to be always automatically loaded by the TeamSpeak 3 client unless
 * the user manually disabled it in the plugin dialog.
 * This function is optional. If missing, no autoload is assumed.
 */
int ts3plugin_requestAutoload() {
	return 1;  /* 1 = request autoloaded, 0 = do not request autoload */
}
//
///* Helper function to create a menu item */
static struct PluginMenuItem* createMenuItem(enum PluginMenuType type, int id, const char* text, const char* icon) {
	struct PluginMenuItem* menuItem = (struct PluginMenuItem*)malloc(sizeof(struct PluginMenuItem));
	menuItem->type = type;
	menuItem->id = id;
	_strcpy(menuItem->text, PLUGIN_MENU_BUFSZ, text);
	_strcpy(menuItem->icon, PLUGIN_MENU_BUFSZ, icon);
	return menuItem;
}
//
///* Some makros to make the code to create menu items a bit more readable */
#define BEGIN_CREATE_MENUS(x) const size_t sz = x + 1; size_t n = 0; *menuItems = (struct PluginMenuItem**)malloc(sizeof(struct PluginMenuItem*) * sz);
#define CREATE_MENU_ITEM(a, b, c, d) (*menuItems)[n++] = createMenuItem(a, b, c, d);
#define END_CREATE_MENUS (*menuItems)[n++] = NULL; assert(n == sz);
//
///*
// * Menu IDs for this plugin. Pass these IDs when creating a menuitem to the TS3 client. When the menu item is triggered,
// * ts3plugin_onMenuItemEvent will be called passing the menu ID of the triggered menu item.
// * These IDs are freely choosable by the plugin author. It's not really needed to use an enum, it just looks prettier.
// */
enum {
	MENU_ID_CLIENT_1 = 1,
		MENU_ID_CLIENT_2,
		MENU_ID_CLIENT_3,
		MENU_ID_CLIENT_4,
		MENU_ID_CLIENT_5,
		MENU_ID_CLIENT_6,
		MENU_ID_CLIENT_7,
		MENU_ID_CLIENT_8,
		MENU_ID_CLIENT_9,
		MENU_ID_GLOBAL_1,
		MENU_ID_GLOBAL_2,
		MENU_ID_GLOBAL_3,
		MENU_ID_GLOBAL_4,
		MENU_ID_GLOBAL_5,
		MENU_ID_GLOBAL_6,
		MENU_ID_GLOBAL_7
};
//
///*
// * Initialize plugin menus.
// * This function is called after ts3plugin_init and ts3plugin_registerPluginID. A pluginID is required for plugin menus to work.
// * Both ts3plugin_registerPluginID and ts3plugin_freeMemory must be implemented to use menus.
// * If plugin menus are not used by a plugin, do not implement this function or return NULL.
// */
void ts3plugin_initMenus(struct PluginMenuItem*** menuItems, char** menuIcon) {
	/*
	 * Create the menus
	 * There are three types of menu items:
	 * - PLUGIN_MENU_TYPE_CLIENT:  Client context menu
	 * - PLUGIN_MENU_TYPE_CHANNEL: Channel context menu
	 * - PLUGIN_MENU_TYPE_GLOBAL:  "Plugins" menu in menu bar of main window
	 *
	 * Menu IDs are used to identify the menu item when ts3plugin_onMenuItemEvent is called
	 *
	 * The menu text is required, max length is 128 characters
	 *
	 * The icon is optional, max length is 128 characters. When not using icons, just pass an empty string.
	 * Icons are loaded from a subdirectory in the TeamSpeak client plugins folder. The subdirectory must be named like the
	 * plugin filename, without dll/so/dylib suffix
	 * e.g. for "test_plugin.dll", icon "1.png" is loaded from <TeamSpeak 3 Client install dir>\plugins\test_plugin\1.png
	 */

	BEGIN_CREATE_MENUS(16);  /* IMPORTANT: Number of menu items must be correct! */
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_1, "Nickname (TSViewer)", "name.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_2, "Nickname (GameTracker)", "name.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_3, "Nickname (TS3Index)", "name.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_4, "Nickname (Google)", "name.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_5, "Profile (GameTracker)", "name.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_6, "UID (TS3Index)", "id.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_7, "UID (Google)", "id.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_8, "Owner (TSViewer)", "admin.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_CLIENT, MENU_ID_CLIENT_9, "DBID (mtG)", "id.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_1, "Name (TSViewer)", "name.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_2, "Name (GameTracker)", "name.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_3, "Name (Google)", "name.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_4, "IP (TSViewer)", "ip.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_5, "IP (GameTracker)", "ip.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_6, "IP (Google)", "ip.png");
	CREATE_MENU_ITEM(PLUGIN_MENU_TYPE_GLOBAL, MENU_ID_GLOBAL_7, "About", "about.png");
	END_CREATE_MENUS;  /* Includes an assert checking if the number of menu items matched */

	/*
	 * Specify an optional icon for the plugin. This icon is used for the plugins submenu within context and main menus
	 * If unused, set menuIcon to NULL
	 */
	*menuIcon = (char*)malloc(PLUGIN_MENU_BUFSZ * sizeof(char));
	_strcpy(*menuIcon, PLUGIN_MENU_BUFSZ, "search.png");


}

/* Converts a hex character to its integer value */
char from_hex(char ch) {
	return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

/* Converts an integer value to its hex character*/
char to_hex(char code) {
	static char hex[] = "0123456789abcdef";
	return hex[code & 15];
}

/* Returns a url-encoded version of str */
/* IMPORTANT: be sure to free() the returned string after use */
char *url_encode(char *str) {
	char *pstr = str, *buf = malloc(strlen(str) * 3 + 1), *pbuf = buf;
	while (*pstr) {
		if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~')
			*pbuf++ = *pstr;
		else if (*pstr == ' ')
			*pbuf++ = '+';
		else
			*pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
		pstr++;
	}
	*pbuf = '\0';
	return buf;
}

void ts3plugin_onMenuItemEvent(uint64 serverConnectionHandlerID, enum PluginMenuType type, int menuItemID, uint64 selectedItemID) {
	anyID myID;
	char* Data;
	char string_s[150];
	char url[100];
	int DataINT;
	switch (type) {
		case PLUGIN_MENU_TYPE_GLOBAL:
			if (ts3Functions.getClientID(serverConnectionHandlerID, &myID) != ERROR_ok) {
				MessageBoxA(0, "Cant get your clientID. Are you connected to a server?", PLUGIN_NAME " - Error", MB_ICONERROR);
				break;
			}
			switch (menuItemID) {
				case MENU_ID_GLOBAL_1:
					if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_NAME, &Data) != ERROR_ok) {
						return;
					}
					Data = url_encode(Data);
					strcpy(url, "start http://www.tsviewer.com/index.php?page=search^&action=ausgabe%26suchbereich=name^&suchinhalt=");
					strcat(url, Data);
										strcpy(string_s, "Searching for \"[color=black][u]");
										strcat(string_s, Data);
										strcat(string_s, "[/color]\"");
										ts3Functions.printMessageToCurrentTab(string_s);
					system(url);
										free(Data);
					break;
				case MENU_ID_GLOBAL_2:
					if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_NAME, &Data) != ERROR_ok) {
						return;
					}
					Data = url_encode(Data);
					strcpy(url, "start http://www.gametracker.com/search/?query=");
					strcat(url, Data);
					strcpy(string_s, "Searching for \"[color=black][u]");
					strcat(string_s, Data);
					strcat(string_s, "[/color]\"");
					ts3Functions.printMessageToCurrentTab(string_s);
					system(url);
										free(Data);
					break;
				case MENU_ID_GLOBAL_3:
					if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_NAME, &Data) != ERROR_ok) {
						return;
					}
					Data = url_encode(Data);
					strcpy(url, "start https://www.google.com/search?q=");
					strcat(url, Data);
					strcpy(string_s, "Searching for \"[color=black][u]");
					strcat(string_s, Data);
					strcat(string_s, "[/color]\"");
					ts3Functions.printMessageToCurrentTab(string_s);
					system(url);
					free(Data);
					break;
				case MENU_ID_GLOBAL_4:
					if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_IP, &Data) == ERROR_ok) {
						if (!Data || Data[0] == '\0') {
							if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, 76, &Data) == ERROR_ok) {
								if (!Data || Data[0] == '\0') {
									ts3Functions.getConnectionVariableAsString(serverConnectionHandlerID, myID, 6, &Data);
								}
							}
						}
					}
					strcpy(url, "start http://www.tsviewer.com/index.php?page=search^&action=ausgabe^&suchbereich=ip^&suchinhalt=");
					strcat(url, Data);
					strcpy(string_s, "Searching for \"[color=black][u]");
					strcat(string_s, Data);
					strcat(string_s, "[/color]\"");
					ts3Functions.printMessageToCurrentTab(string_s);
					system(url);
					break;
				case MENU_ID_GLOBAL_5:
					if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_IP, &Data) == ERROR_ok) {
						if (!Data || Data[0] == '\0') {
							if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, 76, &Data) == ERROR_ok) {
								if (!Data || Data[0] == '\0') {
									ts3Functions.getConnectionVariableAsString(serverConnectionHandlerID, myID, 6, &Data);
								}
							}
						}
					}
					strcpy(url, "start http://www.gametracker.com/search/?query=");
					strcat(url, Data);
										strcpy(string_s, "Searching for \"[color=black][u]");
										strcat(string_s, Data);
										strcat(string_s, "[/color]\"");
										ts3Functions.printMessageToCurrentTab(string_s);
					system(url);
					break;
				case MENU_ID_GLOBAL_6:
					if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_IP, &Data) == ERROR_ok) {
						if (!Data || Data[0] == '\0') {
							if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, 76, &Data) == ERROR_ok) {
								if (!Data || Data[0] == '\0') {
									ts3Functions.getConnectionVariableAsString(serverConnectionHandlerID, myID, 6, &Data);
								}
							}
						}
					}
					strcpy(url, "start https://www.google.com/search?q=");
					strcat(url, Data);
										strcpy(string_s, "Searching for \"[color=black][u]");
										strcat(string_s, Data);
										strcat(string_s, "[/color]\"");
										ts3Functions.printMessageToCurrentTab(string_s);
					system(url);
					break;
				case MENU_ID_GLOBAL_7:
					MessageBoxA(0, PLUGIN_NAME " v" PLUGIN_VERSION " developed by " PLUGIN_AUTHOR " (" PLUGIN_CONTACT ")", "About " PLUGIN_NAME, MB_ICONINFORMATION);
					break;
				default:
					break;
			}
			break;
		case PLUGIN_MENU_TYPE_CLIENT:
			switch (menuItemID) {
				case MENU_ID_CLIENT_1:
					if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, selectedItemID, CLIENT_NICKNAME, &Data) != ERROR_ok) {
						return;
					}
					Data = url_encode(Data);
					strcpy(url, "start http://www.tsviewer.com/index.php?page=search^&action=ausgabe_user^&nickname=");
					strcat(url, Data);
										strcpy(string_s, "Searching for \"[color=black][u]");
										strcat(string_s, Data);
										strcat(string_s, "[/color]\"");
										ts3Functions.printMessageToCurrentTab(string_s);
					system(url);
										free(Data);
					break;
				case MENU_ID_CLIENT_2:
					if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, selectedItemID, CLIENT_NICKNAME, &Data) != ERROR_ok) {
						return;
					}
					Data = url_encode(Data);
					strcpy(url, "start http://www.gametracker.com/search/?search_by=online_offline_player^&query=");
					strcat(url, Data);
										strcpy(string_s, "Searching for \"[color=black][u]");
										strcat(string_s, Data);
										strcat(string_s, "[/color]\"");
										ts3Functions.printMessageToCurrentTab(string_s);
					system(url);
										free(Data);
					break;
				case MENU_ID_CLIENT_3:
					if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, selectedItemID, CLIENT_NICKNAME, &Data) != ERROR_ok) {
						return;
					}
					Data = url_encode(Data);
					strcpy(url, "start http://ts3index.com/?page=searchclient^&nickname=");
					strcat(url, Data);
										strcpy(string_s, "Searching for \"[color=black][u]");
										strcat(string_s, Data);
										strcat(string_s, "[/color]\"");
										ts3Functions.printMessageToCurrentTab(string_s);
					system(url);
										free(Data);
					break;
				case MENU_ID_CLIENT_4:
					if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, selectedItemID, CLIENT_NICKNAME, &Data) != ERROR_ok) {
						return;
					}
					Data = url_encode(Data);
					strcpy(url, "start https://www.google.com/search?q=");
					strcat(url, Data);
										strcpy(string_s, "Searching for \"[color=black][u]");
										strcat(string_s, Data);
										strcat(string_s, "[/color]\"");
										ts3Functions.printMessageToCurrentTab(string_s);
					system(url);
										free(Data);
					break;
				case MENU_ID_CLIENT_5:
					if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, selectedItemID, CLIENT_NICKNAME, &Data) != ERROR_ok) {
						return;
					}
					Data = url_encode(Data);
					strcpy(url, "start http://www.gametracker.com/search/?search_by=profile_username^&query=");
					strcat(url, Data);
										strcpy(string_s, "Searching for \"[color=black][u]");
										strcat(string_s, Data);
										strcat(string_s, "[/color]\"");
										ts3Functions.printMessageToCurrentTab(string_s);
					system(url);
										free(Data);
					break;
				case MENU_ID_CLIENT_6:
					if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, selectedItemID, CLIENT_UNIQUE_IDENTIFIER, &Data) != ERROR_ok) {
						return;
					}
					Data = url_encode(Data);
					strcpy(url, "start http://ts3index.com/?page=searchclient^&uid=");
					strcat(url, Data);
					strcpy(string_s, "Searching for \"[color=black][u]");
					strcat(string_s, Data);
					strcat(string_s, "[/color]\"");
					ts3Functions.printMessageToCurrentTab(string_s);
					system(url);
					//if (MessageBox(NULL, ("Text" + clientUID + "more Text").c_str(), "Title", MB_YESNO) == 6)
					//MessageBoxA(0, "Test", PLUGIN_NAME, MB_ICONINFORMATION);
					//ShellExecuteA(NULL, "open", url.c_str(), NULL, NULL, SW_SHOWNORMAL);
					free(Data);
					break;
				case MENU_ID_CLIENT_7:
					if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, selectedItemID, CLIENT_UNIQUE_IDENTIFIER, &Data) != ERROR_ok) {
						return;
					}
					Data = url_encode(Data);
					strcpy(url, "start https://www.google.com/search?q=");
					strcat(url, Data);
										strcpy(string_s, "Searching for \"[color=black][u]");
										strcat(string_s, Data);
										strcat(string_s, "[/color]\"");
										ts3Functions.printMessageToCurrentTab(string_s);
					system(url);
										free(Data);
					break;
				case MENU_ID_CLIENT_8:
					if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, selectedItemID, CLIENT_NICKNAME, &Data) != ERROR_ok) {
						return;
					}
					Data = url_encode(Data);
					strcpy(url, "start http://www.tsviewer.com/index.php?page=search^&action=ausgabe^&suchbereich=ansprechpartner^&suchinhalt=");
					strcat(url, Data);
										strcpy(string_s, "Searching for \"[color=black][u]");
										strcat(string_s, Data);
										strcat(string_s, "[/color]\"");
										ts3Functions.printMessageToCurrentTab(string_s);
					system(url);
										free(Data);
										break;
				case MENU_ID_CLIENT_9:
					if (ts3Functions.getClientVariableAsInt(serverConnectionHandlerID, selectedItemID, CLIENT_DATABASE_ID, &DataINT) != ERROR_ok) {
						return;
					}
					char d[15];
					strcpy(url, "start https://www.mtg-esport.de/viewpage.php?page_id=7^&pki=");
					itoa(DataINT, d,10);
					
					strcat(url, d);
					system(url);
					//ts3Functions.logMessage(url, LogLevel_DEBUG, "bla", 0);
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
}