/**
  ******************************************************************************
  * File Name          : STM32DMA.cpp
  * Description        : Software RGB565 painter translation unit.
  ******************************************************************************
  *
  * DMA2D acceleration is intentionally disabled in this validated fallback
  * configuration. The previous project mixed manually patched DMA2D painter
  * code with generated TouchGFX 4.26.1 code. A DMA2D error or a missing
  * completion interrupt could stop the very first frame.
  *
  * TouchGFXConfiguration.cpp now instantiates touchgfx::NoDMA. This file is
  * retained because STM32CubeIDE links it as a linked resource. Including the
  * official RGB565 software painter implementation here provides all paint
  * symbols exactly once without using DMA2D.
  ******************************************************************************
  */

#include <touchgfx/hal/PaintRGB565Impl.hpp>

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
