#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>
#include "Utility.hpp"
#include "Debug.hpp"

using namespace Vulkan_Test;

std::shared_ptr<std::vector<vk::AttachmentReference>> getAttachmentReferences()
{
    // vk::AttachmentReference構造体のattachmentメンバは「何番のアタッチメント」という形で
    // レンダーパスの中のアタッチメントを指定する
    // ここでは0を指定しているので0番のアタッチメントの意味
    std::shared_ptr<std::vector<vk::AttachmentReference>> result = std::make_shared<std::vector<vk::AttachmentReference>>();
    result->push_back(vk::AttachmentReference());
    (*result)[0].attachment = 0;
    (*result)[0].layout = vk::ImageLayout::eColorAttachmentOptimal;
    
    return result;
}

std::shared_ptr<vk::AttachmentReference> getDepthStencilAttachmentReference()
{
    std::shared_ptr<vk::AttachmentReference> result = std::make_shared<vk::AttachmentReference>();
    result->attachment = 1;
    result->layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    return result;
}

std::shared_ptr<std::vector<vk::SubpassDescription>> getSubpassDescription(std::vector<vk::AttachmentReference>& subpass0_attachmentReferences, vk::AttachmentReference& subpass0_depthAttachmentReference)
{
    std::shared_ptr<std::vector<vk::SubpassDescription>> result = std::make_shared<std::vector<vk::SubpassDescription>>();
    result->push_back(vk::SubpassDescription());

    (*result)[0].pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    (*result)[0].colorAttachmentCount = subpass0_attachmentReferences.size();
    (*result)[0].pColorAttachments = subpass0_attachmentReferences.data();
    (*result)[0].pDepthStencilAttachment = &subpass0_depthAttachmentReference;
    return result;
}
