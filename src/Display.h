#pragma once
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include "Config.h"

// What the screen should show. Display doesn't know about sales or WiFi.
struct View
{
    String title;
    String price;
    String footer;
    String tagId; // <- new: small label in the top-left corner
    bool onSale = false;
    bool compact = false;

    bool sameLayoutAs(const View &o) const
    {
        return title == o.title && price == o.price && tagId == o.tagId && compact == o.compact &&
               (footer.length() > 0) == (o.footer.length() > 0);
    }
};

// Font candidates, largest first. The first that fits is used; if none fit,
// the last one is used and the line scrolls. Add more entries to allow shrinking.
static const GFXfont *const kNameFonts[] = {&FreeSansBold18pt7b};
static const GFXfont *const kPriceFonts[] = {&FreeSansBold24pt7b, &FreeSansBold18pt7b};
static const GFXfont *const kStatusFonts[] = {&FreeSansBold18pt7b, &FreeSans9pt7b};

class Display
{
public:
    Display() : tft_(cfg::TFT_CS, cfg::TFT_DC, cfg::TFT_RST) {}

    bool begin()
    {
        tft_.init(240, 240); // keep your SPI mode arg here if your working code had one
        tft_.setSPISpeed(cfg::SPI_HZ);
        tft_.setRotation(1);
        tft_.fillScreen(0x0000);

        canvas_ = new GFXcanvas16(cfg::SCREEN_W, cfg::HALF_H);
        if (!canvas_->getBuffer())
            return false;
        canvas_->setTextWrap(false);
        return true;
    }

    // Cheap to call every second: only redraws what actually changed.
    void show(const View &v)
    {
        bool relayout = !hasView_ || !v.sameLayoutAs(view_);
        bool repaintPrice = relayout || v.footer != view_.footer || v.onSale != view_.onSale;
        view_ = v;
        hasView_ = true;

        if (relayout)
        {
            layout();
            renderName();
        }
        if (repaintPrice)
            renderPrice();
    }

    // Call every loop; advances scrolling text.
    void tick()
    {
        uint32_t ms = millis();
        if (!hasView_ || ms - lastScroll_ < (uint32_t)cfg::SCROLL_MS)
            return;
        lastScroll_ = ms;
        if (nameLine_.scroll)
        {
            nameLine_.step();
            renderName();
        }
        if (priceLine_.scroll)
        {
            priceLine_.step();
            renderPrice();
        }
    }

private:
    // One line of text: picks a font, centres or scrolls it.
    struct Line
    {
        String text;
        const GFXfont *font = nullptr;
        int w = 0, x1 = 0, baseline = 0, offset = 0;
        bool scroll = false;

        template <size_t N>
        void set(GFXcanvas16 &c, const String &t, const GFXfont *const (&fonts)[N], int areaH)
        {
            text = t;
            offset = 0;
            for (size_t i = 0; i < N; i++)
            {
                font = fonts[i];
                measure(c, areaH);
                if (w <= cfg::SCREEN_W)
                    break;
            }
            scroll = w > cfg::SCREEN_W;
        }

        void step() { offset = (offset + cfg::SCROLL_PX) % (w + cfg::SCROLL_GAP); }

        void draw(GFXcanvas16 &c) const
        {
            c.setFont(font);
            c.setTextSize(1);
            if (!scroll)
            {
                c.setCursor((cfg::SCREEN_W - w) / 2 - x1, baseline);
                c.print(text);
            }
            else
            { // two copies side by side for a seamless loop
                int x = -offset - x1;
                c.setCursor(x, baseline);
                c.print(text);
                c.setCursor(x + w + cfg::SCROLL_GAP, baseline);
                c.print(text);
            }
        }

    private:
        void measure(GFXcanvas16 &c, int areaH)
        {
            int16_t bx, by;
            uint16_t bw, bh;
            c.setFont(font);
            c.setTextSize(1);
            c.getTextBounds(text, 0, 0, &bx, &by, &bw, &bh);
            w = bw;
            x1 = bx;
            baseline = (areaH - (int)bh) / 2 - by; // vertically centred in the area
        }
    };

    void layout()
    {
        int priceArea = cfg::HALF_H - (view_.footer.length() ? cfg::FOOTER_H : 0);
        if (view_.compact)
        {
            nameLine_.set(*canvas_, view_.title, kStatusFonts, cfg::HALF_H);
            priceLine_.set(*canvas_, view_.price, kStatusFonts, priceArea);
        }
        else
        {
            nameLine_.set(*canvas_, view_.title, kNameFonts, cfg::HALF_H);
            priceLine_.set(*canvas_, view_.price, kPriceFonts, priceArea);
        }
    }

    void renderName()
    {
        canvas_->fillScreen(cfg::NAME_BG);
        canvas_->setTextColor(cfg::NAME_FG);
        nameLine_.draw(*canvas_);
        drawTagId(); // <- new
        push(0);
    }

    void renderPrice()
    {
        uint16_t bg = view_.onSale ? cfg::SALE_BG : cfg::PRICE_BG;
        uint16_t fg = view_.onSale ? cfg::SALE_FG : cfg::PRICE_FG;
        canvas_->fillScreen(bg);
        canvas_->setTextColor(fg);
        priceLine_.draw(*canvas_);
        if (view_.footer.length())
            drawFooter();
        push(cfg::HALF_H);
    }

    void drawFooter()
    {
        int16_t bx, by;
        uint16_t bw, bh;
        canvas_->setFont(&FreeSans9pt7b);
        canvas_->setTextSize(1);
        canvas_->getTextBounds(view_.footer, 0, 0, &bx, &by, &bw, &bh);
        canvas_->setCursor((cfg::SCREEN_W - bw) / 2 - bx,
                           cfg::HALF_H - cfg::FOOTER_H + (cfg::FOOTER_H - (int)bh) / 2 - by);
        canvas_->print(view_.footer);
    }

    void drawTagId()
    {
        if (!view_.tagId.length())
            return;
        int16_t bx, by;
        uint16_t bw, bh;
        canvas_->setFont(&FreeSans9pt7b);
        canvas_->setTextSize(1);
        canvas_->getTextBounds(view_.tagId, 0, 0, &bx, &by, &bw, &bh);
        canvas_->setTextColor(cfg::ID_FG);
        canvas_->setCursor(cfg::ID_MARGIN - bx, cfg::ID_MARGIN - by); // top-left, with margin
        canvas_->print(view_.tagId);
    }

    void push(int y)
    {
        tft_.drawRGBBitmap(0, y, canvas_->getBuffer(), cfg::SCREEN_W, cfg::HALF_H);
    }

    Adafruit_ST7789 tft_;
    GFXcanvas16 *canvas_ = nullptr;
    View view_;
    bool hasView_ = false;
    Line nameLine_, priceLine_;
    uint32_t lastScroll_ = 0;
};