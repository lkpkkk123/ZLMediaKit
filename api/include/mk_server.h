struct mk_video_info
{
	char strName[64];
	char strApp[64];
	int nW;
	int nH;
	int nCodecId;//0: 264 1:265
	float fps;//帧率

};
#define H_MK_STREAM void*

int mk_start_server(int port, const char* user ,const char* pass);//启动服务器，port：rtsp端口  user rtsp用户名，为null时不需要用户名认证, pass 密码
H_MK_STREAM mk_add_stream(mk_video_info *vi);//添加一路流
void mk_del_stream(H_MK_STREAM *hs);//移除一路流
void mk_input_video(H_MK_STREAM hs, const void* pData, int nLen);//输入某个流的数据