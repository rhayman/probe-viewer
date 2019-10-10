#plugin build file
cmake_minimum_required(VERSION 3.5.0)

#include common rules
include(../PluginRules.cmake)

#add sources, not including OpenEphysLib.cpp
	add_sources(${PLUGIN_NAME}
	Source/ProbeViewerCanvas.cpp
	Source/ProbeViewerCanvas.h
	Source/ProbeViewerEditor.cpp
	Source/ProbeViewerEditor.h
	Source/ProbeViewerNode.cpp
	Source/ProbeViewerNode.h
	Source/ChannelViewCanvas/ChannelViewCanvas.cpp
	Source/ChannelViewCanvas/ChannelViewCanvas.hpp
	Source/ChannelViewCanvas/CanvasOptionsBar.cpp
	Source/ChannelViewCanvas/CanvasOptionsBar.hpp
	Source/kissfft/_kiss_fft_guts.h
	Source/kissfft/kiss_fft.c
	Source/kissfft/kiss_fft.h
	Source/kissfft/kiss_fftr.c
	Source/kissfft/kiss_fftr.h
	Source/NeuropixInterface/NeuropixInterface.cpp
	Source/NeuropixInterface/NeuropixInterface.hpp
	Source/TimeScale/ProbeViewerTimeScale.cpp
	Source/TimeScale/ProbeViewerTimeScale.hpp
	Source/Utilities/CircularBuffer.cpp
	Source/Utilities/CircularBuffer.hpp
	Source/Utilities/ColourScheme.cpp
	Source/Utilities/ColourScheme.hpp
	)
target_link_libraries(${PLUGIN_NAME}  ${OpenCV_LIBS})
#optional: create IDE groups
plugin_create_filters()