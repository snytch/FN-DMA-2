#pragma once

#include "json.hpp"
using json = nlohmann::json;

namespace settings {

	// general
	namespace runtime {
		inline int version_major = 1;
		inline int version_minor = 2;
		inline bool criticalPriority = false;
		inline bool headless = false; // no console
		inline bool windowless = false; // no window (to fix)
		inline bool graphicsOnly = false; // just the menu
		inline std::string configFile = "configs.txt";

		// internal
		inline bool hotKeys = false;
	}

	// window settings
	namespace window {
		inline LPCWSTR ClassName = L"Castiel";
		inline LPCWSTR WindowName = L"Castiel";
		inline int Width = GetSystemMetrics(SM_CXSCREEN);
		inline int Height = GetSystemMetrics(SM_CYSCREEN);
	}

	// cheat internal settings
	namespace cheat {
		inline bool GData = true;
		inline bool Camera = true;
		inline bool PlayerList = true;
		inline bool Players = true;
	}

	// dma settings
	namespace dma {
		inline bool MemoryMap = true;

		inline json toJson() {
			return {
				{"MemoryMap", MemoryMap}
			};
		}
		inline void fromJson(const json& j) {
			if (j.contains("MemoryMap")) MemoryMap = j["MemoryMap"];
		}

	}

	// menus
	namespace menu {
		// temp
		inline bool open = true;

		// menus
		inline bool AdvancedDebug = false;
		inline bool PlayerList = false;
		inline bool Statistics = false;
		inline bool Internals = false;

		inline json toJson() {
			return {
				{"AdvancedDebug", AdvancedDebug},
				{"PlayerList", PlayerList},
				{"Statistics", Statistics},
				{"Internals", Internals}
			};
		}
		inline void fromJson(const json& j) {
			if (j.contains("AdvancedDebug")) AdvancedDebug = j["AdvancedDebug"];
			if (j.contains("PlayerList")) PlayerList = j["PlayerList"];
			if (j.contains("Statistics")) Statistics = j["Statistics"];
			if (j.contains("Internals")) Internals = j["Internals"];
		}
	}

	// kmbox
	namespace kmbox {

		inline bool SerialKmbox = false;
		inline bool NetKmbox = false;

		namespace net {
			inline std::string ip = "192.168.2.188";
			inline std::string port = "1234";
			inline std::string uuid = "1234ABCD";

			inline json toJson() {
				return {
					{"KmboxNetIp", ip},
					{"KmboxNetPort", port},
					{"KmboxNetUuid", uuid}
				};
			}
			inline void fromJson(const json& j) {
				if (j.contains("KmboxNetIp")) ip = j["KmboxNetIp"];
				if (j.contains("KmboxNetPort")) port = j["KmboxNetPort"];
				if (j.contains("KmboxNetUuid")) uuid = j["KmboxNetUuid"];
			}

			inline bool saveConfig() {
				std::ifstream file(settings::runtime::configFile);
				if (file.is_open()) {
					json combinedConfig;
					file >> combinedConfig;
					file.close();
					std::ofstream fileOut(settings::runtime::configFile);
					if (fileOut.is_open()) {
						combinedConfig["KmboxNet"] = kmbox::net::toJson();
						fileOut << combinedConfig.dump(4);
						fileOut.close();
					}
				}
				else {
					std::ofstream fileOut(settings::runtime::configFile);
					if (fileOut.is_open()) {
						json combinedConfig;
						combinedConfig["KmboxNet"] = kmbox::net::toJson();
						fileOut << combinedConfig.dump(4);
						fileOut.close();
					}
					return false;
				}

				return true;
			}

			inline bool loadConfig() {
				std::ifstream file(settings::runtime::configFile);
				if (file.is_open()) {
					json combinedConfig;
					file >> combinedConfig;
					file.close();
					if (combinedConfig.contains("KmboxNet")) {
						kmbox::net::fromJson(combinedConfig["KmboxNet"]);
					}
				}
				else {
					std::cout << hue::yellow << "(/) " << hue::white << "Failed to open configuration file" << std::endl;
					return false;
				}

				return true;
			}
		}
	}

	namespace config {
		// misc
		inline bool Fuser = false;
		inline bool vSync = false;

		// aim
		inline bool Aimbot = true;
		inline int AimFov = 100;
		inline bool ShowAimFov = false;
		inline float AimSmoothing = 5;
		inline int AimKey = 2;
		inline int AimKey2 = 70;
		inline int AimKey3 = 70;
		inline bool Prediction = true;
		inline float PredictionMultiplier = 1.f;
		inline bool MoonlightAim = false;

		inline float StepPerDegreeX = 10;
		inline float StepPerDegreeY = -10;

		// trigger
		inline bool TriggerBot = true;
		inline int TriggerDelay = 25; // ms
		inline int TriggerKey = 2;

		// esp
		inline bool Skeleton = true;
		inline bool Box = false;
		inline bool Distance = true;
		inline bool Bots = false;

		// radar
		inline bool Radar = true;
		inline int RadarX = 30;
		inline int RadarY = 30;
		inline int RadarXSize = 300;
		inline int RadarYSize = 300;
		inline int RadarScale = INT_MAX;
		inline float RadarZoom = 1.f;

		inline json toJson() {
			return {
				{"Fuser", Fuser},
				{"vSync", vSync},
				{"Aimbot", Aimbot},
				{"AimFov", AimFov},
				{"ShowAimFov", ShowAimFov},
				{"AimSmoothing", AimSmoothing},
				{"AimKey", AimKey},
				{"AimKey2", AimKey2},
				{"AimKey3", AimKey3},
				{"Prediction", Prediction},
				{"PredictionMultiplier", PredictionMultiplier},
				{"StepPerDegreeX", StepPerDegreeX},
				{"StepPerDegreeY", StepPerDegreeY},
				{"MoonlightAim", MoonlightAim},
				{"TriggerBot", TriggerBot},
				{"TriggerDelay", TriggerDelay},
				{"TriggerKey", TriggerKey},
				{"Skeleton", Skeleton},
				{"Box", Box},
				{"Distance", Distance},
				{"Bots", Bots},
				{"RadarX", RadarX},
				{"RadarY", RadarY},
				{"Radar", Radar},
				{"RadarXSize", RadarXSize},
				{"RadarYSize", RadarYSize},
				{"radarScale", RadarScale},
				{"RadarZoom", RadarZoom}
			};
		}
		inline void fromJson(const json& j) {
			if (j.contains("Fuser")) Fuser = j["Fuser"];
			if (j.contains("vSync")) vSync = j["vSync"];
			if (j.contains("Aimbot")) Aimbot = j["Aimbot"];
			if (j.contains("AimFov")) AimFov = j["AimFov"];
			if (j.contains("ShowAimFov")) ShowAimFov = j["ShowAimFov"];
			if (j.contains("AimSmoothing")) AimSmoothing = j["AimSmoothing"];
			if (j.contains("AimKey")) AimKey = j["AimKey"];
			if (j.contains("AimKey2")) AimKey2 = j["AimKey2"];
			if (j.contains("AimKey3")) AimKey3 = j["AimKey3"];
			if (j.contains("Prediction")) Prediction = j["Prediction"];
			if (j.contains("PredictionMultiplier")) PredictionMultiplier = j["PredictionMultiplier"];
			if (j.contains("StepPerDegreeX")) StepPerDegreeX = j["StepPerDegreeX"];
			if (j.contains("StepPerDegreeY")) StepPerDegreeY = j["StepPerDegreeY"];
			if (j.contains("MoonlightAim")) MoonlightAim = j["MoonlightAim"];
			if (j.contains("TriggerBot")) TriggerBot = j["TriggerBot"];
			if (j.contains("TriggerDelay")) TriggerDelay = j["TriggerDelay"];
			if (j.contains("TriggerKey")) TriggerKey = j["TriggerKey"];
			if (j.contains("AimFov")) AimFov = j["AimFov"];
			if (j.contains("Skeleton")) Skeleton = j["Skeleton"];
			if (j.contains("Box")) Box = j["Box"];
			if (j.contains("Distance")) Distance = j["Distance"];
			if (j.contains("Bots")) Bots = j["Bots"];
			if (j.contains("Radar")) Radar = j["Radar"];
			if (j.contains("RadarX")) RadarX = j["RadarX"];
			if (j.contains("RadarY")) RadarY = j["RadarY"];
			if (j.contains("RadarXSize")) RadarXSize = j["RadarXSize"];
			if (j.contains("RadarYSize")) RadarYSize = j["RadarYSize"];
			if (j.contains("RadarScale")) RadarScale = j["RadarScale"];
			if (j.contains("RadarZoom")) RadarZoom = j["RadarZoom"];
		}

	}
	
	inline bool saveConfig() {
		json combinedConfig;

		combinedConfig["Dma"] = dma::toJson();
		combinedConfig["Menu"] = menu::toJson();
		combinedConfig["KmboxNet"] = kmbox::net::toJson();
		combinedConfig["Config"] = config::toJson();

		std::ofstream file(settings::runtime::configFile);
		if (file.is_open()) {
			file << combinedConfig.dump(4);
			file.close();
			std::cout << hue::green << "(+) " << hue::white << "Saved configuration file" << std::endl;
		}
		else {
			std::cout << hue::yellow << "(/) " << hue::white << "Failed to open configuration file" << std::endl;
			return false;
		}

		return true;
	}

	inline bool loadConfig() {
		std::ifstream file(settings::runtime::configFile);
		if (file.is_open()) {
			json combinedConfig;
			file >> combinedConfig;
			file.close();

			if (combinedConfig.contains("Dma")) {
				dma::fromJson(combinedConfig["Dma"]);
			}
			if (combinedConfig.contains("Menu")) {
				menu::fromJson(combinedConfig["Menu"]);
			}
			if (combinedConfig.contains("KmboxNet")) {
				kmbox::net::fromJson(combinedConfig["KmboxNet"]);
			}
			if (combinedConfig.contains("Config")) {
				config::fromJson(combinedConfig["Config"]);
			}

			std::cout << hue::green << "(+) " << hue::white << "Loaded configuration file" << std::endl;
		}
		else {
			std::cout << hue::yellow << "(/) " << hue::white << "Failed to open configuration file" << std::endl;
			return false;
		}

		return true;
	}

}

struct feature {
	void (*func)();
	__int64 lasttime;
	__int64 period;
};