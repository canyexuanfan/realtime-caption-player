// src/player/MpvPlayer.cpp
#include "MpvPlayer.h"

MpvPlayer::~MpvPlayer() {
    if (m_handle) {
        mpv_destroy(m_handle);
        m_handle = nullptr;
    }
}

unsigned long MpvPlayer::clientApiVersion() {
    return mpv_client_api_version();
}

bool MpvPlayer::create() {
    m_handle = mpv_create();
    return m_handle != nullptr;
}

int MpvPlayer::setOption(const QString& name, const QString& value) {
    if (!m_handle) return -1;
    const QByteArray n = name.toUtf8();
    const QByteArray v = value.toUtf8();
    return mpv_set_option_string(m_handle, n.constData(), v.constData());
}

int MpvPlayer::initialize() {
    if (!m_handle) return -1;
    return mpv_initialize(m_handle);
}

int MpvPlayer::loadFile(const QString& path, bool replace) {
    if (!m_handle) return -1;
    const QByteArray p = path.toUtf8();
    const char* args[] = {
        "loadfile",
        p.constData(),
        replace ? "replace" : "append",
        nullptr
    };
    return mpv_command(m_handle, args);
}

int MpvPlayer::createRenderContext(mpv_render_context** outCtx,
                                   void* (*getProcAddr)(void* ctx, const char* name),
                                   void* glCtx) {
    if (!m_handle) return -1;

    mpv_opengl_init_params glInit{};
    glInit.get_proc_address = getProcAddr;
    glInit.get_proc_address_ctx = glCtx;

    const char* apiType = MPV_RENDER_API_TYPE_OPENGL;
    mpv_render_param params[] = {
        {MPV_RENDER_PARAM_API_TYPE,
         const_cast<void*>(static_cast<const void*>(apiType))},
        {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &glInit},
        {MPV_RENDER_PARAM_INVALID, nullptr},
    };
    return mpv_render_context_create(outCtx, m_handle, params);
}
