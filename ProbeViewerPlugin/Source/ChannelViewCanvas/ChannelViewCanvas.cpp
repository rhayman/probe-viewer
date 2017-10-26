/*
 ------------------------------------------------------------------
 
 This file is part of the Open Ephys GUI
 Copyright (C) 2017 Open Ephys
 
 ------------------------------------------------------------------
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 
 */

#include "ChannelViewCanvas.hpp"
#include "ProbeViewerCanvas.h"

#include "../NeuropixInterface/NeuropixInterface.hpp"
#include "../Utilities/ColourScheme.hpp"
#include "CanvasOptionsBar.hpp"

using namespace ProbeViewer;

#pragma mark - ChannelViewCanvas -

ChannelViewCanvas::ChannelViewCanvas(ProbeViewerCanvas* canvas)
: isDirty(false)
, numPixelUpdates(0)
, canvas(canvas)
, channelHeight(10)
, colourSchemeId(ColourSchemeId::INFERNO)
, screenBufferImage(Image::RGB, CHANNEL_DISPLAY_WIDTH, CHANNEL_DISPLAY_MAX_HEIGHT * 394, false)
, renderMode(RenderMode::RMS)
, frontBackBufferPixelOffset(CHANNEL_DISPLAY_TILE_WIDTH - 1)
{
    
    // get number of tiles that covers *at least* the display width
    int numTiles = ceil(ChannelViewCanvas::CHANNEL_DISPLAY_WIDTH / float(ChannelViewCanvas::CHANNEL_DISPLAY_TILE_WIDTH)) + 1;
    
    for (int i = 0; i < numTiles; ++i)
    {
        auto tile = new BitmapRenderTile(CHANNEL_DISPLAY_TILE_WIDTH, CHANNEL_DISPLAY_MAX_HEIGHT * 394);
        displayBitmapTiles.add(tile);
    }
}

ChannelViewCanvas::~ChannelViewCanvas()
{ }

void ChannelViewCanvas::paint(Graphics& g)
{
    renderTilesToScreenBufferImage();
    
    const float verticalScale = (getChannelHeight() * ChannelViewCanvas::NUM_READ_SITES_FOR_MAX_CHANNELS) / getFrontBufferPtr()->height;
    const float horizontalScale = getWidth() / float(ChannelViewCanvas::CHANNEL_DISPLAY_WIDTH);
    
    const auto transform = AffineTransform::scale(horizontalScale, verticalScale);
    
    g.drawImageTransformed(screenBufferImage, transform);
}

void ChannelViewCanvas::resized()
{
    repaint();
}

void ChannelViewCanvas::refresh()
{
    if (isDirty.get())
    {
        while (numPixelUpdates > 0)
        {
            // get the front buffer and create a BitmapData accessor object for it
            auto frontBufferPtr = getFrontBufferPtr();
            auto frontTile = frontBufferPtr->getTileForRenderMode(RenderMode::RMS);
            
            Image::BitmapData bdFrontBufferBitmap(*frontTile, 0, 0, frontBufferPtr->width, frontBufferPtr->height, Image::BitmapData::ReadWriteMode::writeOnly);
            
            // paint the pixel updates for each channel to the bitmap data
            for (auto channel : channels)
            {
                channel->pxPaint();
            }
            --numPixelUpdates;
            tick();
        }
        repaint(0, 0, getWidth(), getHeight());
        isDirty.set(false);
    }
}

void ChannelViewCanvas::renderTilesToScreenBufferImage()
{
    // TODO: (kelly) clean this up after deciding whether to use Graphics::drawImageAt or Graphics::drawImageTransformed
    Graphics gScreenBuffer(screenBufferImage);
    const auto tileImage = getFrontBufferPtr()->getTileForRenderMode(renderMode);
    
    // auto transform = AffineTransform::translation(-frontBackBufferPixelOffset - 1, 0.0f);
    float xPosition = -frontBackBufferPixelOffset - 1;
    
    gScreenBuffer.drawImageAt(*tileImage, xPosition, 0.0f);
    // gScreenBuffer.drawImageTransformed(*tileImage, transform);
    
    // get each backbuffer (all bitmap tiles before the last tile in the list, in descending order)
    for (int backBufferIdx = displayBitmapTiles.size() - 2;
         backBufferIdx >= 0;
         --backBufferIdx)
    {
        // move right by one tile width
        xPosition += CHANNEL_DISPLAY_TILE_WIDTH;
        // transform = transform.translated(ChannelViewCanvas::CHANNEL_DISPLAY_TILE_WIDTH, 0.0f);
        
        // draw the backbuffer
        // gScreenBuffer.drawImageTransformed(*displayBitmapTiles[backBufferIdx]->getTileForRenderMode(renderMode), transform);
        gScreenBuffer.drawImageAt(*displayBitmapTiles[backBufferIdx]->getTileForRenderMode(renderMode), xPosition, 0.0f);
        
    }
}

void ChannelViewCanvas::tick()
{
    if (--frontBackBufferPixelOffset < 0)
    {
        frontBackBufferPixelOffset = getFrontBufferPtr()->getTileForRenderMode(renderMode)->getWidth() - 1;
        
        auto prevFrontBuffer = displayBitmapTiles.getFirst();
        displayBitmapTiles.remove(0, false);
        displayBitmapTiles.add(prevFrontBuffer);
    }
}

void ChannelViewCanvas::setChannelHeight(float height)
{
    channelHeight = height;
    
    for (int readSiteId = 0; readSiteId < readSites.size(); ++readSiteId)
    {
        readSites[readSiteId]->setBounds(30.0f,
                   getHeight() - (readSiteId / 2 + 1) * (getChannelHeight() * 2) + (readSiteId % 2 == 0 ? getChannelHeight() : 0),
                   getWidth() - 30.0f,
                   getChannelHeight());
    }
    repaint();
}

float ChannelViewCanvas::getChannelHeight()
{
    return channelHeight;
}

void ChannelViewCanvas::pushPixelValueForChannel(int channel, float rms, float spikeRate, float fft)
{
    channels[channel]->pushSamples(rms, spikeRate, fft);
}

void ChannelViewCanvas::setDisplayedSubprocessor(int subProcessorIdx)
{
    drawableSubprocessorIdx = subProcessorIdx;
}

BitmapRenderTile* const ChannelViewCanvas::getFrontBufferPtr() const
{
    return displayBitmapTiles[displayBitmapTiles.size() - 1];
}

int ChannelViewCanvas::getBufferOffsetPosition() const
{
    return frontBackBufferPixelOffset;
}

RenderMode ChannelViewCanvas::getCurrentRenderMode() const
{
    return renderMode;
}

void ChannelViewCanvas::setCurrentRenderMode(RenderMode r)
{
    renderMode = r;
    repaint();
}

ColourSchemeId ChannelViewCanvas::getCurrentColourScheme() const
{
    return colourSchemeId;
}

void ChannelViewCanvas::setCurrentColourScheme(ColourSchemeId schemeId)
{
    colourSchemeId = schemeId;
}




# pragma mark - ChannelViewCanvas Constants

const int ChannelViewCanvas::NUM_READ_SITES_FOR_MAX_CHANNELS = 394;
const int ChannelViewCanvas::CHANNEL_DISPLAY_MAX_HEIGHT = 2; // more efficient than scaling down, but slightly lossy in definition (cross channel bleed)
const int ChannelViewCanvas::CHANNEL_DISPLAY_WIDTH = 1920;
const int ChannelViewCanvas::CHANNEL_DISPLAY_TILE_WIDTH = 64;
const Colour ChannelViewCanvas::backgroundColour(0, 18, 43);




# pragma mark - BitmapRenderTile -

BitmapRenderTile::BitmapRenderTile(int width, int height)
: width(width)
, height(height)
{
    rms = new Image(Image::RGB, width, height, false);
    spikeRate = new Image(Image::RGB, width, height, false);
    fft = new Image(Image::RGB, width, height, false);
    
    const int subImageHeight = height / ChannelViewCanvas::NUM_READ_SITES_FOR_MAX_CHANNELS;
    
    for (auto img : {rms.get(), spikeRate.get(), fft.get()})
    {
        Graphics g(*img);
        g.setColour(Colours::black);
        g.fillRect(0, 0, width, height);
        
        readSiteSubImage.add(new Array<Image>());
        for (int readSite = 0; readSite < ChannelViewCanvas::NUM_READ_SITES_FOR_MAX_CHANNELS; ++readSite)
        {
            auto subImage = img->getClippedImage(Rectangle<int>(0, height - (readSite + 1) * subImageHeight, width, subImageHeight));
            readSiteSubImage.getLast()->add(subImage);
        }
    }
    
}

Image* const BitmapRenderTile::getTileForRenderMode(RenderMode mode)
{
    if (mode == RenderMode::RMS) return rms;
    else if (mode == RenderMode::FFT) return fft;
    
    return spikeRate;
}

Image& BitmapRenderTile::getReadSiteSubImageForRenderMode(int readSite, RenderMode mode)
{
    int modeIdx = 0;
    if (mode == RenderMode::RMS)
        modeIdx = 0;
    else if (mode == RenderMode::SPIKE_RATE)
        modeIdx = 1;
    else // if mode is FFT
        modeIdx = 2;
    
    return readSiteSubImage[modeIdx]->getReference(readSite);
}




# pragma mark - ProbeChannelDisplay -

ProbeChannelDisplay::ProbeChannelDisplay(ChannelViewCanvas* channelsView, CanvasOptionsBar* optionsBar, ChannelState status, int channelID, int readSiteID, float sampleRate)
: channelsView(channelsView)
, optionsBar(optionsBar)
, sampleRate(sampleRate)
, samplesPerPixel(0)
, channelState(status)
, channelID(channelID)
, readSiteID(readSiteID)
{
    yBitmapPos = (ChannelViewCanvas::NUM_READ_SITES_FOR_MAX_CHANNELS * ChannelViewCanvas::CHANNEL_DISPLAY_MAX_HEIGHT) - (readSiteID + 1) * ChannelViewCanvas::CHANNEL_DISPLAY_MAX_HEIGHT;
    
    if (channelState != ChannelState::reference)
        samplesPerPixel = sampleRate * ProbeViewerCanvas::TRANSPORT_WINDOW_TIMEBASE / float(ChannelViewCanvas::CHANNEL_DISPLAY_WIDTH);
}

ProbeChannelDisplay::~ProbeChannelDisplay()
{ }

void ProbeChannelDisplay::paint(Graphics& g)
{ }

//void ProbeChannelDisplay::pxPaint(Image::BitmapData *bitmapData)
void ProbeChannelDisplay::pxPaint()
{
    // render RMS
    {
        Image::BitmapData bdSubImage(channelsView->getFrontBufferPtr()->getReadSiteSubImageForRenderMode(readSiteID, RenderMode::RMS), Image::BitmapData::ReadWriteMode::writeOnly);
        
        float boundSpread = optionsBar->getRMSBoundSpread();
        if (boundSpread == 0) boundSpread = 1;
        const auto val = (rms[rms.size() - channelsView->numPixelUpdates] - optionsBar->getRMSLowBound()) / boundSpread;
        
        Colour colour = ColourScheme::getColourForNormalizedValueInScheme(val, channelsView->getCurrentColourScheme());
        
        const int xPix = channelsView->getBufferOffsetPosition();
        for (int yPix = 0; yPix < bdSubImage.height; ++yPix)
        {
            bdSubImage.setPixelColour(xPix, yPix, colour);
        }
    }
    
    // render SPIKE_RATE
    {
        Image::BitmapData bdSubImage(channelsView->getFrontBufferPtr()->getReadSiteSubImageForRenderMode(readSiteID, RenderMode::SPIKE_RATE), Image::BitmapData::ReadWriteMode::writeOnly);
        
        float boundSpread = optionsBar->getSpikeRateBoundSpread();
        if (boundSpread == 0) boundSpread = 1;
        const auto val = (spikeRate[spikeRate.size() - channelsView->numPixelUpdates] - optionsBar->getSpikeRateLowBound()) / boundSpread;
        
        Colour colour = ColourScheme::getColourForNormalizedValueInScheme(val, channelsView->getCurrentColourScheme());
        
        const int xPix = channelsView->getBufferOffsetPosition();
        for (int yPix = 0; yPix < bdSubImage.height; ++yPix)
        {
            bdSubImage.setPixelColour(xPix, yPix, colour);
        }
    }
    
    // render FFT
    {
        Image::BitmapData bdSubImage(channelsView->getFrontBufferPtr()->getReadSiteSubImageForRenderMode(readSiteID, RenderMode::FFT), Image::BitmapData::ReadWriteMode::writeOnly);
        
        float boundSpread = optionsBar->getFFTBoundSpread();
        if (boundSpread == 0) boundSpread = 1;
        const auto val = (fft[fft.size() - channelsView->numPixelUpdates] - optionsBar->getFFTLowBound()) / boundSpread;
        
        Colour colour = ColourScheme::getColourForNormalizedValueInScheme(val, channelsView->getCurrentColourScheme());
        
        const int xPix = channelsView->getBufferOffsetPosition();
        for (int yPix = 0; yPix < bdSubImage.height; ++yPix)
        {
            bdSubImage.setPixelColour(xPix, yPix, colour);
        }
    }
    
    if (channelsView->numPixelUpdates == 1)
    {
        rms.clear();
        spikeRate.clear();
        fft.clear();
    }
}

ChannelState ProbeChannelDisplay::getChannelState() const
{
    return channelState;
}

void ProbeChannelDisplay::setChannelState(ChannelState status)
{
    channelState = status;
}

void ProbeChannelDisplay::pushSamples(float rms, float spikeRate, float fft)
{
    // TODO: (kelly) @TESTING clean this up
    if (this->rms.size() > 1024)
    {
        std::cout << "rms size is growing uncontrollably (" << this->rms.size() << ")" << std::endl;
    }
    
    // TODO: (kelly) @TESTING clean this up
    if (this->spikeRate.size() > 1024)
    {
        std::cout << "spikeRate size is growing uncontrollably (" << this->spikeRate.size() << ")" << std::endl;
    }
    
    // TODO: (kelly) @TESTING clean this up
    if (this->fft.size() > 1024)
    {
        std::cout << "fft size is growing uncontrollably (" << this->fft.size() << ")" << std::endl;
    }
    
    this->rms.add(rms);
    this->spikeRate.add(spikeRate);
    this->fft.add(fft);
}

int ProbeChannelDisplay::getChannelId() const
{
    return channelID;
}

void ProbeChannelDisplay::setChannelId(int id)
{
    channelID = id;
}

int ProbeChannelDisplay::getReadSiteId() const
{
    return readSiteID;
}

void ProbeChannelDisplay::setReadSiteId(int id)
{
    readSiteID = id;
}

float ProbeChannelDisplay::getSampleRate()
{
    return sampleRate;
}

int ProbeChannelDisplay::getNumSamplesPerPixel()
{
    return samplesPerPixel;
}
