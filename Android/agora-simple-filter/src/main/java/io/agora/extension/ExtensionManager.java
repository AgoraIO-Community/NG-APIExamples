package io.agora.extension;

import androidx.annotation.Keep;

@Keep
public class ExtensionManager {
    public static final String EXTENSION_NAME = "agora-simple-filter"; // Name of target link library used in CMakeLists.txt
    public static final String EXTENSION_VENDOR_NAME = "Agora"; // Provider name used for registering in agora-bytedance.cpp
    public static final String EXTENSION_VIDEO_FILTER_NAME = "Watermark"; // Video filter name defined in ExtensionProvider.h
    public static final String EXTENSION_AUDIO_FILTER_NAME = "VolumeChange"; // Audio filter name defined in ExtensionProvider.h

    public static final String KEY_ENABLE_WATER_MARK = "wmEffectEnabled";
    public static final String KEY_ADJUST_VOLUME_CHANGE = "volume";
}