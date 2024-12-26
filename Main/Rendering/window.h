#pragma once

// Include necessary Vulkan and ImGui headers
#include <vulkan/vulkan.h>
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_vulkan.h>
#include <windows.h>
#include <chrono>
#include <iostream>

// Forward declare the Win32 WndProc handler for ImGui
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Global Vulkan objects (note: for a real-world project, you would organize these carefully)
static VkInstance               g_VkInstance        = VK_NULL_HANDLE;
static VkPhysicalDevice         g_VkPhysicalDevice  = VK_NULL_HANDLE;
static VkDevice                 g_VkDevice          = VK_NULL_HANDLE;
static VkSwapchainKHR           g_VkSwapchain       = VK_NULL_HANDLE;
static VkQueue                  g_VkQueue           = VK_NULL_HANDLE;
static uint32_t                 g_QueueFamily       = (uint32_t)-1;
static VkSurfaceKHR             g_VkSurface         = VK_NULL_HANDLE;
static VkRenderPass             g_VkRenderPass      = VK_NULL_HANDLE;
static VkCommandPool            g_CommandPool       = VK_NULL_HANDLE;
static VkCommandBuffer          g_CommandBuffer     = VK_NULL_HANDLE;
static std::vector<VkImageView> g_SwapchainViews;
static std::vector<VkFramebuffer> g_Framebuffers;

static HWND         g_Window      = nullptr;
static WNDCLASSEXW  g_Wc          = {};
static bool         g_Running     = true;

// Simple error check helper
static void check_vk_result(VkResult err)
{
    if (err != VK_SUCCESS)
    {
        std::cerr << "Vulkan Error: " << err << std::endl;
        abort();
    }
}

// Win32 procedure
LRESULT CALLBACK window_procedure(HWND window, UINT message, WPARAM w_param, LPARAM l_param)
{
    if (ImGui_ImplWin32_WndProcHandler(window, message, w_param, l_param))
        return 0L;

    if (message == WM_DESTROY)
    {
        PostQuitMessage(0);
        return 0L;
    }
    return DefWindowProc(window, message, w_param, l_param);
}

// Create basic Vulkan instance with surface (for demonstration)
bool CreateVulkanInstance(HINSTANCE instance)
{
    // 1) Create Vulkan Instance
    VkApplicationInfo app_info = {};
    app_info.sType            = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.apiVersion       = VK_API_VERSION_1_1;
    app_info.pApplicationName = "ExampleVulkanApp";

    const char* extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
    VkInstanceCreateInfo create_info = {};
    create_info.sType                    = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    create_info.pApplicationInfo         = &app_info;
    create_info.enabledExtensionCount    = 2;
    create_info.ppEnabledExtensionNames  = extensions;
    create_info.enabledLayerCount        = 0;
    create_info.ppEnabledLayerNames      = nullptr;

    VkResult result = vkCreateInstance(&create_info, nullptr, &g_VkInstance);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create Vulkan instance." << std::endl;
        return false;
    }

    // 2) Create Win32 surface
    VkWin32SurfaceCreateInfoKHR create_surface_info = {};
    create_surface_info.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    create_surface_info.hinstance = instance;
    create_surface_info.hwnd      = g_Window;

    result = vkCreateWin32SurfaceKHR(g_VkInstance, &create_surface_info, nullptr, &g_VkSurface);
    check_vk_result(result);

    return true;
}

// Select physical device, create logical device + queue
bool InitVulkanDevice()
{
    // Enumerate physical devices
    uint32_t gpu_count;
    vkEnumeratePhysicalDevices(g_VkInstance, &gpu_count, nullptr);
    if (gpu_count == 0)
    {
        std::cerr << "No Vulkan-compatible GPU detected." << std::endl;
        return false;
    }

    std::vector<VkPhysicalDevice> gpus(gpu_count);
    vkEnumeratePhysicalDevices(g_VkInstance, &gpu_count, gpus.data());

    // For simplicity, pick the first device
    g_VkPhysicalDevice = gpus[0];

    // Find a queue that supports graphics
    uint32_t queue_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(g_VkPhysicalDevice, &queue_count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_props(queue_count);
    vkGetPhysicalDeviceQueueFamilyProperties(g_VkPhysicalDevice, &queue_count, queue_props.data());

    for (uint32_t i = 0; i < queue_count; i++)
    {
        // Check if this queue can support graphics
        if (queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            g_QueueFamily = i;
            break;
        }
    }

    if (g_QueueFamily == (uint32_t)-1)
    {
        std::cerr << "No suitable graphics queue found." << std::endl;
        return false;
    }

    float queue_priority = 1.0f;
    VkDeviceQueueCreateInfo queue_info = {};
    queue_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.queueFamilyIndex = g_QueueFamily;
    queue_info.queueCount       = 1;
    queue_info.pQueuePriorities = &queue_priority;

    VkDeviceCreateInfo device_info = {};
    device_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.queueCreateInfoCount    = 1;
    device_info.pQueueCreateInfos       = &queue_info;

    VkResult result = vkCreateDevice(g_VkPhysicalDevice, &device_info, nullptr, &g_VkDevice);
    check_vk_result(result);

    vkGetDeviceQueue(g_VkDevice, g_QueueFamily, 0, &g_VkQueue);
    return true;
}

// Create a basic swapchain and render pass
bool CreateSwapchainAndRenderPass(int width, int height)
{
    // For brevity, we omit full swapchain creation details. 
    // Placeholder for demonstration only.

    // Normally, you'd query surface capabilities, choose formats/present modes, etc.
    // Below is minimal/no-op style code, not production-ready.

    VkSwapchainCreateInfoKHR swapchain_info = {};
    swapchain_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface          = g_VkSurface;
    swapchain_info.minImageCount    = 2;
    swapchain_info.imageFormat      = VK_FORMAT_B8G8R8A8_UNORM;
    swapchain_info.imageColorSpace  = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapchain_info.imageExtent      = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.queueFamilyIndexCount = 0;
    swapchain_info.pQueueFamilyIndices   = nullptr;
    swapchain_info.preTransform     = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    swapchain_info.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode      = VK_PRESENT_MODE_FIFO_KHR;
    swapchain_info.clipped          = VK_TRUE;

    VkResult result = vkCreateSwapchainKHR(g_VkDevice, &swapchain_info, nullptr, &g_VkSwapchain);
    check_vk_result(result);

    // Create a basic render pass (single attachment)
    VkAttachmentDescription attachment = {};
    attachment.format         = VK_FORMAT_B8G8R8A8_UNORM;
    attachment.samples        = VK_SAMPLE_COUNT_1_BIT;
    attachment.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    attachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment.initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment.finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment = {};
    color_attachment.attachment = 0;
    color_attachment.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount    = 1;
    subpass.pColorAttachments       = &color_attachment;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass    = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass    = 0;
    dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments    = &attachment;
    render_pass_info.subpassCount    = 1;
    render_pass_info.pSubpasses     = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies   = &dependency;

    result = vkCreateRenderPass(g_VkDevice, &render_pass_info, nullptr, &g_VkRenderPass);
    check_vk_result(result);

    return true;
}

// Setup ImGui Vulkan
bool InitImGuiVulkan()
{
    // Must create a Vulkan descriptor pool for ImGui
    VkDescriptorPoolSize pool_sizes[] =
    {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets       = 1000 * IM_ARRAYSIZE(pool_sizes);
    pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
    pool_info.pPoolSizes    = pool_sizes;

    VkDescriptorPool g_DescriptorPool = VK_NULL_HANDLE;
    VkResult result = vkCreateDescriptorPool(g_VkDevice, &pool_info, nullptr, &g_DescriptorPool);
    check_vk_result(result);

    // Initialize ImGui for Win32 + Vulkan
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(g_Window);

    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance       = g_VkInstance;
    init_info.PhysicalDevice = g_VkPhysicalDevice;
    init_info.Device         = g_VkDevice;
    init_info.QueueFamily    = g_QueueFamily;
    init_info.Queue          = g_VkQueue;
    init_info.PipelineCache  = VK_NULL_HANDLE;
    init_info.DescriptorPool = g_DescriptorPool;
    init_info.Subpass        = 0;
    init_info.MinImageCount  = 2;
    init_info.ImageCount     = 2; // match the swapchain image count
    init_info.MSAASamples    = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplVulkan_Init(&init_info, g_VkRenderPass);

    // Upload fonts (placeholder: removed fonts references as requested)
    {
        // Begin a temporary command buffer for font upload
        // Command pool creation, etc., omitted for brevity
        VkCommandBuffer cmd = VK_NULL_HANDLE; 
        // Typically you'd allocate a command buffer from a pool for uploading.
        // The actual commands are not shown here to keep things minimal.

        // ImGui_ImplVulkan_CreateFontsTexture(cmd);
        // end command buffer, submit, wait, and then:
        // ImGui_ImplVulkan_DestroyFontUploadObjects();
    }

    return true;
}

bool InitWindow(HINSTANCE instance, INT cmd_show)
{
    g_Wc.cbSize        = sizeof(WNDCLASSEXW);
    g_Wc.style         = CS_HREDRAW | CS_VREDRAW;
    g_Wc.lpfnWndProc   = window_procedure;
    g_Wc.hInstance     = instance;
    g_Wc.lpszClassName = L"VulkanWindowClass";

    RegisterClassExW(&g_Wc);

    // Create the Win32 window
    g_Window = CreateWindowExW(
        0,
        g_Wc.lpszClassName,
        L"Vulkan ImGui Example",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        1280, // Default width
        720,  // Default height
        nullptr,
        nullptr,
        g_Wc.hInstance,
        nullptr
    );

    if (!g_Window)
    {
        std::cerr << "Failed to create window." << std::endl;
        return false;
    }

    ShowWindow(g_Window, cmd_show);
    UpdateWindow(g_Window);

    // Create Vulkan instance + device, swapchain, etc.
    if (!CreateVulkanInstance(instance))
        return false;
    if (!InitVulkanDevice())
        return false;

    RECT rect;
    if (!GetClientRect(g_Window, &rect))
        return false;
    int width  = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    if (!CreateSwapchainAndRenderPass(width, height))
        return false;
    if (!InitImGuiVulkan())
        return false;

    return true;
}

// Main loop update
bool UpdateWindow(bool (*mainfunc)())
{
    auto start = std::chrono::high_resolution_clock::now();

    MSG msg;
    while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

        if (msg.message == WM_QUIT)
        {
            g_Running = false;
            return false;
        }
    }

    // Start the ImGui frame for Vulkan
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    // Run user function (place your UI or rendering calls here)
    if (!mainfunc())
        return false;

    ImGui::Render();

    // Normally you'd record commands into a command buffer, begin render pass, etc.
    // For brevity, we skip detailed Vulkan rendering steps.

    // Present the swapchain
    // Typically you'd call vkQueuePresentKHR here with a VkPresentInfoKHR.

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::high_resolution_clock::now() - start).count();
    // ... do something with 'elapsed' if needed

    return true;
}

// Cleanup
bool DestroyVulkan()
{
    vkDeviceWaitIdle(g_VkDevice);

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    if (g_VkSwapchain)
        vkDestroySwapchainKHR(g_VkDevice, g_VkSwapchain, nullptr);
    if (g_VkRenderPass)
        vkDestroyRenderPass(g_VkDevice, g_VkRenderPass, nullptr);
    if (g_VkSurface)
        vkDestroySurfaceKHR(g_VkInstance, g_VkSurface, nullptr);
    if (g_VkDevice)
        vkDestroyDevice(g_VkDevice, nullptr);
    if (g_VkInstance)
        vkDestroyInstance(g_VkInstance, nullptr);

    return true;
}

bool DestroyWindow_()
{
    DestroyVulkan();

    if (g_Window)
        ::DestroyWindow(g_Window);
    UnregisterClassW(g_Wc.lpszClassName, g_Wc.hInstance);

    return true;
}
