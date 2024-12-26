
// dont move
#include "kmbox/kmboxNet.h"
#include "kmbox/HidTable.h"
//

#include <Windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <string>
#include "console/console.h"

#include "performance.h"
#include "settings.h"

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_impl_vulkan.h>
#include <imgui/imgui_impl_win32.h>

#include "rendering/Font.h"
#define STB_IMAGE_IMPLEMENTATION
#include "rendering/stb_image.h"
#include "rendering/images.h"

#include "menu/menu.h"

#include "rendering/window.h"

#include "dma/DMAHandler.h"
#include "kmbox/kmbox_interface.h"

#include "kmbox/kmbox_util.h"

#include "cheat/cheat.h"
#include "cheat/aim.h"
#include "cheat/radar.h"
#include "cheat/esp.h"

vector<feature> memoryList;
vector<feature> mainList;

void memRefreshLight() {
	mem.RefreshLight();
}

void memKeysUpdate() {

	// features that need the keys updated
	if (!settings::config::Aimbot && !settings::config::TriggerBot)
		return;

	mem.UpdateKeys();
}

bool on_initialize() {

	if (connect_serial_kmbox()) {
		settings::kmbox::SerialKmbox = true;
	}

	// configs
	settings::loadConfig();

	// try to connect to kmbox net
	if (connect_net_kmbox()) {
		settings::kmbox::NetKmbox = true;
		kmNet_lcd_picture_bottom((unsigned char*)images::mini_supernatural_image);
	}
	
	if (mem.Init(L"FortniteClient-Win64-Shipping.exe", settings::dma::MemoryMap) < 0) {
		std::cout << hue::red << "(!) " << hue::white << "Failed to initialize" << std::endl;
		return false;
	}

	std::cout << hue::green << "(+) " << hue::white << "Initialized VMMDLL" << std::endl;

	int fixResult = mem.FixDTB();

	std::cout << "\n";

	if (fixResult == -1) { // fix needed but failed
		std::cout << hue::red << "(!) " << hue::white << "Failed to find correct dtb" << std::endl;
		return false;
	}

	if (fixResult == 0) { // fix needed and successfull
		if (!mem.cachePML4()) {
			std::cout << hue::red << "(!) " << hue::white << "Failed to cache tables" << std::endl;
			return false;
		}

		std::cout << hue::green << "(+) " << hue::white << "Cached tables" << std::endl;
	}

	if (fixResult == 1) // fix not needed
		std::cout << hue::green << "(+) " << hue::white << "Dtb fix and tables caching was not needed" << std::endl;

	// idk why sometimes it fails
	point::Base = mem.GetBaseAddress();
	if (!point::Base)
	{
		std::cout << hue::red << "(!) " << hue::white << "Failed to refresh process" << std::endl; // couldnt get base
		return false;
	}

	std::cout << hue::green << "(+) " << hue::white << "Successfully refreshed process" << std::endl;

	if (!mem.SCreate()) {
		std::cout << hue::red << "(!) " << hue::white << "Failed to initialize all handles" << std::endl;
		return false;
	}

	std::cout << hue::green << "(+) " << hue::white << "Scatter handles Created" << std::endl;	
	
	if (!mem.InitKeyboard()) 
	{
		std::cout << hue::yellow << "(/) " << hue::white << "Failed to initialize keyboard hotkeys" << std::endl;
	}
	else {
		settings::runtime::hotKeys = true;
		std::cout << hue::green << "(+) " << hue::white << "Initialized keyboard hotkeys" << std::endl;
	}
	std::cout << "i got here 1\n";
	// no longer any offset (for now)
	point::va_text = point::Base;
	//if (!update_va_text()) {
	//	std::cout << hue::red << "(!) " << hue::white << "Failed to get text_va" << std::endl;
	//	return false;
	//}

	// memory features
	{
		// refresh memory LOW PRIORITY
		feature RefreshLight = { memRefreshLight , 1, 5000 };
		memoryList.push_back(RefreshLight);

		// update uworld and basics LOW PRIORITY / FAST
		feature GDataUpdate = { newInfo , 1, 1000 };
		memoryList.push_back(GDataUpdate);

		// update local weapon projectile stats MID PRIORITY
		feature WeaponUpdate = { weaponUpdate, 1, 500 };
		memoryList.push_back(WeaponUpdate);

		// update player list MID PRIORITY
		feature PlayerListUpdate = { updatePlayerList , 1, 1000 };
		memoryList.push_back(PlayerListUpdate);

		// update keys MID/HIGH PRIORITY
		feature KeysUpdate = { memKeysUpdate , 1, 5 };
		memoryList.push_back(KeysUpdate);

	}

	// main thread features
	{
		// health checks (not many)
		feature HealthCheck = { healthChecks, 1, 100 };
		mainList.push_back(HealthCheck);

		// aimbot
		feature Aimbot = { aim::updateAimbot, 1, 5 };
		mainList.push_back(Aimbot);

		// triggerbot
		feature Triggerbot = { aim::updateTriggerBot, 1, 5 };
		mainList.push_back(Triggerbot);

		// debugging drawings
		feature Debug = { esp::Debug, 1, -1 };
		mainList.push_back(Debug);

		// update camera and players location
		feature PlayersUpdate = { MainUpdate , 1, -1 };
		mainList.push_back(PlayersUpdate);

		// esp 
		feature Walls = { esp::renderPlayers, 1, -1 };
		mainList.push_back(Walls);

		// radar
		feature Radar = { radar::Render, 1, -1 };
		mainList.push_back(Radar);
	}
	std::cout << "i got here 1\n";
	return true;
}

void on_exit() {
	CloseHandle(hSerial);
	std::cout << "i got here 2\n";

	if (!settings::runtime::headless) {
		fclose(stdin);
		fclose(stdout);
		fclose(stderr);
		std::cout << "i got here 3\n";

	}
}

void memoryloop() {

	if (settings::runtime::criticalPriority) {
		// set thread priority
		std::cout << "i got here 4\n";
		if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
			std::cout << hue::yellow << "(/) " << hue::white << "Failed to set critical priority to memory thread" << std::endl;
	}

	// never quit?
	while (true) {
		auto start = std::chrono::high_resolution_clock::now();
		std::cout << "i got here 5\n";

		for (feature& i : memoryList) {
			std::cout << "i got here 6\n";

			if ((chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - i.lasttime) >= i.period) {
				i.lasttime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
				i.func();
				std::cout << "i got here 7\n";
			}

		}
		std::cout << "i got here 8\n";
		__int64 elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();
		std::cout << "i got here 9\n";
		stats::memoryThreadData.addValue(static_cast<float>(elapsed));
		std::cout << "i got here 10\n";
	}
}

void mainFeatures() {
	for (feature& i : mainList) {
		std::cout << "i got here 11\n";
		if ((chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count() - i.lasttime) >= i.period) {
			std::cout << "i got here 12\n";
			i.lasttime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now().time_since_epoch()).count();
			std::cout << "i got here 13\n";
			i.func(); // <====
			std::cout << "i got here 14\n";

		}
	}
}

void mainloop() {
	std::cout << "i got here 13\n";

	if (!settings::config::MoonlightAim) {
		if (ImGui::IsKeyPressed(ImGuiKey_Insert))
			std::cout << "i got here 14\n";
			settings::menu::open = !settings::menu::open;
	}
	else {
		if (GetAsyncKeyState(VK_INSERT) & 1) {
			settings::menu::open = !settings::menu::open;
		}
		ImGuiIO& io = ImGui::GetIO();
		io.DeltaTime = 1.0f / 60.0f;
		POINT p;
		GetCursorPos(&p);
		io.MousePos.x = p.x;
		io.MousePos.y = p.y;
		if (GetAsyncKeyState(0x1)) {
			io.MouseDown[0] = true;
			io.MouseClicked[0] = true;
			io.MouseClickedPos[0].x = io.MousePos.x;
			io.MouseClickedPos[0].x = io.MousePos.y;
		}
		else
			io.MouseDown[0] = false;
	}

	if (settings::menu::open)
		menu::Menu();

	// fov idk where to put it
	if (settings::config::Aimbot && settings::config::ShowAimFov) ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(settings::window::Width/2, settings::window::Height/2), settings::config::AimFov, ImColor(255,255,255,255), 1000, 1.f);

	mainFeatures();

	return;
}


INT APIENTRY WinMain(HINSTANCE instance, HINSTANCE, PSTR, INT cmd_show) {
	std::cout << "i got here 14\n";
	if (!settings::runtime::headless) {
		AllocConsole();
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
	}

	if (settings::runtime::criticalPriority) {
		// set thread priority
		std::cout << "i got here 15\n";

		if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
			std::cout << hue::yellow << "(/) " << hue::white << "Failed to set critical priority to main thread" << std::endl;
	}
	
	if (!settings::runtime::graphicsOnly) {
		std::cout << "i got here 16\n";

		if (!on_initialize()) {
			std::cout << hue::yellow << "(/) " << hue::white << "Press enter to exit" << std::endl;
			std::cin.get();
			std::cout << "i got here 17\n";

			return 1;
		}

		thread memoryThread(memoryloop);
		memoryThread.detach();
	}

	// wanted to make it run without a window for debugging not sure if needed
	if (!settings::runtime::windowless) {
		InitWindow(instance, cmd_show);

		while (UpdateWindow(mainloop)) {

		}

		DestroyWindow();
	}
	else {
		while (true) {
			std::cout << "i got here 18\n";

			auto start = std::chrono::high_resolution_clock::now();
			std::cout << "i got here 19\n";

			mainFeatures();
			std::cout << "i got here 20\n";

			__int64 elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();
			stats::mainThreadData.addValue(static_cast<float>(elapsed));
			std::cout << "i got here 21\n";

		}
	}
	std::cout << "i got here 22\n";

	on_exit();
	std::cout << "i got here 23\n";

	return 0;
}