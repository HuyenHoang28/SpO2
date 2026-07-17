#ifndef TouchGFXHAL_HPP
#define TouchGFXHAL_HPP

#include <TouchGFXGeneratedHAL.hpp>

class TouchGFXHAL : public TouchGFXGeneratedHAL
{
public:
    TouchGFXHAL(touchgfx::DMA_Interface& dma,
                touchgfx::LCD& display,
                touchgfx::TouchController& tc,
                uint16_t width,
                uint16_t height)
        : TouchGFXGeneratedHAL(dma, display, tc, width, height)
    {
    }

    virtual void initialize() override;
    virtual void taskEntry() override;
    virtual void disableInterrupts() override;
    virtual void enableInterrupts() override;
    virtual void configureInterrupts() override;
    virtual void enableLCDControllerInterrupt() override;

    virtual void flushFrameBuffer() override
    {
        TouchGFXGeneratedHAL::flushFrameBuffer();
    }

    virtual void flushFrameBuffer(const touchgfx::Rect& rect) override;
    virtual bool sampleKey(uint8_t& key) override;

protected:
    virtual uint16_t* getTFTFrameBuffer() const override;
    virtual void setTFTFrameBuffer(uint16_t* address) override;
};

#endif // TouchGFXHAL_HPP
