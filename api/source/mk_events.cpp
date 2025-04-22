/*
 * Copyright (c) 2016-present The ZLMediaKit project authors. All Rights Reserved.
 *
 * This file is part of ZLMediaKit(https://github.com/ZLMediaKit/ZLMediaKit).
 *
 * Use of this source code is governed by MIT-like license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include "mk_events.h"
#include "Common/config.h"
#include "Common/MediaSource.h"
#include "Http/HttpSession.h"
#include "Rtsp/RtspSession.h"
#include "Record/MP4Recorder.h"

#ifdef ENABLE_WEBRTC
#include "webrtc/WebRtcTransport.h"
#endif

using namespace toolkit;
using namespace mediakit;

static void* s_tag;
static mk_events s_events = {0};

API_EXPORT void API_CALL mk_events_listen(const mk_events *events){
    if (events) {
        memcpy(&s_events, events, sizeof(s_events));
    } else {
        memset(&s_events, 0, sizeof(s_events));
    }

    static onceToken token([]{
		if (s_events.on_mk_media_changed) {
			NoticeCenter::Instance().addListener(&s_tag, Broadcast::kBroadcastMediaChanged, [](BroadcastMediaChangedArgs) {
					s_events.on_mk_media_changed(bRegist,
												 (mk_media_source)&sender);
			});
		}
		if (s_events.on_mk_record_mp4) {
			NoticeCenter::Instance().addListener(&s_tag, Broadcast::kBroadcastRecordMP4, [](BroadcastRecordMP4Args) {
				s_events.on_mk_record_mp4((mk_record_info)&info);
				});
		}

		if (s_events.on_mk_record_start_or_stop) {
			NoticeCenter::Instance().addListener(&s_tag, Broadcast::kBroadcastRecordStartOrStop, [](BroadcastRecordStartOrStopArgs) {
				s_events.on_mk_record_start_or_stop(bStartOrStop, recordName.c_str(), app.c_str(), streamId.c_str());
				});
		}

		if (s_events.on_mk_record_ts) {
			NoticeCenter::Instance().addListener(&s_tag, Broadcast::kBroadcastRecordTs, [](BroadcastRecordTsArgs) {
				s_events.on_mk_record_ts((mk_record_info)&info);
				});
		}

		if (s_events.on_mk_http_request) {
			NoticeCenter::Instance().addListener(&s_tag, Broadcast::kBroadcastHttpRequest, [](BroadcastHttpRequestArgs) {
				int consumed_int = consumed;
				s_events.on_mk_http_request((mk_parser)&parser,
					(mk_http_response_invoker)&invoker,
					&consumed_int,
					(mk_sock_info)&sender);
				consumed = consumed_int;
				});
		}

		if (s_events.on_mk_http_access) {
			NoticeCenter::Instance().addListener(&s_tag, Broadcast::kBroadcastHttpAccess, [](BroadcastHttpAccessArgs) {
				s_events.on_mk_http_access((mk_parser)&parser,
					path.c_str(),
					is_dir,
					(mk_http_access_path_invoker)&invoker,
					(mk_sock_info)&sender);
				});
		}
		if (s_events.on_mk_http_before_access) {
			NoticeCenter::Instance().addListener(&s_tag, Broadcast::kBroadcastHttpBeforeAccess, [](BroadcastHttpBeforeAccessArgs) {
				char path_c[4 * 1024] = { 0 };
				strcpy(path_c, path.c_str());
				s_events.on_mk_http_before_access((mk_parser)&parser,
					path_c,
					(mk_sock_info)&sender);
				path = path_c;
				});
		}

		if (s_events.on_mk_rtsp_get_realm) {
			NoticeCenter::Instance().addListener(&s_tag, Broadcast::kBroadcastOnGetRtspRealm, [](BroadcastOnGetRtspRealmArgs) {
				s_events.on_mk_rtsp_get_realm((mk_media_info)&args,
					(mk_rtsp_get_realm_invoker)&invoker,
					(mk_sock_info)&sender);
				});
		}

		if (s_events.on_mk_rtsp_auth) {
			NoticeCenter::Instance().addListener(&s_tag, Broadcast::kBroadcastOnRtspAuth, [](BroadcastOnRtspAuthArgs) {
				s_events.on_mk_rtsp_auth((mk_media_info)&args,
					realm.c_str(),
					user_name.c_str(),
					must_no_encrypt,
					(mk_rtsp_auth_invoker)&invoker,
					(mk_sock_info)&sender);
				});
		}

		if (s_events.on_mk_media_publish) {
			NoticeCenter::Instance().addListener(&s_tag,Broadcast::kBroadcastMediaPublish,[](BroadcastMediaPublishArgs){
					s_events.on_mk_media_publish((mk_media_info) &args,
												 (mk_publish_auth_invoker) &invoker,
												 (mk_sock_info) &sender);
			});
		}

		if (s_events.on_mk_media_play) {
			NoticeCenter::Instance().addListener(&s_tag, Broadcast::kBroadcastMediaPlayed, [](BroadcastMediaPlayedArgs) {
				s_events.on_mk_media_play((mk_media_info)&args,
					(mk_auth_invoker)&invoker,
					(mk_sock_info)&sender);
				});
		}

		if (s_events.on_mk_shell_login) {
			NoticeCenter::Instance().addListener(&s_tag, Broadcast::kBroadcastShellLogin, [](BroadcastShellLoginArgs) {
				s_events.on_mk_shell_login(user_name.c_str(),
					passwd.c_str(),
					(mk_auth_invoker)&invoker,
					(mk_sock_info)&sender);
				});
		}

		if (s_events.on_mk_flow_report) {
			NoticeCenter::Instance().addListener(&s_tag, Broadcast::kBroadcastFlowReport, [](BroadcastFlowReportArgs) {
				s_events.on_mk_flow_report((mk_media_info)&args,
					totalBytes,
					totalDuration,
					isPlayer,
					(mk_sock_info)&sender);
				});
		}

		if (s_events.on_mk_media_not_found) {
			NoticeCenter::Instance().addListener(&s_tag, Broadcast::kBroadcastNotFoundStream, [](BroadcastNotFoundStreamArgs) {
				if (s_events.on_mk_media_not_found((mk_media_info)&args,
					(mk_sock_info)&sender)) {
					closePlayer();
				}
				});
		}

		if (s_events.on_mk_media_no_reader) {
			NoticeCenter::Instance().addListener(&s_tag, Broadcast::kBroadcastStreamNoneReader, [](BroadcastStreamNoneReaderArgs) {
				s_events.on_mk_media_no_reader((mk_media_source)&sender);
				});
		}

		if (s_events.on_mk_log) {
			NoticeCenter::Instance().addListener(&s_tag, EventChannel::kBroadcastLogEvent, [](BroadcastLogEventArgs) {
				auto log = ctx->str();
				s_events.on_mk_log((int)ctx->_level, ctx->_file.data(), ctx->_line, ctx->_function.data(), log.data());
				});
		}

		if (s_events.on_mk_media_send_rtp_stop) {
			NoticeCenter::Instance().addListener(&s_tag, Broadcast::kBroadcastSendRtpStopped,[](BroadcastSendRtpStoppedArgs){
					s_events.on_mk_media_send_rtp_stop(sender.getMediaTuple().vhost.c_str(), sender.getMediaTuple().app.c_str(),
													   sender.getMediaTuple().stream.c_str(), ssrc.c_str(), ex.getErrCode(), ex.what());
			});
		}

#ifdef ENABLE_WEBRTC
		if (s_events.on_mk_rtc_sctp_connecting) {
			NoticeCenter::Instance().addListener(&s_tag, Broadcast::kBroadcastRtcSctpConnecting, [](BroadcastRtcSctpConnectArgs) {
				s_events.on_mk_rtc_sctp_connecting((mk_rtc_transport)&sender);
				});
		}

		if (s_events.on_mk_rtc_sctp_connected) {
			NoticeCenter::Instance().addListener(&s_tag, Broadcast::kBroadcastRtcSctpConnected, [](BroadcastRtcSctpConnectArgs) {
				s_events.on_mk_rtc_sctp_connected((mk_rtc_transport)&sender);
				});
		}

		if (s_events.on_mk_rtc_sctp_failed) {
			NoticeCenter::Instance().addListener(&s_tag, Broadcast::kBroadcastRtcSctpFailed, [](BroadcastRtcSctpConnectArgs) {
				s_events.on_mk_rtc_sctp_failed((mk_rtc_transport)&sender);
				});
		}

		if (s_events.on_mk_rtc_sctp_closed) {
			NoticeCenter::Instance().addListener(&s_tag, Broadcast::kBroadcastRtcSctpClosed, [](BroadcastRtcSctpConnectArgs) {
				s_events.on_mk_rtc_sctp_closed((mk_rtc_transport)&sender);
				});
		}

		if (s_events.on_mk_rtc_sctp_send) {
			NoticeCenter::Instance().addListener(&s_tag, Broadcast::kBroadcastRtcSctpSend, [](BroadcastRtcSctpSendArgs) {
				s_events.on_mk_rtc_sctp_send((mk_rtc_transport)&sender, data, len);
				});
		}

		if (s_events.on_mk_rtc_sctp_received) {

			NoticeCenter::Instance().addListener(&s_tag, Broadcast::kBroadcastRtcSctpReceived, [](BroadcastRtcSctpReceivedArgs) {
				s_events.on_mk_rtc_sctp_received((mk_rtc_transport)&sender, streamId, ppid, msg, len);
				});
		}

#endif
    });

}
