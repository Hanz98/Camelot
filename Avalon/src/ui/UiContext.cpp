/*
 * Copyright 2024 Jan Filip
 *
 * Licensed under the MIT License. You may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 *
 * https://opensource.org/licenses/MIT
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Avalon/src/ui/UiContext.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <implot.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <stdexcept>

namespace avalon {

namespace {
void checkVkResult(VkResult result) {
  if (result != VK_SUCCESS) {
    spdlog::error("ImGui Vulkan backend: VkResult {}",
                  static_cast<int>(result));
  }
}
}  // namespace

UiContext::UiContext(const UiInitInfo& info)
    : m_device(info.device), m_window(info.window) {
  if (info.instance == nullptr || m_device == nullptr || m_window == nullptr ||
      info.renderPass == VK_NULL_HANDLE) {
    spdlog::error(
        "UiContext: instance, device, window or render pass missing.");
    throw std::runtime_error(
        "UiContext: instance, device, window or render pass missing.");
  }
  IMGUI_CHECKVERSION();
  m_imgui = ImGui::CreateContext();
  m_implot = ImPlot::CreateContext();
  ImGui::SetCurrentContext(m_imgui);
  ImPlot::SetCurrentContext(m_implot);
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.IniFilename = nullptr;  // no imgui.ini next to the binary
  ImGui::StyleColorsDark();

  if (!ImGui_ImplGlfw_InitForVulkan(m_window->getWindow(), true)) {
    cleanUp();
    throw std::runtime_error("UiContext: ImGui GLFW backend failed.");
  }
  ImGui_ImplVulkan_InitInfo vk = {};
  vk.ApiVersion = kVulkanApiVersion;
  vk.Instance = info.instance->getInstance();
  vk.PhysicalDevice = m_device->getPhysicalDevice();
  vk.Device = m_device->getDevice();
  vk.QueueFamily = m_device->getGraphicsQueueFamily();
  vk.Queue = m_device->getGraphicsQueue();
  vk.DescriptorPoolSize = kDescriptorPoolSize;  // backend-owned pool
  vk.MinImageCount = info.minImageCount;
  vk.ImageCount = info.imageCount;
  vk.PipelineInfoMain.RenderPass = info.renderPass;
  vk.PipelineInfoMain.Subpass = info.subpass;
  vk.PipelineInfoMain.MSAASamples = info.samples;
  vk.CheckVkResultFn = &checkVkResult;
  if (!ImGui_ImplVulkan_Init(&vk)) {
    ImGui_ImplGlfw_Shutdown();
    cleanUp();
    throw std::runtime_error("UiContext: ImGui Vulkan backend failed.");
  }
  spdlog::debug("UiContext: ImGui {} ready.", IMGUI_VERSION);
}

UiContext::~UiContext() {
  if (m_imgui != nullptr) {
    if (m_device != nullptr) {
      m_device->waitIdle();
    }
    ImGui::SetCurrentContext(m_imgui);
    if (m_frameOpen) {
      ImGui::EndFrame();
      m_frameOpen = false;
    }
    if (ImGui::GetIO().BackendRendererUserData != nullptr) {
      ImGui_ImplVulkan_Shutdown();
    }
    if (ImGui::GetIO().BackendPlatformUserData != nullptr) {
      ImGui_ImplGlfw_Shutdown();
    }
  }
  cleanUp();
}

void UiContext::cleanUp() {
  if (m_implot != nullptr) {
    ImPlot::DestroyContext(m_implot);
    m_implot = nullptr;
  }
  if (m_imgui != nullptr) {
    ImGui::DestroyContext(m_imgui);
    m_imgui = nullptr;
  }
}

void UiContext::newFrame() {
  if (m_frameOpen) {
    discardFrame();
  }
  ImGui::SetCurrentContext(m_imgui);
  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  m_frameOpen = true;
}

void UiContext::render(VkCommandBuffer commandBuffer) {
  if (!m_frameOpen) {
    return;
  }
  ImGui::SetCurrentContext(m_imgui);
  ImGui::Render();
  ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
  m_frameOpen = false;
  ++m_renderedFrames;
}

void UiContext::discardFrame() {
  if (!m_frameOpen) {
    return;
  }
  ImGui::SetCurrentContext(m_imgui);
  ImGui::EndFrame();
  m_frameOpen = false;
}

VkDescriptorSet UiContext::registerTexture(VkSampler sampler, VkImageView view,
                                           VkImageLayout layout) const {
  ImGui::SetCurrentContext(m_imgui);
  return ImGui_ImplVulkan_AddTexture(sampler, view, layout);
}

void UiContext::unregisterTexture(VkDescriptorSet set) const {
  if (set == VK_NULL_HANDLE) {
    return;
  }
  ImGui::SetCurrentContext(m_imgui);
  ImGui_ImplVulkan_RemoveTexture(set);
}

void UiContext::setMinImageCount(uint32_t minImageCount) const {
  ImGui::SetCurrentContext(m_imgui);
  ImGui_ImplVulkan_SetMinImageCount(minImageCount);
}

bool UiContext::wantsMouse() const {
  ImGui::SetCurrentContext(m_imgui);
  return ImGui::GetIO().WantCaptureMouse;
}

bool UiContext::wantsKeyboard() const {
  ImGui::SetCurrentContext(m_imgui);
  return ImGui::GetIO().WantCaptureKeyboard;
}

}  // namespace avalon
