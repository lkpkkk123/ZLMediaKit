/*
 * Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include "mk_media.h"
#include "Util/logger.h"
#include "Common/Device.h"
#if HAVE_GIT_VER
#include "git_ver.h"
#endif
using namespace std;
using namespace toolkit;
using namespace mediakit;

class MediaHelper: public MediaSourceEvent, public std::enable_shared_from_this<MediaHelper> {
public:
    using Ptr = std::shared_ptr<MediaHelper>;
    MediaHelper(const char *vhost, const char *app, const char *stream, float duration, const ProtocolOption &option) {
        _poller = EventPollerPool::Instance().getPoller();
        // 在poller线程中创建DevChannel(MultiMediaSourceMuxer)对象，确保严格的线程安全限制  [AUTO-TRANSLATED:d5063d7a]
        // Create a DevChannel (MultiMediaSourceMuxer) object in the poller thread to ensure strict thread safety restrictions
        auto tuple = MediaTuple{vhost, app, stream};
        _poller->sync([&]() { _channel = std::make_shared<DevChannel>(tuple, duration, option); });
    }

    ~MediaHelper() = default;

    void attachEvent() { _channel->setMediaListener(shared_from_this()); }

    DevChannel::Ptr &getChannel() { return _channel; }

    void setOnClose(on_mk_media_close cb, std::shared_ptr<void> user_data) {
        _on_close = cb;
        _on_close_data = std::move(user_data);
    }

    void setOnSeek(on_mk_media_seek cb, std::shared_ptr<void> user_data) {
        _on_seek = cb;
        _on_seek_data = std::move(user_data);
    }

    void setOnPause(on_mk_media_pause cb, std::shared_ptr<void> user_data) {
        _on_pause = cb;
        _on_pause_data = std::move(user_data);
    }

    void setOnSpeed(on_mk_media_speed cb, std::shared_ptr<void> user_data) {
        _on_speed = cb;
        _on_speed_data = std::move(user_data);
    }

    void setOnRegist(on_mk_media_source_regist cb, std::shared_ptr<void> user_data) {
        _on_regist = cb;
        _on_regist_data = std::move(user_data);
    }

protected:
    // 通知其停止推流  [AUTO-TRANSLATED:d69d10d8]
    // Notify it to stop streaming
    bool close(MediaSource &sender) override {
        if (!_on_close) {
            // 未设置回调，没法关闭  [AUTO-TRANSLATED:2c1423fe]
            // No callback is set, so it cannot be closed
            WarnL << "请使用mk_media_set_on_close函数设置回调函数!";
            return false;
        }
        // 请在回调中调用mk_media_release函数释放资源,否则MediaSource::close()操作不会生效  [AUTO-TRANSLATED:da067eb0]
        // Please call the mk_media_release function to release resources in the callback, otherwise the MediaSource::close() operation will not take effect
        _on_close(_on_close_data.get());
        WarnL << "close media: " << sender.getUrl();
        return true;
    }

    bool seekTo(MediaSource &sender, uint32_t stamp) override {
        if (!_on_seek) {
            return false;
        }
        return _on_seek(_on_seek_data.get(), stamp);
    }

    // 通知暂停或恢复  [AUTO-TRANSLATED:ee3c219f]
    // Notify pause or resume
    bool pause(MediaSource &sender, bool pause) override {
        if (!_on_pause) {
            return false;
        }
        return _on_pause(_on_pause_data.get(), pause);
    }

    // 通知倍数播放  [AUTO-TRANSLATED:12e66e3f]
    // Notify playback speed
    bool speed(MediaSource &sender, float speed) override {
        if (!_on_speed) {
            return false;
        }
        return _on_speed(_on_speed_data.get(), speed);
    }

    void onRegist(MediaSource &sender, bool regist) override {
        if (_on_regist) {
            _on_regist(_on_regist_data.get(), (mk_media_source)&sender, regist);
        }
    }

    toolkit::EventPoller::Ptr getOwnerPoller(MediaSource &sender) override { return _poller; }

private:
    EventPoller::Ptr _poller;
    DevChannel::Ptr _channel;
    on_mk_media_close _on_close = nullptr;
    on_mk_media_seek _on_seek = nullptr;
    on_mk_media_pause _on_pause = nullptr;
    on_mk_media_speed _on_speed = nullptr;
    on_mk_media_source_regist _on_regist = nullptr;
    std::shared_ptr<void> _on_seek_data;
    std::shared_ptr<void> _on_pause_data;
    std::shared_ptr<void> _on_speed_data;
    std::shared_ptr<void> _on_close_data;
    std::shared_ptr<void> _on_regist_data;
};

API_EXPORT void API_CALL mk_media_set_on_close(mk_media ctx, on_mk_media_close cb, void *user_data) {
    mk_media_set_on_close2(ctx, cb, user_data, nullptr);
}

API_EXPORT void API_CALL mk_media_set_on_close2(mk_media ctx, on_mk_media_close cb, void *user_data, on_user_data_free user_data_free) {
    assert(ctx);
    MediaHelper::Ptr *obj = (MediaHelper::Ptr *) ctx;
    std::shared_ptr<void> ptr(user_data, user_data_free ? user_data_free : [](void *) {});
    (*obj)->setOnClose(cb, std::move(ptr));
}

API_EXPORT void API_CALL mk_media_set_on_seek(mk_media ctx, on_mk_media_seek cb, void *user_data) {
    mk_media_set_on_seek2(ctx, cb, user_data, nullptr);
}

API_EXPORT void API_CALL mk_media_set_on_seek2(mk_media ctx, on_mk_media_seek cb, void *user_data, on_user_data_free user_data_free) {
    assert(ctx);
    MediaHelper::Ptr *obj = (MediaHelper::Ptr *) ctx;
    std::shared_ptr<void> ptr(user_data, user_data_free ? user_data_free : [](void *) {});
    (*obj)->setOnSeek(cb, std::move(ptr));
}

API_EXPORT void API_CALL mk_media_set_on_pause(mk_media ctx, on_mk_media_pause cb, void *user_data) {
    mk_media_set_on_pause2(ctx, cb, user_data, nullptr);
}

API_EXPORT void API_CALL mk_media_set_on_pause2(mk_media ctx, on_mk_media_pause cb, void *user_data, on_user_data_free user_data_free) {
    assert(ctx);
    MediaHelper::Ptr *obj = (MediaHelper::Ptr *) ctx;
    std::shared_ptr<void> ptr(user_data, user_data_free ? user_data_free : [](void *) {});
    (*obj)->setOnPause(cb, std::move(ptr));
}

API_EXPORT void API_CALL mk_media_set_on_speed(mk_media ctx, on_mk_media_speed cb, void *user_data) {
    mk_media_set_on_speed2(ctx, cb, user_data, nullptr);
}

API_EXPORT void API_CALL mk_media_set_on_speed2(mk_media ctx, on_mk_media_speed cb, void *user_data, on_user_data_free user_data_free) {
    assert(ctx);
    MediaHelper::Ptr *obj = (MediaHelper::Ptr *) ctx;
    std::shared_ptr<void> ptr(user_data, user_data_free ? user_data_free : [](void *) {});
    (*obj)->setOnSpeed(cb, std::move(ptr));
}

API_EXPORT void API_CALL mk_media_set_on_regist(mk_media ctx, on_mk_media_source_regist cb, void *user_data) {
    mk_media_set_on_regist2(ctx, cb, user_data, nullptr);
}

API_EXPORT void API_CALL mk_media_set_on_regist2(mk_media ctx, on_mk_media_source_regist cb, void *user_data, on_user_data_free user_data_free) {
    assert(ctx);
    MediaHelper::Ptr *obj = (MediaHelper::Ptr *) ctx;
    std::shared_ptr<void> ptr(user_data, user_data_free ? user_data_free : [](void *) {});
    (*obj)->setOnRegist(cb, std::move(ptr));
}

API_EXPORT int API_CALL mk_media_total_reader_count(mk_media ctx) {
    assert(ctx);
    MediaHelper::Ptr *obj = (MediaHelper::Ptr *) ctx;
    return (*obj)->getChannel()->totalReaderCount();
}

API_EXPORT mk_media API_CALL mk_media_create(const char *vhost, const char *app, const char *stream,
                                             float duration, int hls_enabled, int mp4_enabled) {
    assert(vhost && app && stream);
    ProtocolOption option;
    option.enable_hls = hls_enabled;
    option.enable_mp4 = mp4_enabled;

    MediaHelper::Ptr *obj(new MediaHelper::Ptr(new MediaHelper(vhost, app, stream, duration, option)));
    (*obj)->attachEvent();
    return (mk_media) obj;
}

API_EXPORT mk_media API_CALL mk_media_create2(const char *vhost, const char *app, const char *stream, float duration, mk_ini ini) {
    assert(vhost && app && stream && ini);
    ProtocolOption option(*((mINI *)ini));
    MediaHelper::Ptr *obj(new MediaHelper::Ptr(new MediaHelper(vhost, app, stream, duration, option)));
    (*obj)->attachEvent();
    return (mk_media) obj;
}

API_EXPORT void API_CALL mk_media_release(mk_media ctx) {
    assert(ctx);
    MediaHelper::Ptr *obj = (MediaHelper::Ptr *) ctx;
    delete obj;
}

API_EXPORT int API_CALL mk_media_init_video(mk_media ctx, int codec_id, int width, int height, float fps, int bit_rate) {
    assert(ctx);
    MediaHelper::Ptr *obj = (MediaHelper::Ptr *) ctx;
    VideoInfo info;
    info.codecId = (CodecId)codec_id;
    info.iFrameRate = fps;
    info.iWidth = width;
    info.iHeight = height;
    info.iBitRate = bit_rate;
    return (*obj)->getChannel()->initVideo(info);
}

API_EXPORT int API_CALL mk_media_init_audio(mk_media ctx, int codec_id, int sample_rate, int channels, int sample_bit) {
    assert(ctx);
    MediaHelper::Ptr *obj = (MediaHelper::Ptr *) ctx;
    AudioInfo info;
    info.codecId = (CodecId)codec_id;
    info.iSampleRate = sample_rate;
    info.iChannel = channels;
    info.iSampleBit = sample_bit;
    return (*obj)->getChannel()->initAudio(info);
}

API_EXPORT void API_CALL mk_media_init_track(mk_media ctx, mk_track track) {
    assert(ctx && track);
    MediaHelper::Ptr *obj = (MediaHelper::Ptr *) ctx;
    (*obj)->getChannel()->addTrack(*((Track::Ptr *) track));
}

API_EXPORT void API_CALL mk_media_init_complete(mk_media ctx) {
    assert(ctx);
    MediaHelper::Ptr *obj = (MediaHelper::Ptr *) ctx;
    (*obj)->getChannel()->addTrackCompleted();
}

API_EXPORT int API_CALL mk_media_input_frame(mk_media ctx, mk_frame frame) {
    assert(ctx && frame);
    MediaHelper::Ptr *obj = (MediaHelper::Ptr *) ctx;
    return (*obj)->getChannel()->inputFrame(*((Frame::Ptr *) frame));
}

API_EXPORT int API_CALL mk_media_input_h264(mk_media ctx, const void *data, int len, uint64_t dts, uint64_t pts) {
    assert(ctx && data && len > 0);
    MediaHelper::Ptr *obj = (MediaHelper::Ptr *) ctx;
    return (*obj)->getChannel()->inputH264((const char *) data, len, dts, pts);
}

API_EXPORT int API_CALL mk_media_input_h265(mk_media ctx, const void *data, int len, uint64_t dts, uint64_t pts) {
    assert(ctx && data && len > 0);
    MediaHelper::Ptr *obj = (MediaHelper::Ptr *) ctx;
    return (*obj)->getChannel()->inputH265((const char *) data, len, dts, pts);
}

API_EXPORT void API_CALL mk_media_input_yuv(mk_media ctx, const char *yuv[3], int linesize[3], uint64_t cts) {
    assert(ctx && yuv && linesize);
    MediaHelper::Ptr *obj = (MediaHelper::Ptr *) ctx;
    (*obj)->getChannel()->inputYUV((char **) yuv, linesize, cts);
}

API_EXPORT int API_CALL mk_media_input_aac(mk_media ctx, const void *data, int len, uint64_t dts, void *adts) {
    assert(ctx && data && len > 0);
    MediaHelper::Ptr *obj = (MediaHelper::Ptr *) ctx;
    return (*obj)->getChannel()->inputAAC((const char *) data, len, dts, (char *) adts);
}

API_EXPORT int API_CALL mk_media_input_pcm(mk_media ctx, void *data, int len, uint64_t pts) {
    assert(ctx && data && len > 0);
    MediaHelper::Ptr* obj = (MediaHelper::Ptr*) ctx;
    return (*obj)->getChannel()->inputPCM((char*)data, len, pts);
}

API_EXPORT int API_CALL mk_media_input_audio(mk_media ctx, const void *data, int len, uint64_t dts) {
    assert(ctx && data && len > 0);
    MediaHelper::Ptr* obj = (MediaHelper::Ptr*) ctx;
    return (*obj)->getChannel()->inputAudio((const char*)data, len, dts);
}

API_EXPORT void API_CALL mk_media_start_send_rtp(mk_media ctx, const char *dst_url, uint16_t dst_port, const char *ssrc, int con_type, on_mk_media_send_rtp_result cb, void *user_data) {
    mk_media_start_send_rtp2(ctx, dst_url, dst_port, ssrc, con_type, cb, user_data, nullptr);
}

API_EXPORT void API_CALL mk_media_start_send_rtp2(mk_media ctx, const char *dst_url, uint16_t dst_port, const char *ssrc, int con_type, on_mk_media_send_rtp_result cb, void *user_data,
    on_user_data_free user_data_free) {
    assert(ctx && dst_url && ssrc);
    MediaHelper::Ptr* obj = (MediaHelper::Ptr*) ctx;

    MediaSourceEvent::SendRtpArgs args;
    args.dst_url = dst_url;
    args.dst_port = dst_port;
    args.ssrc = ssrc;
    args.close_delay_ms = 30 * 1000;
    args.con_type = (mediakit::MediaSourceEvent::SendRtpArgs::ConType)con_type;

    // sender参数无用  [AUTO-TRANSLATED:21590ae5]
    // The sender parameter is useless
    auto ref = *obj;
    std::shared_ptr<void> ptr(user_data, user_data_free ? user_data_free : [](void *) {});
    (*obj)->getChannel()->getOwnerPoller(MediaSource::NullMediaSource())->async([args, ref, cb, ptr]() {
        ref->getChannel()->startSendRtp(MediaSource::NullMediaSource(), args, [cb, ptr](uint16_t local_port, const SockException &ex) {
            if (cb) {
                cb(ptr.get(), local_port, ex.getErrCode(), ex.what());
            }
        });
    });
}

API_EXPORT void API_CALL mk_media_start_send_rtp3(mk_media ctx, const char *dst_url, uint16_t dst_port, const char *ssrc, int con_type, mk_ini options, on_mk_media_send_rtp_result cb, void *user_data) {
     mk_media_start_send_rtp4(ctx, dst_url, dst_port, ssrc, con_type,options, cb, user_data, nullptr);
}

API_EXPORT void API_CALL mk_media_start_send_rtp4(mk_media ctx, const char *dst_url, uint16_t dst_port, const char *ssrc, int con_type, mk_ini options, on_mk_media_send_rtp_result cb, void *user_data,on_user_data_free user_data_free) {
    assert(ctx && dst_url && ssrc);
    MediaHelper::Ptr *obj = (MediaHelper::Ptr *)ctx;
    MediaSourceEvent::SendRtpArgs args;
    args.dst_url = dst_url;
    args.dst_port = dst_port;
    args.ssrc = ssrc;
    args.con_type = (mediakit::MediaSourceEvent::SendRtpArgs::ConType)con_type;
    auto ini_ptr = (mINI *)options;
    args.src_port = (*ini_ptr)["src_port"].empty() ? 0 : (*ini_ptr)["src_port"].as<int>();
    args.ssrc_multi_send = (*ini_ptr)["ssrc_multi_send"].empty() ? false : (*ini_ptr)["ssrc_multi_send"].as<bool>();
    args.pt = (*ini_ptr)["pt"].empty() ? 96 : (*ini_ptr)["pt"].as<int>();
    args.data_type = (*ini_ptr)["data_type"].empty() ? MediaSourceEvent::SendRtpArgs::DataType::kRtpPS
                                                     : (MediaSourceEvent::SendRtpArgs::DataType)(*ini_ptr)["data_type"].as<int>();
    args.only_audio = (*ini_ptr)["only_audio"].empty() ? false : (*ini_ptr)["only_audio"].as<bool>();
    args.udp_rtcp_timeout = (*ini_ptr)["udp_rtcp_timeout"].empty() ? false : (*ini_ptr)["udp_rtcp_timeout"].as<bool>();
    args.recv_stream_id =(*ini_ptr)["recv_stream_id"];
    args.recv_stream_app =obj->get()->getChannel()->getMediaTuple().app.c_str();
    args.recv_stream_vhost = obj->get()->getChannel()->getMediaTuple().vhost.c_str();
    args.close_delay_ms = (*ini_ptr)["close_delay_ms"].empty() ? 30000 : (*ini_ptr)["close_delay_ms"].as<int>();
    args.rtcp_timeout_ms = (*ini_ptr)["rtcp_timeout_ms"].empty() ? 30000 : (*ini_ptr)["rtcp_timeout_ms"].as<int>();
    args.rtcp_send_interval_ms = (*ini_ptr)["rtcp_send_interval_ms"].empty() ? 5000 : (*ini_ptr)["rtcp_send_interval_ms"].as<int>();
    // sender参数无用  [AUTO-TRANSLATED:21590ae5]
    // The sender parameter is useless
    auto ref = *obj;
    std::shared_ptr<void> ptr(
        user_data, user_data_free ? user_data_free : [](void *) {});
    (*obj)->getChannel()->getOwnerPoller(MediaSource::NullMediaSource())->async([args, ref, cb, ptr]() {
        ref->getChannel()->startSendRtp(MediaSource::NullMediaSource(), args, [cb, ptr](uint16_t local_port, const SockException &ex) {
            if (cb) {
                cb(ptr.get(), local_port, ex.getErrCode(), ex.what());
            }
        });
    });
}

API_EXPORT void API_CALL mk_media_stop_send_rtp(mk_media ctx, const char *ssrc) {
    assert(ctx);
    MediaHelper::Ptr *obj = (MediaHelper::Ptr *)ctx;
    // sender参数无用  [AUTO-TRANSLATED:21590ae5]
    // The sender parameter is useless
    auto ref = *obj;
    string ssrc_str = ssrc ? ssrc : "";
    (*obj)->getChannel()->getOwnerPoller(MediaSource::NullMediaSource())->async([ref, ssrc_str]() {
        ref->getChannel()->stopSendRtp(MediaSource::NullMediaSource(), ssrc_str);
    });
}

API_EXPORT mk_thread API_CALL mk_media_get_owner_thread(mk_media ctx) {
    MediaHelper::Ptr *obj = (MediaHelper::Ptr *)ctx;
    return (mk_thread)(*obj)->getChannel()->getOwnerPoller(MediaSource::NullMediaSource()).get();
}

// #include "mk_mediakit.h"
#include "mk_events.h"
#include "mk_server.h"
#include <string.h>
#include <string>

using namespace std;

void strcpySe2__(char *pDest, const char *pSrc, int nMaxDestLen) {
    pDest[0] = 0; // 防止字符串为空的时候出问题
    strncpy(pDest, pSrc, nMaxDestLen);
    pDest[nMaxDestLen - 1] = 0;
}
#define strcpySe3(d, s) strcpySe2__(d, s, sizeof(d))

uint64_t GetTickCount_tt() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts); // 开机到现在的时间
    return (ts.tv_sec * 1000 + ts.tv_nsec / (1000 * 1000));
}

string g_strUser;
string g_strPass;

void API_CALL on_mk_media_play(const mk_media_info url_info, const mk_auth_invoker invoker, const mk_sock_info sender) {

    char ip[64];
    printf(
        "rtsp play info, local: %s:%d, peer: %s:%d\n"
        "%s/%s/%s/%s, url params: %s\n",
        mk_sock_info_local_ip(sender, ip), mk_sock_info_local_port(sender), mk_sock_info_peer_ip(sender, ip + 32), mk_sock_info_peer_port(sender),
        mk_media_info_get_schema(url_info), mk_media_info_get_vhost(url_info), mk_media_info_get_app(url_info), mk_media_info_get_stream(url_info),
        mk_media_info_get_params(url_info));

    mk_auth_invoker_do(invoker, nullptr);

    if (0) {
        // 验证权限
        // MediaAuthInfo authParam;
        // mk_sock_info_peer_ip(sender, authParam.peerIp);
        // strcpySe2(authParam.schema, mk_media_info_get_schema(url_info));
        // strcpySe2(authParam.streamName, mk_media_info_get_stream(url_info));
        // strcpySe2(authParam.app, mk_media_info_get_app(url_info));
        // strcpySe2(authParam.urlParam, mk_media_info_get_params(url_info));
        // 允许播放
        bool bAuthSucc = true;
        if (bAuthSucc) {
            mk_auth_invoker_do(invoker, nullptr);
        } else {
            mk_auth_invoker_do(invoker, "user or pass err");
        }
    }
}

void API_CALL on_mk_media_no_reader(const mk_media_source sender) {
    printf(
        "no reader %s/%s/%s/%s\n", mk_media_source_get_schema(sender), mk_media_source_get_vhost(sender), mk_media_source_get_app(sender),
        mk_media_source_get_stream(sender));
}

void on_mk_rtsp_get_realm(const mk_media_info url_info, const mk_rtsp_get_realm_invoker invoker, const mk_sock_info sender) {

    char ip[64];
    printf(
        "client info, local: %s:%d, peer: %s:%d\n"
        "%s/%s/%s/%s, url params: %s",
        mk_sock_info_local_ip(sender, ip), mk_sock_info_local_port(sender), mk_sock_info_peer_ip(sender, ip + 32), mk_sock_info_peer_port(sender),
        mk_media_info_get_schema(url_info), mk_media_info_get_vhost(url_info), mk_media_info_get_app(url_info), mk_media_info_get_stream(url_info),
        mk_media_info_get_params(url_info));

    // rtsp播放默认鉴权
    mk_rtsp_get_realm_invoker_do(invoker, "mk_server");
}

void API_CALL on_mk_rtsp_auth(
    const mk_media_info url_info, const char *realm, const char *user_name, int must_no_encrypt, const mk_rtsp_auth_invoker invoker,
    const mk_sock_info sender) {

    char ip[64];
    printf(
        "client info, local: %s:%d, peer: %s:%d\n"
        "%s/%s/%s/%s, url params: %s\n"
        "realm: %s, user_name: %s, must_no_encrypt: %d",
        mk_sock_info_local_ip(sender, ip), mk_sock_info_local_port(sender), mk_sock_info_peer_ip(sender, ip + 32), mk_sock_info_peer_port(sender),
        mk_media_info_get_schema(url_info), mk_media_info_get_vhost(url_info), mk_media_info_get_app(url_info), mk_media_info_get_stream(url_info),
        mk_media_info_get_params(url_info), realm, user_name, (int)must_no_encrypt);

    mk_rtsp_auth_invoker_do(invoker, 0, g_strPass.c_str());
}

class MediaRun : public mk_video_info {
public:
    MediaRun() {}
    ~MediaRun() {
        mk_media_release(media);
        media = nullptr;
    }
    mk_media media = nullptr;
};

H_MK_STREAM mk_add_stream(mk_video_info *vi) {
    if (vi == nullptr) {
        return nullptr;
    }

    MediaRun *pMedia = new MediaRun();
    mk_video_info *pB = pMedia;
    *pB = *vi;

    int codecid = pMedia->nCodecId;
    int w = pMedia->nW;
    int h = pMedia->nH;
    float fps = pMedia->fps;

    string strApp;
    strApp = string(pMedia->strApp) == "" ? "live" : pMedia->strApp;
    pMedia->media = mk_media_create("__defaultVhost__", strApp.c_str(), pMedia->strName, 0, 0, 0);
    // int audio_codec = -1;// mk_player_audio_codec_id(ctx->player);
    if (codecid != -1) {
        int nBitRate = 1024 * 1024 * 4;
        mk_media_init_video(pMedia->media, codecid, w, h, fps, 1024 * 1024 * 4);
    }

    mk_media_init_complete(pMedia->media);

    return pMedia;
}
void mk_del_stream(H_MK_STREAM *hs) {
    MediaRun **pM = (MediaRun **)hs;
    if (pM && *pM) {
        delete (*pM);
        *pM = 0;
    }
}
void mk_input_video(H_MK_STREAM hs, const void *pData, int nLen) {
    if (hs == nullptr || pData == nullptr || nLen <= 0) {
        return;
    }

    MediaRun *pm = (MediaRun *)hs;

    if (pm) {
        uint32_t tmsrap = GetTickCount_tt();

        switch (pm->nCodecId) {
            case 0: {
                // h264
                mk_media_input_h264(pm->media, pData, (int)nLen, tmsrap, tmsrap);
                break;
            }
            case 1: {
                // h265
                mk_media_input_h265(pm->media, pData, (int)nLen, tmsrap, tmsrap);
                break;
            }
        }
    }
}
string strIni = R"([general]
broadcast_player_count_changed=0
check_nvidia_dev=1
enableVhost=0
enable_ffmpeg_log=0
flowThreshold=1024
listen_ip=::
maxStreamWaitMS=15000
mediaServerId=ONTWIzVUs0fpAUqE
mergeWriteMS=0
resetWhenRePlay=1
streamNoneReaderDelayMS=20000
unready_frame_cache=100
wait_add_track_ms=3000
wait_audio_track_data_ms=1000
wait_track_ready_ms=10000

[hls]
broadcastRecordTs=0
deleteDelaySec=10
fastRegister=0
fileBufSize=65536
segDelay=0
segDur=2
segKeep=0
segNum=3
segRetain=5
tsNumInOneM3u8=720

[http]
allow_cross_domains=1
allow_ip_range=::1,127.0.0.1,172.16.0.0-172.31.255.255,192.168.0.0-192.168.255.255,10.0.0.0-10.255.255.255
charSet=utf-8
dirMenu=1
forbidCacheSuffix=
forwarded_ip_header=
keepAliveSecond=15
maxReqSize=40960
notFound=<html><head><title>404 Not Found</title></head><body bgcolor="white"><center><h1>您访问的资源不存在！</h1></center><hr><center>ZLMediaKit(git hash:/,branch:,build time:2025-10-11T15:43:56)</center></body></html>
rootPath=./www
sendBufSize=65536
virtualPath=

[multicast]
addrMax=239.255.255.255
addrMin=239.0.0.0
udpTTL=64

[protocol]
add_mute_audio=1
auto_close=0
continue_push_ms=15000
enable_audio=0
enable_fmp4=0
enable_hls=0
enable_hls_fmp4=0
enable_mp4=0
enable_rtmp=0
enable_rtsp=1
enable_ts=0
fmp4_demand=0
hls_demand=0
hls_save_path=./www
modify_stamp=1
mp4_as_player=0
mp4_max_second=3600
mp4_save_path=./www
paced_sender_ms=0
rtmp_demand=0
rtsp_demand=1
ts_demand=0

[record]
appName=record
enableFmp4=0
fastStart=0
fileBufSize=65536
fileRepeat=0
recordOnce=0
sampleMS=500

[rtmp]
directProxy=1
enhanced=0
handshakeSecond=15
keepAliveSecond=15

[rtp]
audioMtuSize=600
h264_stap_a=1
lowLatency=0
rtpMaxSize=10
videoMtuSize=1400

[rtp_proxy]
dumpDir=
gop_cache=1
h264_pt=98
h265_pt=99
opus_pt=100
port_range=30000-35000
ps_pt=96
rtp_g711_dur_ms=100
timeoutSec=15
udp_recv_socket_buffer=4194304

[rtsp]
authBasic=0
directProxy=1
handshakeSecond=15
keepAliveSecond=15
lowLatency=0
rtpTransportType=-1

[shell]
maxReqSize=1024

[srt]
latencyMul=4
passPhrase=
pktBufSize=8192
port=9000
timeoutSec=5)";
bool pathfile_exists(const char *path) {
    if (path == nullptr) {
        return false;
    }
    FILE *fp = fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    fclose(fp);
    return true;
}
int mk_start_server(int nPort, const char *user, const char *pass) {
    char *ini_path = mk_util_get_exe_dir("mk_server.ini");
    // char *ssl_path = ""//mk_util_get_exe_dir("ssl.p12");
    if (pathfile_exists(ini_path) == false) {
        FILE *fp = fopen(ini_path, "wb");
        if (fp) {
            fwrite(strIni.data(), 1, strIni.size(), fp);
            fclose(fp);
        }
    }

    if (user != nullptr) {
        g_strUser = user;
    }
    if (pass != nullptr) {
        g_strPass = pass;
    }

    mk_config config;
    memset(&config, 0, sizeof(config));

    config.ini = ini_path, config.ini_is_path = 1;
    config.log_level = 0;
    config.log_mask = LOG_CALLBACK;
    config.log_file_path = NULL;
    config.ssl = nullptr;
    config.thread_num = 0;

    mk_env_init(&config);
    free(ini_path);
    // free(ssl_path);

    // mk_http_server_start(80, 0);
    // mk_http_server_start(443, 1);
    mk_rtsp_server_start(nPort, 0);
    // mk_rtmp_server_start(1935, 0);
    // mk_shell_server_start(9000);
    // mk_rtp_server_start(10000);
    // mk_rtc_server_start(8000);
    // mk_srt_server_start(9000);

    mk_events events = {
        //.on_mk_media_changed = on_mk_media_changed,
        //.on_mk_media_publish = on_mk_media_publish,
        .on_mk_media_play = on_mk_media_play,
        //.on_mk_media_not_found = on_mk_media_not_found,
        .on_mk_media_no_reader = on_mk_media_no_reader,
        //.on_mk_http_request = on_mk_http_request,
        //.on_mk_http_access = on_mk_http_access,
        //.on_mk_http_before_access = on_mk_http_before_access,
        .on_mk_rtsp_get_realm = on_mk_rtsp_get_realm,
        .on_mk_rtsp_auth = on_mk_rtsp_auth,
        //.on_mk_record_mp4 = on_mk_record_mp4,
        //.on_mk_shell_login = on_mk_shell_login,
        //.on_mk_flow_report = on_mk_flow_report
    };
    if (g_strPass.empty() || g_strUser.empty()) {
        events.on_mk_rtsp_get_realm = nullptr;
        events.on_mk_rtsp_auth = nullptr;
    }
    mk_events_listen(&events);
    // log_info("media server %s", "stared!");

    // log_info("enter any key to exit");
    // getchar();

    // mk_stop_all_server();
    return 0;
}
