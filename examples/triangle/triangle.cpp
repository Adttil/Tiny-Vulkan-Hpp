#include <array>
#include <ranges>
#include <algorithm>
#include <print>

#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <tinyvkpp/tinyvkpp.hpp>

struct Frame
{
    VkImageView     backbuffer_view;
    VkFramebuffer   framebuffer;
    VkCommandPool   command_pool;
    VkCommandBuffer command_buffer;
    VkFence         fence;
    
    VkSemaphore     image_acquired_semaphore;
    VkSemaphore     render_complete_semaphore;
};

class Displayer
{
public:
    Displayer()
    {
        init_glfw();

        constexpr auto init_steps = 
            vk::transaction(&Displayer::create_window, &Displayer::destroy_window)
            | vk::transaction(&Displayer::create_instance, &Displayer::destroy_instance)
            | vk::transaction(&Displayer::create_device, &Displayer::destroy_device)
            | vk::transaction(&Displayer::set_queue)
            | vk::transaction(&Displayer::create_surface, &Displayer::destroy_surface)
            | vk::transaction(&Displayer::create_swapchain, &Displayer::destroy_swapchain)
            | vk::transaction(&Displayer::create_render_pass, &Displayer::destroy_render_pass)
            | vk::transaction(&Displayer::create_frames, &Displayer::destroy_frames);

        init_steps.submit(*this);
    }

    void run()
    {
        bool is_quit = false;

        while (not glfwWindowShouldClose(window_)) 
        {
            glfwPollEvents();
        }
    }

    ~Displayer() noexcept
    {
        destroy_frames();
        destroy_render_pass();
        destroy_swapchain();
        destroy_surface();
        destroy_device();
        destroy_instance();
        destroy_window();
    }
private:
    void init_glfw()
    {
        if(not glfwInit())
        {
            throw std::runtime_error{ "failed to init glfw" };
        }
    }

    void create_window()
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window_ = glfwCreateWindow(800, 720, "Hello Vulkan", nullptr, nullptr);
        if(not window_)
        {
            throw std::runtime_error{ "failed to create window" };
        }
    }

    void destroy_window() noexcept
    {
        glfwDestroyWindow(window_);
    }

    void create_instance()
    {        
        uint32_t glfw_extension_count;
        const auto glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);
        if(not glfw_extensions)
        {
            throw std::runtime_error{ "failed to glfwGetRequiredInstanceExtensions" };
        }

        const std::array layers{ "VK_LAYER_KHRONOS_validation" };

        const VkInstanceCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .ppEnabledLayerNames = vk::set_range(layers, info.enabledLayerCount),
            .enabledExtensionCount = glfw_extension_count,
            .ppEnabledExtensionNames = glfw_extensions
        };

        instance_ = vk::enhance<vkCreateInstance>(&info, nullptr)
            .value_or_throw(std::runtime_error{"failed to create instance"});

        for(auto extension : std::span{ glfw_extensions, glfw_extension_count })
        {
            std::println("{}", extension);
        }
    }

    void destroy_instance() noexcept
    {
        vkDestroyInstance(instance_, nullptr);
    }

    void select_gpu()
    {
        const auto gpus = vk::enumerate<vkEnumeratePhysicalDevices>(instance_)
            .value_or_throw(std::runtime_error{ "failed to enumerate gpus" });

        if(gpus.empty())
        {
            throw std::runtime_error{ "gpu not find" };
        }
        auto iter = std::ranges::find_if(gpus, [](const auto& gpu){
            return vk::get<vkGetPhysicalDeviceProperties>(gpu).deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        });
        if(iter != gpus.end())
        {
            gpu_ = *iter;
        } 
        else
        {
            gpu_ = gpus[0];
        }

        std::println("gpu: {}", vk::get<vkGetPhysicalDeviceProperties>(gpu_).deviceName);
    }

    void select_queue_family()
    {
        auto& self = *this;
        const auto queue_family_properties = vk::enumerate<vkGetPhysicalDeviceQueueFamilyProperties>(gpu_);
        const auto iter = std::ranges::find_if(queue_family_properties, [](const auto& properties){
            return properties.queueFlags & VK_QUEUE_GRAPHICS_BIT;
        });
        if(iter == queue_family_properties.end())
        {
            throw std::runtime_error{ "graphic queue not find" };
        }
        queue_family_ = iter - queue_family_properties.begin();
    }

    void create_device()
    {            
        select_gpu();
        select_queue_family();

        const char* const device_extensions[]{ 
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };

        const auto extension_properties = vk::enumerate<vkEnumerateDeviceExtensionProperties>(gpu_, nullptr)
            .value_or_throw(std::runtime_error{ "failed to EnumerateDeviceExtensionProperties" });

        const float queue_priority = 1.0f;
        const VkDeviceQueueCreateInfo queue_info{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = queue_family_,
            .pQueuePriorities = vk::set_range(queue_priority, queue_info.queueCount)
        };
        const VkPhysicalDeviceFeatures features{
            .geometryShader = true,
            .samplerAnisotropy = true
        };
        const VkDeviceCreateInfo create_info{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pQueueCreateInfos = vk::set_range(queue_info, create_info.queueCreateInfoCount),
            .ppEnabledExtensionNames = vk::set_range(device_extensions, create_info.enabledExtensionCount),
            .pEnabledFeatures = &features
        };

        device_ = vk::get<vkCreateDevice>(gpu_, &create_info, nullptr)
            .value_or_throw(std::runtime_error{ "failed to create device" });
    }

    void destroy_device() noexcept
    {
        vkDestroyDevice(device_, nullptr);
    }

    void set_queue() noexcept
    {
        queue_ = vk::get<vkGetDeviceQueue>(device_, queue_family_, 0);
    }

    void create_surface()
    {
        surface_ = vk::get<glfwCreateWindowSurface>(instance_, window_, nullptr)
            .value_or_throw(std::runtime_error{ "failed to create surface" });
    }

    void destroy_surface() noexcept
    {
        vkDestroySurfaceKHR(instance_, surface_, nullptr);
    }

    VkSurfaceFormatKHR select_surface_format() const
    {
        constexpr VkFormat request_formats[] = { 
            VK_FORMAT_B8G8R8A8_UNORM, 
            VK_FORMAT_R8G8B8A8_UNORM, 
            VK_FORMAT_B8G8R8_UNORM, 
            VK_FORMAT_R8G8B8_UNORM 
        };
        constexpr VkColorSpaceKHR request_color_space = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
        const auto avail_formats = vk::enumerate<vkGetPhysicalDeviceSurfaceFormatsKHR>(gpu_, surface_)
            .value_or_throw(std::runtime_error{ "failed to get PhysicalDeviceSurfaceFormatsKHR" });
    
        if (avail_formats.size() == 1)
        {
            if (avail_formats[0].format != VK_FORMAT_UNDEFINED)
            {
                return avail_formats[0];   
            }
            else
            {
                VkSurfaceFormatKHR ret;
                ret.format = request_formats[0];
                ret.colorSpace = request_color_space;
                return ret;
            }
        }

        for (const VkFormat request_format : request_formats)
        {
            for (const auto& avail_format : avail_formats)
            {
                if (avail_format.format == request_format && avail_format.colorSpace == request_color_space)
                {
                    return avail_format;
                }
            }
        }
        // If none of the requested image formats could be found, use the first available
        return avail_formats[0];
    }

    VkPresentModeKHR select_present_mode() const
    {
#ifdef UNLIMITED_FRAME_RATE
        constexpr VkPresentModeKHR request_present_modes[] = {
            VK_PRESENT_MODE_MAILBOX_KHR, 
            VK_PRESENT_MODE_IMMEDIATE_KHR, 
            VK_PRESENT_MODE_FIFO_KHR 
        };
#else
        constexpr VkPresentModeKHR request_present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
#endif
        const auto avail_modes = vk::enumerate<vkGetPhysicalDeviceSurfacePresentModesKHR>(gpu_, surface_)
            .value_or_throw(std::runtime_error{ "failed to get PhysicalDeviceSurfacePresentModesKHR" });

        for (const VkPresentModeKHR request_present_mode : request_present_modes)
        {
            for (const auto& avail_mode : avail_modes)
            {
                if (request_present_mode == avail_mode)
                {
                    return request_present_mode;
                }
            }
        }
        return VK_PRESENT_MODE_FIFO_KHR; // Always available
    }

    static int min_image_count_by_present_mode(VkPresentModeKHR present_mode)
    {
        if (present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
            return 3;
        if (present_mode == VK_PRESENT_MODE_FIFO_KHR || present_mode == VK_PRESENT_MODE_FIFO_RELAXED_KHR)
            return 2;
        if (present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
            return 1;
        return 1;
    }

    void create_swapchain()
    {
        surface_format_ = select_surface_format();
        present_mode_ = select_present_mode();

        int w, h;
        glfwGetWindowSize(window_, &w, &h);

        uint32_t min_image_count = 2;
        if(min_image_count == 0)
        {
            min_image_count = min_image_count_by_present_mode(present_mode_);
        }
        const auto cap = vk::get<vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(gpu_, surface_)
            .value_or_throw(std::runtime_error{ "failed to get PhysicalDeviceSurfaceCapabilitiesKHR" });

        if (min_image_count < cap.minImageCount)
            min_image_count = cap.minImageCount;
        else if (cap.maxImageCount != 0 && min_image_count > cap.maxImageCount)
            min_image_count = cap.maxImageCount;

        if (cap.currentExtent.width == 0xffffffff)
        {
            width_ = w;
            height_ = h;
        }
        else
        {
            width_ = cap.currentExtent.width;
            height_ = cap.currentExtent.height;
        }

        const VkSwapchainCreateInfoKHR info = 
        {
            .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface = surface_,
            .minImageCount = min_image_count,
            .imageFormat = surface_format_.format,
            .imageColorSpace = surface_format_.colorSpace,
            .imageExtent = { width_, height_ },
            .imageArrayLayers = 1,
            .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,           // Assume that graphics family == present family
            .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
            .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode = present_mode_,
            .clipped = VK_TRUE,
            .oldSwapchain = nullptr
        };        
        
        swapchain_ = vk::get<vkCreateSwapchainKHR>(device_, &info, nullptr)
            .value_or_throw(std::runtime_error{ "failed to create swapchain" });
    }

    void destroy_swapchain() noexcept
    {
        vkDestroySwapchainKHR(device_, swapchain_, nullptr);
    }

    void create_render_pass()
    {
        const VkAttachmentDescription attachment{
            .format = surface_format_.format,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .loadOp = true ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        };

        const VkAttachmentReference color_attachment = 
        {
            .attachment = 0,
            .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };

        const VkSubpassDescription subpass = {
            .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .pColorAttachments = vk::set_range(color_attachment, subpass.colorAttachmentCount)
        };

        const VkSubpassDependency dependency = {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
        };

        const VkRenderPassCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .pAttachments = vk::set_range(attachment, info.attachmentCount),
            .pSubpasses = vk::set_range(subpass, info.subpassCount),
            .pDependencies = vk::set_range(dependency, info.dependencyCount)
        };

        render_pass_ = vk::get<vkCreateRenderPass>(device_, &info, nullptr)
            .value_or_throw(std::runtime_error{ "failed to create render pass" });
    }

    void destroy_render_pass() noexcept
    {
        vkDestroyRenderPass(device_, render_pass_, nullptr);
    }

    void create_frames()
    {
        const auto backbuffers = vk::enumerate<vkGetSwapchainImagesKHR>(device_, swapchain_)
            .value_or_throw(std::runtime_error{ "failed to GetSwapchainImages" });

        for(const auto& backbuffer : backbuffers)
        {
            create_frame(backbuffer);
        }
    }
    
    void destroy_frames() noexcept
    {
        while(not frames_.empty())
        {
            destroy_frame();
        }
    }

    void create_frame(VkImage backbuffer)
    {
        const auto steps = 
            vk::transaction(
                [](Displayer& self){ self.frames_.push_back({}); }, 
                [](Displayer& self){ self.frames_.pop_back(); self.destroy_frames(); 
            })
            | vk::transaction([=](auto& self){ self.create_backbuffer_view(backbuffer); }, &Displayer::destroy_backbuffer_view)
            | vk::transaction(&Displayer::create_framebuffer, &Displayer::destroy_framebuffer)
            | vk::transaction(&Displayer::create_command_pool, &Displayer::destroy_command_pool)
            | vk::transaction(&Displayer::create_command_buffer, &Displayer::destroy_command_buffer)
            | vk::transaction(&Displayer::create_fence, &Displayer::destroy_fence)
            | vk::transaction(&Displayer::create_image_acquired_semaphore, &Displayer::destroy_image_acquired_semaphore)
            | vk::transaction(&Displayer::create_render_complete_semaphore, &Displayer::destroy_render_complete_semaphore);

        steps.submit(*this);
    }

    void destroy_frame() noexcept
    {
        destroy_render_complete_semaphore();
        destroy_image_acquired_semaphore();
        destroy_fence();
        destroy_command_buffer();
        destroy_command_pool();
        destroy_framebuffer();
        destroy_backbuffer_view();
        frames_.pop_back();
    }

    void create_backbuffer_view(VkImage backbuffer)
    {
        const VkImageViewCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = backbuffer,
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = surface_format_.format,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_R,
                .g = VK_COMPONENT_SWIZZLE_G,
                .b = VK_COMPONENT_SWIZZLE_B,
                .a = VK_COMPONENT_SWIZZLE_A
            },
            .subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
        };

        frames_.back().backbuffer_view = vk::get<vkCreateImageView>(device_, &info, nullptr)
            .value_or_throw(std::runtime_error{ "failed to create backbuffer view" });
    }

    void destroy_backbuffer_view() noexcept
    {
        vkDestroyImageView(device_, frames_.back().backbuffer_view, nullptr);
    }

    void create_framebuffer()
    {
        const VkFramebufferCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = render_pass_,
            .pAttachments = vk::set_range(frames_.back().backbuffer_view, info.attachmentCount),
            .width = width_,
            .height = height_,
            .layers = 1
        };

        frames_.back().framebuffer = vk::get<vkCreateFramebuffer>(device_, &info, nullptr)
            .value_or_throw(std::runtime_error{ "failed to create framebuffer" });
    }

    void destroy_framebuffer() noexcept
    {
        vkDestroyFramebuffer(device_, frames_.back().framebuffer, nullptr);
    }

    void create_command_pool()
    {
        const VkCommandPoolCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .queueFamilyIndex = queue_family_
        };

        frames_.back().command_pool = vk::get<vkCreateCommandPool>(device_, &info, nullptr)
            .value_or_throw(std::runtime_error{ "failed to create command pool" });
    }

    void destroy_command_pool() noexcept
    {
        vkDestroyCommandPool(device_, frames_.back().command_pool, nullptr);
    }

    void create_command_buffer()
    {
        const VkCommandBufferAllocateInfo info{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool = frames_.back().command_pool,
            .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1
        };

        frames_.back().command_buffer = vk::get<vkAllocateCommandBuffers>(device_, &info)
            .value_or_throw(std::runtime_error{ "failed to allocate command buffer" });
    }

    void destroy_command_buffer() noexcept
    {
        vkFreeCommandBuffers(device_, frames_.back().command_pool, 1, &frames_.back().command_buffer);
    }

    void create_fence()
    {
        const VkFenceCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT
        };

        frames_.back().fence = vk::get<vkCreateFence>(device_, &info, nullptr)
            .value_or_throw(std::runtime_error{ "failed to create fence" });
    }

    void destroy_fence() noexcept
    {
        vkDestroyFence(device_, frames_.back().fence, nullptr);
    }

    void create_image_acquired_semaphore()
    {
        const VkSemaphoreCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };

        auto& frame = frames_.back();
        frame.image_acquired_semaphore = vk::get<vkCreateSemaphore>(device_, &info, nullptr)
            .value_or_throw(std::runtime_error{ "failed to create image acquired semaphore" });
    }

    void destroy_image_acquired_semaphore() noexcept
    {
        vkDestroySemaphore(device_, frames_.back().image_acquired_semaphore, nullptr);
    }

    void create_render_complete_semaphore()
    {
        const VkSemaphoreCreateInfo info{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
        };

        auto& frame = frames_.back();
        frame.render_complete_semaphore = vk::get<vkCreateSemaphore>(device_, &info, nullptr)
            .value_or_throw(std::runtime_error{ "failed to create render complete semaphore" });
    }

    void destroy_render_complete_semaphore() noexcept
    {
        vkDestroySemaphore(device_, frames_.back().render_complete_semaphore, nullptr);
    }

private:
    GLFWwindow* window_;
    VkInstance instance_;
    VkPhysicalDevice gpu_;
    uint32_t queue_family_;
    VkDevice device_;
    VkQueue queue_;
    VkSurfaceKHR surface_;

    VkSurfaceFormatKHR surface_format_;
    VkPresentModeKHR present_mode_;
    uint32_t width_;
    uint32_t height_;
    VkSwapchainKHR swapchain_;
    VkRenderPass render_pass_;

    vk::emplace_vector<Frame, 4> frames_;
    uint32_t frame_index_ = 0;
    uint32_t semaphore_index_ = 0;
};

class Renderer{};

int main(int, char*[])
{
    try
    {
        Displayer app{};
        app.run();
    }
    catch(std::exception& e)
    {
        std::println("err: {}", e.what());
        const char* description;
        glfwGetError(&description);
        std::println("glfw err: {}", description);
    }
    
    return 0;
}