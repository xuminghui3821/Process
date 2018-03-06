#pragma  once
#ifndef _process_of_communication_H  
#define _process_of_communication_H

#define STRUCT_SIZE  2097152 //数据交互结构的总大小  
#define DATA_MAX_SIZE 2000000//传续文件的最大大小
#define NAME_LENG  1024//文件路径最大大小
#define WAIT_LOCK_MAX_TIME  1000000//用于记录等待上锁最大时间单位毫秒
#define HEARTBEAT_TIME      1000 // 心跳间隔时间

#define CLIENT_MAPPING				0x00001//客户端内存映射初始化完毕
#define SERVICE_MAPPING				0x00002//服务器内存映射初始化完毕
#define CLIENT_CLIENT_MUTEX			0x00004//客户端的客户互斥锁初始化完毕
#define CLIENT_SERVICE_MUTEX		0x00008//客户端的服务互斥锁初始化完毕
#define SERVICE_CLIENT_MUTEX		0x00010//服务端的客户互斥锁初始化完毕
#define SERVICE_SERVICE_MUTEX		0x00020//服务端的服务互斥锁初始化完毕
#define CLIENT_HANDSCAKE_WAIT       0x00040//客户端握手等待
#define SERVICE_HANDSCAKE_WAIT 		0x00080//服务器握手等待

#define CLIENT_RECE_DATA            0x00100//客户端接收数据
#define CLIENT_RECE_OVER			0x00200//客户端数据接收完毕
#define CLIENT_WAIT_RESULT          0x00400//客户端数据接收完毕已将数据写入内存映射中，等待服务器处理完毕
#define SERVICE_WAIT_DATA           0x00800//服务器等待数据
#define SERVICE_RUN                 0x01000//服务器处理数据
#define SERVICE_RUN_OVER            0x02000//服务器数据处理完毕
#define CLIENT_UNLOCK               0x04000//客户端解锁状态
#define CLIENT_LOCK                 0x08000//客户端上锁状态

#define SERVICE_UNLOCK              0x10000//服务器解锁状态
#define SERVICE_LOCK                0x20000//服务器上锁状态
#define SERVICE_ONLINE              0x40000//服务器在线，用于在上锁之前进行确认，不然会造成进程锁死
#define CLIENT_ONLINE               0x80000//客户端在线,同上
#define SERVER_ALL_STATUS           0x738B2//所有的服务端属性
#define CLIENT_ALL_STATUS           0x8C74D//所有的客户端属性


#define DISCERN  0//屏蔽了在CProcessOfCommunication类中使用Classifier
#define CONSOLE  1//当前是否在控制台环境下

#if CONSOLE
#define  PRINTF(...) printf(__VA_ARGS__)
#else
#define  PRINTF(...)
#endif
#pragma pack(1) //让编译器对这个结构作1字节对齐
union Data
{
	struct CommunicationData//用于双方通讯的数据
	{
		int status_;//当前的状态
		struct ClientToServer//客户端传给服务器需要处理的数据
		{
			unsigned int data_size_;//用于记录数据大小
			char  file_name_[NAME_LENG];//用于记录文件名
			char  data_[DATA_MAX_SIZE];//接收数据的最大上限
		}client_to_server_;
		struct ServerToClient//服务器处理完数据之后所返回的数据
		{
			enum RESULT//处理的结果
			{
				RESULT_UNDISPOSED = 0,//未处理状态
				RESULT_ERROR,//处理发生错误
				RESULT_NORMAL,//正常
				RESULT_SEX,//性感
				RESULT_PORNOGRAPHIC//色情
			};
			RESULT result_;//存放处理结果
		}server_to_client_;
	}communication_head_;
	char StructSize_[STRUCT_SIZE];//用于记录该结构的大小
};

class CEventLock;//事件锁
class CMutexLock;//事件锁
class CFileMapping;//内存映射类
class CTimerThread;
#if DISCERN
class Classifier;
namespace cv { class Mat; }
#endif







class CProcessOfCommunication//进程通讯类
{

public:	
	enum PORT//当前的程序时客户端还是服务端
	{
		PORT_CLIENT,//客户端
		PORT_SERVICE//服务端
	};
	enum LINK_STATUS
	{
		WAIT_HANDSCAKE,//等待握手
		CLIENT_DISCONNECT,//客户端断开连接
		SERVICE_DISCONNECT,//服务器断开连接
		LINK_SUCCESS//连接成功
	};
	CProcessOfCommunication(WCHAR *file_mapping_name, PORT port, unsigned int handscake_wait_time = 0xFFFFFFFF);
	//第一个参数是内存映射和互斥量的名字
	//第二个参数是指当前的端是客户端还是服务端
	//第三个参数是握手协议等待时长默认是一直等待
	~CProcessOfCommunication();
//上锁与解锁
	BOOL Lock(PORT port, DWORD &result, DWORD wait_time = INFINITE);
	BOOL Unlock(PORT port);
	BOOL ReadMapping();//读取映射文件
	BOOL WriteMapping(Data *m_write_data);//写入映射文件
	Data *GetSharedData();//获取共享数据指针
	Data *GetData();//获取数据结构体
	BOOL HandscakeWait();//准备工作就绪进行握手操作

	//状态相关的操作
	BOOL AddOnlineStatus(int status);//用于更新线上状态
	BOOL DeleteOnlineStatus(int status);//用于获取线上最新状态
	BOOL GetOnlineStatus(int &status);//获取线上的状态
	BOOL CheckStatusEnable(int status,BOOL &enable);//查看某一状态是否使能
	BOOL ClientReceiveDataStatus();//客户端接收数据状态
	BOOL ClientReceiveOverStatus();//客户端接收数据完毕状态
	BOOL ClientWaitResultStatus();//客户端等待服务器相应
	BOOL ServiceWaitDatrStatus();//服务器等待数据
	BOOL ServiceRunStatus();//服务器处理数据
	BOOL ServiceRunOverStatus();//服务器处理数据完毕

	//等待客户端/服务端上锁
	BOOL WaitClientLock();//等待客户端上锁
	BOOL WaitServiceLock();//等待服务端上锁

	//该服务器和客户端的操作采用一问一答的方式进行 可根据实际情况进行修改
	BOOL ServiceDispose();//服务器的操作
	BOOL ClientDispose(char *filepath, DWORD wait_time = INFINITE);//客户端的操作

	//下面的函数是数据处理相关的操作
	BOOL ServiceDataDispose();
	BOOL ClientDataDispose(char *filepath);
	BOOL ServiceDisposeError();//当数据未能通过服务器来进行处理
public:
	int m_online_status;//用于保存获取线上的状态
	PORT m_port_;//当前的端口
	CEventLock *service_event_lock_;//服务端互斥锁运用方法为在服务端上锁，在客户端解锁
	CEventLock *client_event_lock_;//客户端互斥锁运用方法为再客户端上锁，在客户端解锁
	CMutexLock *client_thread_lock_;//在客户端传输数据时，保证同一时段只有一个客户端与服务器通讯
	CFileMapping *m_filemapping_;
	UINT handscake_wait_time_;//握手等待时间
	LINK_STATUS link_status;//连接状态
	CTimerThread *_timer;
	int heartbeat_delay_;//心跳延迟，如果长达一分钟之后连接不上对端则认为断开连接
	BOOL client_agaim_coupling;//客户端重新连接，主要用于接触服务器等待客户端死锁问题
#if DISCERN
	cv::Mat *img;
	Classifier *classifier;
#endif
};



#endif