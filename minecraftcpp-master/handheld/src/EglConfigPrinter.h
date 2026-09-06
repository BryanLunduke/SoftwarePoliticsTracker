#ifndef EGLCONFIGPRINTER_H__
#define EGLCONFIGPRINTER_H__

#include <cstdio>
#include <string>
#include <EGL/egl.h>

class EGLConfigPrinter
{
public:
    static void printAllConfigs(EGLDisplay display) {

    	const EGLint attribs[] = {
			EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
			EGL_BLUE_SIZE,       5,
			EGL_GREEN_SIZE,      6,
			EGL_RED_SIZE,        5,
		    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT,
			EGL_NONE
    	};

        int num_config[1];
        eglChooseConfig(display, attribs, NULL, 0, num_config);

        int numConfigs = num_config[0];
        LOGI("Found %d configs\n", numConfigs);

        EGLConfig* configs = new EGLConfig[numConfigs];
        eglChooseConfig(display, attribs, configs, numConfigs, num_config);
        LOGI("Found %d configs (was %d))\n", num_config[0], numConfigs);

        for (int i = 0; i < numConfigs; ++i) {
            printConfig(display, configs[i]);
        }
        
        delete[] configs;
    }

    static void printConfig(EGLDisplay display, EGLConfig config) {
        int attributes[] = {
                EGL_BUFFER_SIZE,
                EGL_ALPHA_SIZE,
                EGL_BLUE_SIZE,
                EGL_GREEN_SIZE,
                EGL_RED_SIZE,
                EGL_DEPTH_SIZE,
                EGL_STENCIL_SIZE,
                EGL_CONFIG_CAVEAT,
                EGL_CONFIG_ID,
                EGL_LEVEL,
                EGL_MAX_PBUFFER_HEIGHT,
                EGL_MAX_PBUFFER_PIXELS,
                EGL_MAX_PBUFFER_WIDTH,
                EGL_NATIVE_RENDERABLE,
                EGL_NATIVE_VISUAL_ID,
                EGL_NATIVE_VISUAL_TYPE,
                  0x3030, // EGL10.EGL_PRESERVED_RESOURCES,
                EGL_SAMPLES,
                EGL_SAMPLE_BUFFERS,
                EGL_SURFACE_TYPE,
                EGL_TRANSPARENT_TYPE,
                EGL_TRANSPARENT_RED_VALUE,
                EGL_TRANSPARENT_GREEN_VALUE,
                EGL_TRANSPARENT_BLUE_VALUE,
                  0x3039, // EGL10.EGL_BIND_TO_TEXTURE_RGB,
                  0x303A, // EGL10.EGL_BIND_TO_TEXTURE_RGBA,
                  0x303B, // EGL10.EGL_MIN_SWAP_INTERVAL,
                  0x303C, // EGL10.EGL_MAX_SWAP_INTERVAL,
                EGL_LUMINANCE_SIZE,
                EGL_ALPHA_MASK_SIZE,
                EGL_COLOR_BUFFER_TYPE,
                EGL_RENDERABLE_TYPE,
                  0x3042 // EGL10.EGL_CONFORMANT
        };
        std::string names[] = {
                "EGL_BUFFER_SIZE",
                "EGL_ALPHA_SIZE",
                "EGL_BLUE_SIZE",
                "EGL_GREEN_SIZE",
                "EGL_RED_SIZE",
                "EGL_DEPTH_SIZE",
                "EGL_STENCIL_SIZE",
                "EGL_CONFIG_CAVEAT",
                "EGL_CONFIG_ID",
                "EGL_LEVEL",
                "EGL_MAX_PBUFFER_HEIGHT",
                "EGL_MAX_PBUFFER_PIXELS",
                "EGL_MAX_PBUFFER_WIDTH",
                "EGL_NATIVE_RENDERABLE",
                "EGL_NATIVE_VISUAL_ID",
                "EGL_NATIVE_VISUAL_TYPE",
                "EGL_PRESERVED_RESOURCES",
                "EGL_SAMPLES",
                "EGL_SAMPLE_BUFFERS",
                "EGL_SURFACE_TYPE",
                "EGL_TRANSPARENT_TYPE",
                "EGL_TRANSPARENT_RED_VALUE",
                "EGL_TRANSPARENT_GREEN_VALUE",
                "EGL_TRANSPARENT_BLUE_VALUE",
                "EGL_BIND_TO_TEXTURE_RGB",
                "EGL_BIND_TO_TEXTURE_RGBA",
                "EGL_MIN_SWAP_INTERVAL",
                "EGL_MAX_SWAP_INTERVAL",
                "EGL_LUMINANCE_SIZE",
                "EGL_ALPHA_MASK_SIZE",
                "EGL_COLOR_BUFFER_TYPE",
                "EGL_RENDERABLE_TYPE",
                "EGL_CONFORMANT"
        };
        int value[1];
        LOGI("\nPRINTCONFIG:");
        for (int i = 0; i < 33; i++) {
            int attribute = attributes[i];
            std::string name = names[i];
            if ( eglGetConfigAttrib(display, config, attribute, value)) {
                LOGI("  %s: %d\n", name.c_str(), value[0] );
            } else {
                // Log.w(TAG, String.format("  %s: failed\n", name));
                while (eglGetError() != EGL_SUCCESS);
            }
        }
    }
};

#endif /*EGLCONFIGPRINTER_H__*/