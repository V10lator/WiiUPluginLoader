/****************************************************************************
 * Copyright (C) 2018 Maschell
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ****************************************************************************/
#ifndef __TEXTURE_UTILS_UTILS_H_
#define __TEXTURE_UTILS_UTILS_H_
#include "backwardComp.h"
#include <stdint.h>
#include <video/shaders/Texture2DShader.h>

class TextureUtils {
public:
    static bool convertImageToTexture(const uint8_t *img, int32_t imgSize, void * texture);
    static void drawTexture(GX2Texture * texture, GX2Sampler* sampler, float x, float y, int32_t width, int32_t height, float alpha);
    static void copyToTexture(GX2ColorBuffer* sourceBuffer, GX2Texture * target);
private:
    TextureUtils() {}
    ~TextureUtils() {}

};
#endif
