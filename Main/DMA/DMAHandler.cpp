#include "../console/console.h"

#include "DMAHandler.h"

#include <fstream>
#include <chrono>
#include <thread>
#include <leechcore.h>
#include <filesystem>

uint64_t cbSize = 0x80000;
//callback for VfsFileListU
VOID cbAddFile(_Inout_ HANDLE h, _In_ LPCSTR uszName, _In_ ULONG64 cb, _In_opt_ PVMMDLL_VFS_FILELIST_EXINFO pExInfo) 
{
	if (strcmp(uszName, "dtb.txt") == 0)
		cbSize = cb;
}

struct Info {
	uint32_t index;
	uint32_t process_id;
	uint64_t dtb;
	uint64_t kernelAddr;
	std::string name;
};

void clearLine() {
	std::cout << "                                                                                                                \r" << std::flush;
}

//Directory Table Base requires: symsrv.dll, dbghelp.dll and info.db
int DMAHandler::FixDTB()
{

	PVMMDLL_MAP_MODULEENTRY moduleEntry;
	bool result = VMMDLL_Map_GetModuleFromNameU(this->DMA_HANDLE, this->GetPID(), (LPSTR)this->GameName.c_str(), &moduleEntry, NULL);
	if (result)
		return 1; //Doesn't need to be patched lol

	if (!VMMDLL_InitializePlugins(this->DMA_HANDLE)) {
		std::cout << hue::yellow << "(/) " << hue::white << "Failed to initialize VMMDLL plugins\r" << std::flush;
		return -1;
	}

	std::cout << hue::green << "(+) " << hue::white << "Initializing plugins..\r" << std::flush;

	//have to sleep a little or we try reading the file before the plugin initializes fully
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

	while (true) {
		BYTE bytes[4] = { 0 };
		DWORD i = 0;
		auto nt = VMMDLL_VfsReadW(this->DMA_HANDLE, (LPWSTR)L"\\misc\\procinfo\\progress_percent.txt", bytes, 3, &i, 0);
		if (nt == VMMDLL_STATUS_SUCCESS && atoi((LPSTR)bytes) >= 100)
			break;

		if (nt == VMMDLL_STATUS_SUCCESS) {
			std::string content(reinterpret_cast<char*>(bytes));
			clearLine();
			std::cout << hue::green << "(+) " << hue::white << "Initializing VMMDLL plugins.. (" << content << "%)\r" << std::flush;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	VMMDLL_VFS_FILELIST2 VfsFileList;
	VfsFileList.dwVersion = VMMDLL_VFS_FILELIST_VERSION;
	VfsFileList.h = 0;
	VfsFileList.pfnAddDirectory = 0;
	VfsFileList.pfnAddFile = cbAddFile;//dumb af callback who made this system

	result = VMMDLL_VfsListU(this->DMA_HANDLE, (LPSTR)"\\misc\\procinfo\\", &VfsFileList);
	if (!result)
		return -1;

	//read the data from the txt and parse it
	const size_t buffer_size = cbSize;
	BYTE* bytes = new BYTE[buffer_size];
	DWORD j = 0;
	auto nt = VMMDLL_VfsReadW(this->DMA_HANDLE, (LPWSTR)L"\\misc\\procinfo\\dtb.txt", bytes, buffer_size - 1, &j, 0);
	if (nt != VMMDLL_STATUS_SUCCESS) {
		delete[] bytes;
		return -1;
	}

	clearLine();
	std::cout << hue::green << "(+) " << hue::white << "Recovering possible dtb's..\r" << std::flush;

	std::vector<uint64_t> possibleDTBs;
	char* pLineStart = reinterpret_cast<char*>(bytes);
	for (size_t i = 0; i < 2500; ++i) { //Loop over it 1000 times, assumign they'll never have more than 1k processes ran at the same time
		char* pLineEnd = strchr(pLineStart, '\n');
		if (pLineEnd == nullptr)
			break;

		*pLineEnd = '\0';

		Info info = {};
		std::istringstream iss(pLineStart);
		iss >> std::hex >> info.index >> info.process_id >> info.dtb >> info.kernelAddr;
		std::getline(iss, info.name);
		if (info.process_id == 0) {//parts that lack a name or have a NULL pid are suspects
			possibleDTBs.push_back(info.dtb);
		}
		if (strstr(info.name.c_str(), this->GameName.c_str()))
			possibleDTBs.push_back(info.dtb);

		pLineStart = pLineEnd + 1;
	}

	clearLine();
	std::cout << hue::green << "(+) " << hue::white << "Trying possible dtb's..\r" << std::flush;

	//loop over possible dtbs and set the config to use it til we find the correct one
	for (size_t i = 0; i < possibleDTBs.size(); i++) {
		auto dtb = possibleDTBs[i];
		VMMDLL_ConfigSet(this->DMA_HANDLE, VMMDLL_OPT_PROCESS_DTB | this->GetPID(), dtb);
		PVMMDLL_MAP_MODULEENTRY moduleEntry;
		bool result = VMMDLL_Map_GetModuleFromNameU(this->DMA_HANDLE, this->GetPID(), (LPSTR)this->GameName.c_str(), &moduleEntry, NULL);
		if (result) {
			delete[] bytes;
			clearLine();
			std::cout << hue::green << "(+) " << hue::white << "Correct dtb found\r" << std::flush;
			processInfo.dtb = dtb;
			return 0;
		}
	}

	delete[] bytes;
	return -1;
}

ULONG64 DMAHandler::GetModuleAddress(std::wstring modulename)
{
	if (DMAHandler::Modules.contains(modulename))
		return DMAHandler::Modules[modulename];
	DMAHandler::Modules[modulename] = VMMDLL_ProcessGetModuleBase(this->DMA_HANDLE, GetPID(), (LPWSTR)modulename.c_str());
	return DMAHandler::Modules[modulename];
}
void DMAHandler::log(const char* fmt, ...)
{
	//small check before processing
	if (strlen(fmt) > 2000) return;
	char logBuffer[2001] = { 0 };

	//time related
	const auto now_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
	std::tm time_info;
	localtime_s(&time_info, &now_time_t);
	// Format time as a string
	std::ostringstream oss;
	oss << std::put_time(&time_info, "%H:%M:%S");
	va_list args;
	va_start(args, fmt);
	vsprintf_s(logBuffer, 2000, fmt, args);
	printf("[DMA @ %s]: %s\n", oss.str().c_str(), logBuffer);
}

void DMAHandler::assertNoInit() const
{
	if (!DMA_HANDLE || !PROCESS_INITIALIZED)
	{
		log("DMA or process not inizialized!");
		throw new std::string("DMA not inizialized!");
	}

}

void DMAHandler::retrieveScatter(VMMDLL_SCATTER_HANDLE handle, void* buffer, void* target, SIZE_T size)
{
	if (!handle) {
		log("Invalid handle!");
		return;
	}
	DWORD bytesRead = 0;
	if (!VMMDLL_Scatter_Read(handle, reinterpret_cast<ULONG64>(target), size, static_cast<PBYTE>(buffer), &bytesRead))
		log("Scatter read for %p failed partly or full! Bytes written: %d/%d", target, bytesRead, size);
}

int DMAHandler::Init(const wchar_t* wname, bool memMap)
{

	WGameName = wname;
	GameName = std::string(WGameName.begin(), WGameName.end());
	if (!DMA_HANDLE)
	{
		modules.VMM = LoadLibraryA("vmm.dll");
		modules.FTD3XX = LoadLibraryA("FTD3XX.dll");
		modules.LEECHCORE = LoadLibraryA("leechcore.dll");
		LoadLibraryA("dbghelp.dll");
		LoadLibraryA("symsrv.dll");

		if (!modules.VMM)
		{
			std::cout << hue::red << "(!) " << hue::white << "Could not load vmm.dll" << std::endl;
			return -1;
		}

		if (!modules.VMM)
		{
			std::cout << hue::red << "(!) " << hue::white << "Could not load FTD3XX.dll" << std::endl;
			return -1;
		}

		if (!modules.VMM)
		{
			std::cout << hue::red << "(!) " << hue::white << "Could not load leechcore.dll" << std::endl;
			return -1;
		}

		LPCSTR args[] = { (LPCSTR)"", (LPCSTR)"-device", (LPCSTR)"fpga", (LPCSTR)"-norefresh", (LPCSTR)"", (LPCSTR)"" };
		DWORD argc = 4;


		if (memMap)
		{
			if (!DumpMemoryMap())
			{
				std::cout << hue::yellow << "(/) " << hue::white << "Failed to dump memory" << std::endl;
			}
			else
			{
				//Get Path to executable
				char buffer[MAX_PATH];
				GetModuleFileNameA(nullptr, buffer, MAX_PATH);
				//Remove the executable name


				//Add the memory map to the arguments and increase arg count.
				args[argc++] = const_cast<LPCSTR>("-memmap");
				args[argc++] = const_cast<LPCSTR>("mmap.txt");
			}
		}

		DMA_HANDLE = VMMDLL_Initialize(argc, args);
		if (!DMA_HANDLE)
		{
			std::cout << hue::red << "(!) " << hue::white << "Failed to connect to fpga device" << std::endl;
			return -1;
		}


		ULONG64 FPGA_ID, DEVICE_ID;

		VMMDLL_ConfigGet(DMA_HANDLE, LC_OPT_FPGA_FPGA_ID, &FPGA_ID);
		VMMDLL_ConfigGet(DMA_HANDLE, LC_OPT_FPGA_DEVICE_ID, &DEVICE_ID);
	}

	// Convert the wide string to a standard string because VMMDLL_PidGetFromName expects LPSTR.
	std::wstring ws(wname);
	const std::string str(ws.begin(), ws.end());

	processInfo.name = str;
	processInfo.wname = wname;
	if (!VMMDLL_PidGetFromName(DMA_HANDLE, const_cast<char*>(processInfo.name.c_str()), &processInfo.pid))
	{
		std::cout << hue::red << "(!) " << hue::white << "Failed to find game" << std::endl;
		return -1;
	}
	else
		PROCESS_INITIALIZED = TRUE;

	return 0;
}

void DMAHandler::RefreshLight() {
	VMMDLL_ConfigSet(DMA_HANDLE, VMMDLL_OPT_REFRESH_FREQ_MEM_PARTIAL, 1);
	VMMDLL_ConfigSet(DMA_HANDLE, VMMDLL_OPT_REFRESH_FREQ_TLB_PARTIAL, 1);
	//VMMDLL_ConfigSet(DMA_HANDLE, VMMDLL_OPT_REFRESH_FREQ_FAST, 1);
}

bool DMAHandler::DumpMemoryMap()
{
	LPCSTR args[] = { (LPCSTR)"", (LPCSTR)"-device", (LPCSTR)"fpga", (LPCSTR)"-v" };
	if (const VMM_HANDLE handle = VMMDLL_Initialize(3, args)) {
		PVMMDLL_MAP_PHYSMEM pPhysMemMap = nullptr;
		if (VMMDLL_Map_GetPhysMem(handle, &pPhysMemMap)) {
			if (pPhysMemMap->dwVersion != VMMDLL_MAP_PHYSMEM_VERSION) {
				log("Invalid VMM Map Version\n");
				VMMDLL_MemFree(pPhysMemMap);
				VMMDLL_Close(handle);
				return false;
			}

			//Dump map to file
			std::stringstream sb;
			for (DWORD i = 0; i < pPhysMemMap->cMap; i++) {
				sb << std::setfill('0') << std::setw(4) << i << "  " << std::hex << pPhysMemMap->pMap[i].pa << "  -  " << (pPhysMemMap->pMap[i].pa + pPhysMemMap->pMap[i].cb - 1) << "  ->  " << pPhysMemMap->pMap[i].pa << std::endl;
			}
			std::ofstream nFile("mmap.txt");
			nFile << sb.str();
			nFile.close();

			VMMDLL_MemFree(pPhysMemMap);
			//Little sleep to make sure it's written to file.
			Sleep(3000);
		}
		VMMDLL_Close(handle);
		return true;
	}
	else
		return false;
}

bool DMAHandler::IsInitialized() const
{
	return DMA_HANDLE && PROCESS_INITIALIZED;
}

DWORD DMAHandler::GetPID() const
{
	//assertNoInit();
	return processInfo.pid;
}

ULONG64 DMAHandler::GetBaseAddress()
{
	if (!processInfo.base) {
		VMMDLL_ConfigSet(DMA_HANDLE, VMMDLL_OPT_REFRESH_ALL, 1);
		processInfo.base = VMMDLL_ProcessGetModuleBase(DMA_HANDLE, processInfo.pid, const_cast<LPWSTR>(processInfo.wname));
	}

	return processInfo.base;
}

bool DMAHandler::ReadBool(const ULONG64 address, const ULONG64 buffer, const SIZE_T size) const
{
	assertNoInit();
	DWORD dwBytesRead = 0;

#if COUNT_TOTAL_READSIZE
	readSize += size;
#endif

	bool status = VMMDLL_MemReadEx(DMA_HANDLE, processInfo.pid, address, reinterpret_cast<PBYTE>(buffer), size, &dwBytesRead, VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_ZEROPAD_ON_FAIL | VMMDLL_FLAG_NOPAGING_IO);

	if (dwBytesRead != size)
		log("Didnt read all bytes requested! Only read %llu/%llu bytes!", dwBytesRead, size);
	return status;
}

void DMAHandler::Read(const ULONG64 address, const ULONG64 buffer, const SIZE_T size)
{
	//assertNoInit();
	DWORD dwBytesRead = 0;

#if COUNT_TOTAL_READSIZE
	readSize += size;
#endif

	VMMDLL_MemReadEx(DMA_HANDLE, processInfo.pid, address, reinterpret_cast<PBYTE>(buffer), size, &dwBytesRead, VMMDLL_FLAG_NOCACHE | VMMDLL_FLAG_NOPAGING | VMMDLL_FLAG_ZEROPAD_ON_FAIL | VMMDLL_FLAG_NOPAGING_IO);

	//if (dwBytesRead != size)
	//	log("Didnt read all bytes requested! Only read %llu/%llu bytes!", dwBytesRead, size);
}


bool DMAHandler::Write(const ULONG64 address, const ULONG64 buffer, const SIZE_T size) const
{
	assertNoInit();
	return VMMDLL_MemWrite(DMA_HANDLE, processInfo.pid, address, reinterpret_cast<PBYTE>(buffer), size);
}

ULONG64 DMAHandler::PatternScan(const char* pattern, const std::string& mask, bool returnCSOffset)
{
	assertNoInit();
	//technically not write if you use the same pattern but once with RVA flag and once without
	//but i dont see any case where both results are needed so i cba
	static std::unordered_map<const char*, uint64_t> patternMap{};

	static std::vector<IMAGE_SECTION_HEADER> sectionHeaders;
	static char* textBuff = nullptr;
	static bool init = false;
	static DWORD virtualSize = 0;
	static uint64_t vaStart = 0;

	auto CheckMask = [](const char* Base, const char* Pattern, const char* Mask) {
		for (; *Mask; ++Base, ++Pattern, ++Mask) {
			if (*Mask == 'x' && *Base != *Pattern) {
				return false;
			}
		}
		return true;
		};

	if (patternMap.contains(pattern))
		return patternMap[pattern];

	if (!init)
	{
		init = true;

		static IMAGE_DOS_HEADER dosHeader;
		static IMAGE_NT_HEADERS ntHeaders;

		dosHeader = Read<IMAGE_DOS_HEADER>(GetBaseAddress());


		if (dosHeader.e_magic != IMAGE_DOS_SIGNATURE)
			throw std::runtime_error("dosHeader.e_magic invalid!");

		ntHeaders = Read<IMAGE_NT_HEADERS>(GetBaseAddress() + dosHeader.e_lfanew);

		if (ntHeaders.Signature != IMAGE_NT_SIGNATURE)
			throw std::runtime_error("ntHeaders.Signature invalid!");

		const DWORD sectionHeadersSize = ntHeaders.FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
		sectionHeaders.resize(ntHeaders.FileHeader.NumberOfSections);

		Read(GetBaseAddress() + dosHeader.e_lfanew + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), reinterpret_cast<DWORD64>(sectionHeaders.data()), sectionHeadersSize);


		for (const auto& section : sectionHeaders) {
			std::string sectionName(reinterpret_cast<const char*>(section.Name));
			if (sectionName == ".text") {
				textBuff = static_cast<char*>(calloc(section.Misc.VirtualSize, 1));
				Read(GetBaseAddress() + section.VirtualAddress, reinterpret_cast<DWORD64>(textBuff), section.Misc.VirtualSize);
				virtualSize = section.Misc.VirtualSize;
				vaStart = GetBaseAddress() + section.VirtualAddress;
			}
		}
	}


	const int length = virtualSize - mask.length();

	for (int i = 0; i <= length; ++i)
	{
		char* addr = &textBuff[i];

		if (!CheckMask(addr, pattern, mask.c_str()))
			continue;

		const uint64_t uAddr = reinterpret_cast<uint64_t>(addr);

		if (returnCSOffset)
		{
			const auto res = vaStart + i + *reinterpret_cast<int*>(uAddr + 3) + 7;
			patternMap.insert(std::pair(pattern, res));
			return res;
		}

		const auto res = vaStart + i;
		patternMap.insert(std::pair(pattern, res));
		return res;
	}
	return 0;
}

void DMAHandler::QueueScatterReadEx(VMMDLL_SCATTER_HANDLE handle, uint64_t addr, void* bffr, size_t size) const
{
	//assertNoInit();

	DWORD memoryPrepared = NULL;
	VMMDLL_Scatter_PrepareEx(handle, addr, size, static_cast<PBYTE>(bffr), &memoryPrepared);
}

void DMAHandler::ExecuteScatterRead(VMMDLL_SCATTER_HANDLE handle) const
{
	//assertNoInit();

	VMMDLL_Scatter_ExecuteRead(handle);
	//Clear after using it
	VMMDLL_Scatter_Clear(handle, processInfo.pid, NULL);

	VMMDLL_Scatter_CloseHandle(handle);
}

void DMAHandler::QueueScatterWriteEx(VMMDLL_SCATTER_HANDLE handle, uint64_t addr, void* bffr, size_t size) const
{
	assertNoInit();

	if (!VMMDLL_Scatter_PrepareWrite(handle, addr, static_cast<PBYTE>(bffr), size)) {
		log("failed to prepare scatter write at 0x%p\n", addr);
	}
}

void DMAHandler::ExecuteScatterWrite(VMMDLL_SCATTER_HANDLE handle) const
{
	assertNoInit();

	if (!VMMDLL_Scatter_Execute(handle)) {
		log("failed to Execute Scatter write\n");
	}
	//Clear after using it
	if (!VMMDLL_Scatter_Clear(handle, processInfo.pid, NULL)) {
		log("failed to clear write Scatter\n");
	}
}

VMMDLL_SCATTER_HANDLE DMAHandler::CreateScatterHandle() const
{
	//assertNoInit();

	const VMMDLL_SCATTER_HANDLE ScatterHandle = VMMDLL_Scatter_Initialize(DMA_HANDLE, processInfo.pid, VMMDLL_FLAG_NOCACHE);
	//if (!ScatterHandle) log("failed to create scatter handle\n");
	return ScatterHandle;
}

void DMAHandler::CloseScatterHandle(VMMDLL_SCATTER_HANDLE& handle) const
{
	//assertNoInit();

	VMMDLL_Scatter_CloseHandle(handle);

	handle = nullptr;
}

void DMAHandler::CloseDMA()
{
	log("DMA closed!");
	DMA_HANDLE = nullptr;
	VMMDLL_Close(DMA_HANDLE);
}

#if COUNT_TOTAL_READSIZE

DWORD64 DMAHandler::GetTotalReadSize()
{
	return readSize;
}

void DMAHandler::ResetReadSize()
{
	log("Bytes read since last reset: %llu B, %llu KB, %llu MB", readSize, readSize / 1024, readSize / 1024 / 1024);
	readSize = 0;
}

#endif

