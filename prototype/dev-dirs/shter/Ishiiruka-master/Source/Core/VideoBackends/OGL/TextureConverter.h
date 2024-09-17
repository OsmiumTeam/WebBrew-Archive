// Copyright 2008 Dolphin Emulator Project
// Licensed under GPLv2+
// Refer to the license.txt file included.

#pragma once

#include "Common/GL/GLUtil.h"

#include "VideoCommon/TextureCacheBase.h"
#include "VideoCommon/TextureDecoder.h"
#include "VideoCommon/VideoCommon.h"

namespace OGL
{

// Converts textures between formats using shaders
// TODO: support multiple texture formats
namespace TextureConverter
{

void Init();
void Shutdown();

void EncodeToRamYUYV(GLuint srcTexture, const TargetRectangle& sourceRc,
  u8* destAddr, u32 dstWidth, u32 dstStride, u32 dstHeight);

void DecodeToTexture(u32 xfbAddr, int srcWidth, int srcHeight, GLuint destTexture);

void EncodeToRamFromTexture(u8* dest_ptr, const EFBCopyFormat& format, u32 native_width,
  u32 bytes_per_row, u32 num_blocks_y, u32 memory_stride,
  bool is_depth_copy, const EFBRectangle& src_rect, bool scale_by_half);

}

}  // namespace OGL
