#include "VisualizerView.h"
#include "Thema.h"
#include "CrowPanelRender.h"
#include <algorithm>

bool VisualizerView::begin(lgfx::LGFX_Device &display)
{
    if (m_ready)
        return m_display == &display;
    m_display = &display;
    // Basisgeometrie. Die Zeichenfläche deckt genau den Basisbereich ab, sein Punkt (341,354) liegt bei (0,0)
    VisualizerGeometry radio;
    radio.scale = 1.0f;
    radio.originX = 0;
    radio.originY = 0;
    if (!m_radio.canvas.begin(RadioWidth, RadioHeight) || !m_radio.visualizer.init(m_radio.canvas, radio))
    {
        m_radio.visualizer.cleanup();
        m_radio.canvas.release();
        m_display = nullptr;
        m_active = nullptr;
        return false;
    }
    m_active = &m_radio;
    m_ready = true;
    m_force = true;
    return true;
}

bool VisualizerView::setBounds(int x, int y, int width, int height)
{
    Bank *bank = (width == RadioWidth && height == RadioHeight) ? &m_radio : nullptr;
    if (!m_ready || !bank || x < 0 || y < 0 || x > CrowPanelRender::PanelWidth - width || y > CrowPanelRender::PanelHeight - height)
        return false;
    if (bank != m_active || x != m_x || y != m_y)
    {
        m_active = bank;
        m_active->visualizer.resetSignal();
        m_x = x;
        m_y = y;
        m_tailWasGonio = false;
        invalidate();
    }
    return true;
}

void VisualizerView::invalidate()
{
    m_force = true;
}

void VisualizerView::update(uint32_t /*now*/, AudioManager &audio, bool silent)
{
    if (!m_ready || !m_active)
        return;
    auto &visualizer = m_active->visualizer;
    const auto status = audio.getStatus();
    const bool playing = !silent && status.state == AudioManager::State::Streaming && status.pcmAgeMs < 1000;
    visualizer.setPlaying(playing);
    if (audio.isAudioDataReady())
    {
        if (playing)
        {
            visualizer.setAudioValues(audio.getRMSLeft(), audio.getRMSRight(), audio.getPeakLeft(), audio.getPeakRight());
            visualizer.setSampleData(audio.getSampleBufferLeft(), audio.getSampleBufferRight(), audio.getValidSamplesCount(), audio.getSampleRate());
        }
        audio.consumeAudioData();
    }
    visualizer.draw(false);
    const auto damage = visualizer.takeCanvasDamage();
    // Teilflächen in Basiskoordinaten (800x480), über die Geometrie der Instanz auf ihre Zeichenfläche abgebildet:
    // oberer Streifen 341..800 x 354..425. Beim Goniometer nur bis 713, dazu der Fuß 341..505 x 425..480. Der Fuß endet mit dem
    // Goniometer-VU-Balken bei 505 (mapX 205), die Tastenreihe darunter beginnt erst bei 242 (Favoriten, Display 692).
    // Die Zwischenzeile 341..800 x 425..435 bis zur Tastenreihe zeichnet im Betrieb niemand, sie wird nach einer Invalidierung gesendet.
    auto &canvas = m_active->canvas;
    const bool gonio = mode() == VisMode::VIS_GONIO;
    const int left = visualizer.mapX(341), top = visualizer.mapY(354), stripBottom = visualizer.mapY(425), bottom = visualizer.mapY(480);
    const int gonioRight = visualizer.mapX(713), right = visualizer.mapX(800), tailRight = visualizer.mapX(505), gapBottom = visualizer.mapY(435);
    // Pro Ausgabezone nur eine Kopie und ein Cacheabgleich, auch bei mehreren geänderten Sprites.
    auto presentDamage = [&](int zoneLeft, int zoneTop, int zoneRight, int zoneBottom)
    {
        int dirtyLeft = zoneRight, dirtyTop = zoneBottom, dirtyRight = zoneLeft, dirtyBottom = zoneTop;
        if (m_force || damage.full)
        {
            dirtyLeft = zoneLeft;
            dirtyTop = zoneTop;
            dirtyRight = zoneRight;
            dirtyBottom = zoneBottom;
        }
        else
        {
            for (uint8_t i = 0; i < damage.count; ++i)
            {
                const auto &rect = damage.rects[i];
                const int x0 = std::max(zoneLeft, rect.x), y0 = std::max(zoneTop, rect.y);
                const int x1 = std::min(zoneRight, rect.x + rect.width), y1 = std::min(zoneBottom, rect.y + rect.height);
                if (x1 <= x0 || y1 <= y0)
                    continue;
                dirtyLeft = std::min(dirtyLeft, x0);
                dirtyTop = std::min(dirtyTop, y0);
                dirtyRight = std::max(dirtyRight, x1);
                dirtyBottom = std::max(dirtyBottom, y1);
            }
        }
        if (dirtyRight > dirtyLeft && dirtyBottom > dirtyTop)
            canvas.presentRegion(*m_display, dirtyLeft, dirtyTop, dirtyRight - dirtyLeft, dirtyBottom - dirtyTop,
                                 m_x + dirtyLeft - left, m_y + dirtyTop - top, m_force);
    };
    presentDamage(left, top, gonio ? gonioRight : right, stripBottom);
    // Die Zeichenfläche ist hier Grund, im Goniometermodus links der obere Teil des Fußes. Die Tastenreihe ab 435 bleibt unberührt.
    if (m_force)
        canvas.presentRegion(*m_display, left, stripBottom, right - left, gapBottom - stripBottom, m_x, m_y + stripBottom - top, true);
    if (gonio && m_force)
        canvas.presentRegion(*m_display, gonioRight, top, right - gonioRight, stripBottom - top, m_x + gonioRight - left, m_y, true);
    if (gonio)
        presentDamage(left, stripBottom, tailRight, bottom);
    if (m_tailWasGonio && !gonio)
    {
        canvas.fillRect(left, stripBottom, tailRight - left, bottom - stripBottom, farbe(Rolle::GemGrund));
        canvas.presentRegion(*m_display, left, stripBottom, tailRight - left, bottom - stripBottom, m_x, m_y + stripBottom - top, true);
    }
    m_tailWasGonio = gonio;
    m_force = false;
}

void VisualizerView::nextMode(int direction)
{
    if (!m_ready)
        return;
    const int next = (static_cast<int>(mode()) + direction % VIS_MODE_COUNT + VIS_MODE_COUNT) % VIS_MODE_COUNT;
    setMode(static_cast<VisMode>(next));
}

void VisualizerView::setMode(VisMode mode)
{
    if (!m_ready || static_cast<unsigned>(mode) >= VIS_MODE_COUNT)
        return;
    m_radio.visualizer.setMode(mode);
    invalidate();
}

void VisualizerView::nextStyle()
{
    if (!m_ready)
        return;
    m_radio.visualizer.nextStyle();
    invalidate();
}

void VisualizerView::setLanguage(LangIndex lang)
{
    m_radio.visualizer.setLanguage(lang);
    invalidate();
}

void VisualizerView::resetSignal()
{
    if (!m_ready)
        return;
    m_radio.visualizer.resetSignal();
    invalidate();
}

void VisualizerView::setVuSettings(uint8_t ballistics, uint8_t sensitivity)
{
    m_radio.visualizer.setVuSettings(ballistics, sensitivity);
    invalidate();
}

void VisualizerView::handleTouch(int x, int y, bool touched)
{
    if (!m_ready || !m_active)
        return;
    RadioPreferences before, after;
    m_active->visualizer.preferences(before);
    m_active->visualizer.handleTouch(x, y, touched);
    m_active->visualizer.preferences(after);
    if (before.mode != after.mode || memcmp(before.styles, after.styles, sizeof(before.styles)))
        invalidate();
}
