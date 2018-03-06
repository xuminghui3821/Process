#pragma  once
#if DISCERN
#include "Classifier.h"
#endif
#include <windows.h>
#include <iostream> 
#include <fstream> 
#include <time.h> 
#include <string.h>
#include <sstream>
#include <Shlwapi.h>
#pragma comment(lib,"shlwapi.lib") 
#include "process_of_communication.h"
#include "TimerThread.h"
#if DISCERN
#include "head.h" 
#endif
using namespace std;

extern BOOL Main_ServiceDataDispose();


void UnusualRecode(char *Info)//异常记录
{
	char errorlog_txt[MAX_PATH] = { 0 };
	ZeroMemory(errorlog_txt, MAX_PATH);
	GetModuleFileNameA(NULL, errorlog_txt, MAX_PATH);
	PathRemoveFileSpecA(errorlog_txt);
	PathAppendA(errorlog_txt, "errorlog.txt");
	SYSTEMTIME sys;
	GetLocalTime(&sys);

	ofstream   ofresult(errorlog_txt, ios::app);
	ofresult << "[" << sys.wYear << "-" << sys.wMonth << "-" << sys.wDay <<
		" " << sys.wHour << ":" << sys.wMinute << ":" << sys.wSecond << "]     " << Info << endl;
	ofresult.close();
}

VOID CALLBACK  HeartbeatDetection(void *lpParam, DWORD, DWORD)//心跳检测
{
	CProcessOfCommunication *pro = (CProcessOfCommunication *)lpParam;
	if (pro->m_port_ == CProcessOfCommunication::PORT_CLIENT)
	{
		pro->AddOnlineStatus(CLIENT_ONLINE);//设置心跳包，等待服务器接收并清除
		BOOL enable = FALSE;
		if (pro->CheckStatusEnable(SERVICE_ONLINE, enable))//查看服务器是否在线
		{
			if (enable == TRUE)//说明服务器在线
			{
				pro->DeleteOnlineStatus(SERVICE_ONLINE);//清空该状态，新的心跳包
				pro->heartbeat_delay_ = 0;
				pro->link_status = CProcessOfCommunication::LINK_SUCCESS;
			}
			else
			{
				pro->heartbeat_delay_++;
				if (pro->heartbeat_delay_ >= 6)//已经有1分钟无响应
				{
					PRINTF("与服务器失联");
					if (pro->heartbeat_delay_ == 6)
					{
						char buffer[MAX_PATH] = { 0 };
						sprintf_s(buffer, "%s__%d行 客户端与服务器失联", __FILE__, __LINE__, GetLastError());
						UnusualRecode(buffer);
					}
					pro->link_status = CProcessOfCommunication::SERVICE_DISCONNECT;//与服务器失联
				}
			}
		}

	}
	else
	{
		pro->AddOnlineStatus(SERVICE_ONLINE);//设置服务端心跳包
		BOOL enable = FALSE;
		if (pro->CheckStatusEnable(CLIENT_ONLINE, enable))//查看客户端是否在线
		{
			if (enable == TRUE)//说明客户在线
			{
				pro->DeleteOnlineStatus(CLIENT_ONLINE);//清空该状态，新的心跳包
				pro->heartbeat_delay_ = 0;
				if (pro->link_status = CProcessOfCommunication::CLIENT_DISCONNECT)//客户端重新连接
				{
					pro->client_agaim_coupling = TRUE;//客户端再次连接
				}
				pro->link_status = CProcessOfCommunication::LINK_SUCCESS;
			}
			else
			{
				pro->heartbeat_delay_++;
				if (pro->heartbeat_delay_ >= 6)//已经有1分钟无响应
				{
					PRINTF("与客户端失联\r\n");
					if (pro->heartbeat_delay_ == 6)
					{
						char buffer[MAX_PATH] = { 0 };
						sprintf_s(buffer, "%s__%d行 服务器与客户端失联", __FILE__, __LINE__, GetLastError());
						UnusualRecode(buffer);
					}
					pro->link_status = CProcessOfCommunication::CLIENT_DISCONNECT;//与客户端失联
				}
			}
		}
	}
}

//Mutex互斥锁
///////////////////////////////////////////////////////////////////
class CMutexLock
{
public:
	CMutexLock(WCHAR *MuteName) :
		init_success_(TRUE),
		mutex_lock_Name_(NULL),
		m_mutex_lock(NULL)
	{
		mutex_lock_Name_ = new WCHAR[MAX_PATH];
		lstrcpy(mutex_lock_Name_, MuteName);
		if (m_mutex_lock == NULL)
			m_mutex_lock = CreateMutex(NULL, FALSE, mutex_lock_Name_);

		if (m_mutex_lock == NULL)//创建失败
		{
			init_success_ = FALSE;
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 互斥锁创建失败,错误编号:%d", __FILE__, __LINE__, GetLastError());
			UnusualRecode(buffer);
		}
	}
	~CMutexLock()
	{
		if (m_mutex_lock != NULL)
			::CloseHandle(m_mutex_lock);
		delete mutex_lock_Name_;
	}

	BOOL Lock(DWORD waittime, DWORD &result) const{
		if (init_success_ == FALSE)
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 互斥锁初始化失败无法继续操作", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
		result = WaitForSingleObject(m_mutex_lock, waittime);
		return TRUE;
	}
	BOOL Unlock() const{
		if (init_success_ == FALSE)
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 互斥锁初始化失败无法继续操作", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
		::ReleaseMutex(m_mutex_lock);
		return TRUE;
	}
	BOOL GetInitSuccess()
	{
		return init_success_;
	}
private:
	BOOL   init_success_;//初始化成功标志
	HANDLE m_mutex_lock;
	WCHAR *mutex_lock_Name_;
};
///////////////////////////////////////////////////////////////////


//CEventLock
//////////////////////////////////////////////////////////////////////////
//事件锁类  
class CEventLock
{
public:
	CEventLock(WCHAR *MuteName) :
		init_success_(TRUE),
		event_lock_Name_(NULL),
		m_event_lock(NULL)
	{
		event_lock_Name_ = new WCHAR[MAX_PATH];
		ZeroMemory(event_lock_Name_, MAX_PATH * 2);
		lstrcpy(event_lock_Name_, MuteName);
		lstrcat(event_lock_Name_, L"_event_lock");
		m_event_lock = OpenEvent(EVENT_ALL_ACCESS, FALSE, event_lock_Name_);
		if (m_event_lock == NULL)
			m_event_lock = CreateEvent(NULL, FALSE, FALSE, event_lock_Name_);

		if (m_event_lock == NULL)//创建失败
		{
			init_success_ = FALSE;
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 事件锁创建失败,错误编号:%d", __FILE__, __LINE__, GetLastError());
			UnusualRecode(buffer);
		}
	}
	~CEventLock()
	{
		if (m_event_lock != NULL)
			::CloseHandle(m_event_lock);
		delete event_lock_Name_;
	}

	BOOL Lock(DWORD waittime, DWORD &result) const{
		if (init_success_ == FALSE)
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 事件锁初始化失败无法继续操作", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
		ResetEvent(m_event_lock);
		result = WaitForSingleObject(m_event_lock, waittime);
		return TRUE;
	}
	BOOL Unlock() const{
		if (init_success_ == FALSE)
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 事件锁初始化失败无法继续操作", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
		SetEvent(m_event_lock);
		return TRUE;
	}
	BOOL GetInitSuccess()
	{
		return init_success_;
	}
private:
	BOOL   init_success_;//初始化成功标志
	HANDLE m_event_lock;
	WCHAR *event_lock_Name_;
};
/////////////////////////////////////////////////////////////////////////////

//CFileMapping
//////////////////////////////////////////////////////////////////////////
//互斥对象锁类 
class CFileMapping
{
public:
	CFileMapping(WCHAR *file_mapping_name) :
		h_file_mapping_(NULL),
		m_read_data_(NULL),
		file_mapping_name_(NULL),
		shared_data_(NULL),
		init_success_(TRUE)
	{
		m_read_data_ = new Data();
		file_mapping_name_ = new WCHAR[MAX_PATH];
		ZeroMemory(m_read_data_, STRUCT_SIZE);
		ZeroMemory(file_mapping_name_, MAX_PATH * 2);

		lstrcpy(file_mapping_name_, file_mapping_name);
		lstrcat(file_mapping_name_, L"_mapping");
		if (h_file_mapping_ == NULL)
			h_file_mapping_ = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, file_mapping_name_);//如果已经存在该内存映射 则打开
		if (h_file_mapping_ == NULL)//打开已有的内存映射失败，则尝试创建
			h_file_mapping_ = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,
			PAGE_READWRITE, 0, STRUCT_SIZE, file_mapping_name_);//创建内存映射

		if (h_file_mapping_ != NULL)//创建成功
			shared_data_ = MapViewOfFile(h_file_mapping_, FILE_MAP_ALL_ACCESS, 0, 0, STRUCT_SIZE);//进行对象映射
		else//创建失败
		{
			init_success_ = FALSE;
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 内存映射申请失败，句柄为空,错误编号:%d", __FILE__, __LINE__, GetLastError());
			UnusualRecode(buffer);
			return;
		}

		if (shared_data_ == NULL)
		{
			init_success_ = FALSE;
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 MapViewOfFile()失败,错误编号:%d", __FILE__, __LINE__, GetLastError());
			UnusualRecode(buffer);
			return;
		}
	}
	~CFileMapping()
	{
		if (h_file_mapping_)
		{
			if (shared_data_)
			{
				// 卸载映射内存视图.
				UnmapViewOfFile(shared_data_);
				shared_data_ = NULL;
			}
			// 关闭映射文件对象
			CloseHandle(h_file_mapping_);
			h_file_mapping_ = NULL;
		}
		delete file_mapping_name_;
		delete m_read_data_;
	}
	//上锁与解锁
	BOOL ReadMapping()//读取映射文件
	{
		if (init_success_ == FALSE)
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 初始化失败无法继续操作", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
		m_read_data_ = (Data *)shared_data_;//读取内存中的数据
		return TRUE;
	}
	BOOL WriteMapping(Data *m_write_data)//写入映射文件
	{
		if (init_success_ == FALSE)
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 初始化失败无法继续操作", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
		memcpy_s(shared_data_, STRUCT_SIZE, m_write_data, STRUCT_SIZE);//将数据写入到内存中
		return TRUE;
	}
	BOOL GetInitSuccess()
	{
		return init_success_;
	}
	Data* GetData()
	{
		return m_read_data_;
	}

	BOOL SetOnlineStatus(int status)//用于更新线上状态
	{
		if (init_success_ == FALSE)
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 初始化失败无法继续操作", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
		((Data *)shared_data_)->communication_head_.status_ = status;
		return TRUE;
	}
	BOOL GetOnlineStatus(int &status)//用于获取线上最新状态
	{
		if (init_success_ == FALSE)
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 初始化失败无法继续操作", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
		status = ((Data *)shared_data_)->communication_head_.status_;//读取内存中的数据
		return TRUE;
	}
	Data *GetSharedData()//获取共享数据指针
	{
		return (Data *)shared_data_;
	}
private:
	Data *m_read_data_;
	BOOL   init_success_;//初始化成功标志
	HANDLE h_file_mapping_;
	WCHAR *file_mapping_name_;
	PVOID shared_data_;
};

////////////////////////////////////////////////////////////////////////////



//CProcessOfCommunication
////////////////////////////////////////////////////////////////////////////

CProcessOfCommunication::CProcessOfCommunication(WCHAR *file_mapping_name, PORT port, unsigned int handscake_wait_time) :
m_filemapping_(NULL),
service_event_lock_(NULL),
client_event_lock_(NULL),
client_thread_lock_(NULL),
m_port_(PORT_CLIENT),
m_online_status(0),
link_status(WAIT_HANDSCAKE),
_timer(NULL),
#if DISCERN
img(NULL),
classifier(NULL),
#endif
client_agaim_coupling(FALSE),
handscake_wait_time_(0xFFFFFFFF)
{
	m_port_ = port;
	m_filemapping_ = new CFileMapping(file_mapping_name);//初始化内存映射对象
	WCHAR w_buffer[MAX_PATH] = { 0 };
	lstrcpy(w_buffer, file_mapping_name);
	lstrcat(w_buffer, L"_client");
	client_event_lock_ = new CEventLock(w_buffer);//初始化客户端互斥对象
	lstrcpy(w_buffer, file_mapping_name);
	lstrcat(w_buffer, L"_service");
	service_event_lock_ = new CEventLock(w_buffer);//初始化服务器互斥对象
	if (m_port_ == PORT_CLIENT)
		client_thread_lock_ = new CMutexLock(L"Thread_Lock");//线程锁
	_timer = new CTimerThread();//定时器
	if (m_port_ == PORT_CLIENT)
	{
		DeleteOnlineStatus(CLIENT_ALL_STATUS);
		if (client_event_lock_->GetInitSuccess())
			m_online_status |= CLIENT_CLIENT_MUTEX;//记录当前的初始化状态
		if (service_event_lock_->GetInitSuccess())
			m_online_status |= CLIENT_SERVICE_MUTEX;//记录当前的初始化状态
		if (m_filemapping_->GetInitSuccess())
			m_online_status |= CLIENT_MAPPING;//记录当前的初始化状态
		if (client_event_lock_->GetInitSuccess()
			&& service_event_lock_->GetInitSuccess()
			&& m_filemapping_->GetInitSuccess())
			AddOnlineStatus(m_online_status);//将状态同步
	}
	else
	{
		DeleteOnlineStatus(SERVER_ALL_STATUS);
		if (client_event_lock_->GetInitSuccess())
			m_online_status |= SERVICE_CLIENT_MUTEX;//记录当前的初始化状态
		if (service_event_lock_->GetInitSuccess())
			m_online_status |= SERVICE_SERVICE_MUTEX;//记录当前的初始化状态
		if (m_filemapping_->GetInitSuccess())
			m_online_status |= SERVICE_MAPPING;//记录当前的初始化状态
		if (client_event_lock_->GetInitSuccess()
			&& service_event_lock_->GetInitSuccess()
			&& m_filemapping_->GetInitSuccess())
			AddOnlineStatus(m_online_status);//将状态同步
	}
	//这里初始化图像识别模块
#if DISCERN
	img = new cv::Mat();
	char buffer[MAX_PATH] = { 0 };
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	::google::InitGoogleLogging(buffer);
	string model_file = "";
	string trained_file = "";
	ZeroMemory(buffer, MAX_PATH);
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	PathRemoveFileSpecA(buffer);
	PathAppendA(buffer, "model\\deploy.prototxt");
	model_file = buffer;
	ZeroMemory(buffer, MAX_PATH);
	GetModuleFileNameA(NULL, buffer, MAX_PATH);
	PathRemoveFileSpecA(buffer);
	PathAppendA(buffer, "model\\resnet_50_1by2_nsfw.caffemodel");
	trained_file = buffer;
	classifier = new Classifier(model_file, trained_file);
#endif
}

CProcessOfCommunication::~CProcessOfCommunication()
{
	if (m_filemapping_)
		delete m_filemapping_;
	if (service_event_lock_)
		delete service_event_lock_;
	if (client_event_lock_)
		delete client_event_lock_;
#if DISCERN
	if (img)
		delete img;
	if (classifier)
		delete classifier;
#endif
	if (m_port_ == PORT_CLIENT && client_thread_lock_ != NULL)
		delete client_thread_lock_;
	if (_timer != NULL)
		_timer->KillTimer();
}

BOOL CProcessOfCommunication::Lock(PORT port, DWORD &result, DWORD wait_time)
{
	if (port == PORT_CLIENT)
	{
		DeleteOnlineStatus(CLIENT_UNLOCK);
		AddOnlineStatus(CLIENT_LOCK);
		return client_event_lock_->Lock(wait_time, result);
	}
	else
	{
		DeleteOnlineStatus(SERVICE_UNLOCK);
		AddOnlineStatus(SERVICE_LOCK);
		return service_event_lock_->Lock(wait_time, result);
	}
}

BOOL CProcessOfCommunication::Unlock(PORT port)
{
	if (port == PORT_CLIENT)
	{
		DeleteOnlineStatus(CLIENT_LOCK);
		AddOnlineStatus(CLIENT_UNLOCK);
		return client_event_lock_->Unlock();
	}
	else
	{
		DeleteOnlineStatus(SERVICE_LOCK);
		AddOnlineStatus(SERVICE_UNLOCK);
		return service_event_lock_->Unlock();
	}
}

BOOL CProcessOfCommunication::ReadMapping()//读取映射文件
{
	return m_filemapping_->ReadMapping();
}

BOOL CProcessOfCommunication::WriteMapping(Data *m_write_data)//写入映射文件
{
	return m_filemapping_->WriteMapping(m_write_data);
}

Data* CProcessOfCommunication::GetData()
{
	if (m_filemapping_->GetInitSuccess())
		return m_filemapping_->GetData();
	else
		return NULL;
}

Data* CProcessOfCommunication::GetSharedData()//获取共享数据指针
{
	if (m_filemapping_->GetInitSuccess())
		return m_filemapping_->GetSharedData();
	else
		return FALSE;
}
BOOL CProcessOfCommunication::HandscakeWait()//准备工作就绪进行握手操作
{
	GetOnlineStatus(m_online_status);
	if (m_port_ == PORT_CLIENT)
	{
		if ((m_online_status & CLIENT_CLIENT_MUTEX) && (m_online_status & CLIENT_SERVICE_MUTEX) && (m_online_status & CLIENT_MAPPING))
		{
			AddOnlineStatus(CLIENT_UNLOCK);
			AddOnlineStatus(CLIENT_HANDSCAKE_WAIT);
		}
		else//握手失败
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 客户端握手失败", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
	}
	else
	{
		if ((m_online_status & SERVICE_CLIENT_MUTEX) && (m_online_status & SERVICE_SERVICE_MUTEX) && (m_online_status & SERVICE_MAPPING))
		{
			AddOnlineStatus(SERVICE_UNLOCK);
			AddOnlineStatus(SERVICE_HANDSCAKE_WAIT);
		}
		else//握手失败
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 服务端握手失败", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
	}

	UINT time_m = 0;
	while (true)//等待服务端（客户端）建立握手协议在这里是阻塞的
	{

		GetOnlineStatus(m_online_status);//获取线上的状态
		if ((m_online_status & CLIENT_HANDSCAKE_WAIT) && (m_online_status & SERVICE_HANDSCAKE_WAIT))//如果双方均已握手成功，则跳出循环
			break;
		Sleep(1000);
		time_m += 1000;
		if (time_m >= handscake_wait_time_)//等待超时
		{
			char buffer[MAX_PATH] = { 0 };
			if (m_port_ == PORT_CLIENT)
				sprintf_s(buffer, "%s__%d行 客户端等待握手超时", __FILE__, __LINE__);
			else
				sprintf_s(buffer, "%s__%d行 服务端等待握手超时", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
	}

	link_status = LINK_SUCCESS;//连接成功
	_timer->CreateTimer(HEARTBEAT_TIME, HeartbeatDetection, this);
	return TRUE;
}
BOOL CProcessOfCommunication::AddOnlineStatus(int status)//用于更新线上状态
{
	int online_status = 0;
	if (GetOnlineStatus(online_status))
	{
		online_status |= status;//将属性加入
		m_filemapping_->SetOnlineStatus(online_status);
		return TRUE;
	}
	else
		return FALSE;
}

BOOL CProcessOfCommunication::DeleteOnlineStatus(int status)//用于获取线上最新状态
{
	int online_status = 0;
	if (GetOnlineStatus(online_status))
	{
		online_status &= ~status;//将属性删除
		m_filemapping_->SetOnlineStatus(online_status);
		return TRUE;
	}
	else
		return FALSE;
}

BOOL CProcessOfCommunication::GetOnlineStatus(int &status)//获取线上状态
{
	if (m_filemapping_->GetInitSuccess())
	{
		m_filemapping_->GetOnlineStatus(status);
		return TRUE;
	}
	else
	{
		char buffer[MAX_PATH] = { 0 };
		sprintf_s(buffer, "%s__%d行 获取线上状态失败，失败原因：映射初始化失败", __FILE__, __LINE__);
		UnusualRecode(buffer);
		return FALSE;
	}
}

BOOL CProcessOfCommunication::CheckStatusEnable(int status, BOOL &enable)//查看某一状态是否使能
{
	int onlie_status = 0;
	if (m_filemapping_->GetInitSuccess())
	{
		m_filemapping_->GetOnlineStatus(onlie_status);
		onlie_status &= status;
		if (onlie_status)
			enable = TRUE;
		else
			enable = FALSE;
		return TRUE;
	}
	else
	{
		char buffer[MAX_PATH] = { 0 };
		sprintf_s(buffer, "%s__%d行 获取线上状态失败，失败原因：映射初始化失败", __FILE__, __LINE__);
		UnusualRecode(buffer);
		return FALSE;
	}
}

BOOL CProcessOfCommunication::ClientReceiveDataStatus()//客户端接收数据状态
{
	if (m_filemapping_->GetInitSuccess())
	{
		if (m_port_ == PORT_CLIENT)
		{
			DeleteOnlineStatus(CLIENT_RECE_OVER);
			DeleteOnlineStatus(CLIENT_WAIT_RESULT);
			AddOnlineStatus(CLIENT_RECE_DATA);
			return TRUE;
		}
		else
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 服务端企图修改客户端状态", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
	}
	else
	{
		char buffer[MAX_PATH] = { 0 };
		sprintf_s(buffer, "%s__%d行 获取线上状态失败，失败原因：映射初始化失败", __FILE__, __LINE__);
		UnusualRecode(buffer);
		return FALSE;
	}
}

BOOL CProcessOfCommunication::ClientReceiveOverStatus()//客户端接收数据完毕状态
{
	if (m_filemapping_->GetInitSuccess())
	{
		if (m_port_ == PORT_CLIENT)
		{
			DeleteOnlineStatus(CLIENT_WAIT_RESULT);
			DeleteOnlineStatus(CLIENT_RECE_DATA);
			AddOnlineStatus(CLIENT_RECE_OVER);
			return TRUE;
		}
		else
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 服务端企图修改客户端状态", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
	}
	else
	{
		char buffer[MAX_PATH] = { 0 };
		sprintf_s(buffer, "%s__%d行 获取线上状态失败，失败原因：映射初始化失败", __FILE__, __LINE__);
		UnusualRecode(buffer);
		return FALSE;
	}
}

BOOL CProcessOfCommunication::ClientWaitResultStatus()//客户端等待服务器相应
{
	if (m_filemapping_->GetInitSuccess())
	{
		if (m_port_ == PORT_CLIENT)
		{
			DeleteOnlineStatus(CLIENT_RECE_DATA);
			DeleteOnlineStatus(CLIENT_RECE_OVER);
			AddOnlineStatus(CLIENT_WAIT_RESULT);
			return TRUE;
		}
		else
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 服务端企图修改客户端状态", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
	}
	else
	{
		char buffer[MAX_PATH] = { 0 };
		sprintf_s(buffer, "%s__%d行 获取线上状态失败，失败原因：映射初始化失败", __FILE__, __LINE__);
		UnusualRecode(buffer);
		return FALSE;
	}
}

BOOL CProcessOfCommunication::ServiceWaitDatrStatus()//服务器等待数据
{
	if (m_filemapping_->GetInitSuccess())
	{
		if (m_port_ == PORT_SERVICE)
		{
			DeleteOnlineStatus(SERVICE_RUN);
			DeleteOnlineStatus(SERVICE_RUN_OVER);
			AddOnlineStatus(SERVICE_WAIT_DATA);
			return TRUE;
		}
		else
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 客户端企图修改服务端状态", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
	}
	else
	{
		char buffer[MAX_PATH] = { 0 };
		sprintf_s(buffer, "%s__%d行 获取线上状态失败，失败原因：映射初始化失败", __FILE__, __LINE__);
		UnusualRecode(buffer);
		return FALSE;
	}
}

BOOL CProcessOfCommunication::ServiceRunStatus()//服务器处理数据
{
	if (m_filemapping_->GetInitSuccess())
	{
		if (m_port_ == PORT_SERVICE)
		{
			DeleteOnlineStatus(SERVICE_RUN_OVER);
			DeleteOnlineStatus(SERVICE_WAIT_DATA);
			AddOnlineStatus(SERVICE_RUN);
			return TRUE;
		}
		else
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 客户端企图修改服务端状态", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
	}
	else
	{
		char buffer[MAX_PATH] = { 0 };
		sprintf_s(buffer, "%s__%d行 获取线上状态失败，失败原因：映射初始化失败", __FILE__, __LINE__);
		UnusualRecode(buffer);
		return FALSE;
	}
}

BOOL CProcessOfCommunication::ServiceRunOverStatus()//服务器处理数据完毕
{
	if (m_filemapping_->GetInitSuccess())
	{
		if (m_port_ == PORT_SERVICE)
		{
			DeleteOnlineStatus(SERVICE_WAIT_DATA);
			DeleteOnlineStatus(SERVICE_RUN);
			AddOnlineStatus(SERVICE_RUN_OVER);
			return TRUE;
		}
		else
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 客户端企图修改服务端状态", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
	}
	else
	{
		char buffer[MAX_PATH] = { 0 };
		sprintf_s(buffer, "%s__%d行 获取线上状态失败，失败原因：映射初始化失败", __FILE__, __LINE__);
		UnusualRecode(buffer);
		return FALSE;
	}
}

BOOL  CProcessOfCommunication::WaitClientLock()//等待客户端上锁
{
	UINT wait_time = 0;
	do{//该循环判断客户端是否上锁
		GetOnlineStatus(m_online_status);
		if (m_online_status & CLIENT_LOCK)//如果客户端上锁了
			return TRUE;
		if (client_agaim_coupling)
		{
			client_agaim_coupling = FALSE;//客户端再次连接，终止死锁
			return FALSE;
		}
		Sleep(100);
		wait_time += 100;
		if (wait_time >= WAIT_LOCK_MAX_TIME)
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 等待客户端上锁超时", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
	} while (TRUE);
	return TRUE;
}

BOOL  CProcessOfCommunication::WaitServiceLock()//等待服务端上锁
{
	UINT wait_time = 0;
	do{//该循环判断客户端是否上锁
		GetOnlineStatus(m_online_status);
		if (m_online_status & SERVICE_LOCK)//如果服务端上锁了
			return TRUE;
		Sleep(100);
		wait_time += 100;
		if (wait_time >= WAIT_LOCK_MAX_TIME)
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d行 等待服务端上锁超时", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
	} while (TRUE);
	return TRUE;
}

BOOL CProcessOfCommunication::ServiceDispose()//服务器的操作
{
	if (GetOnlineStatus(m_online_status) && m_port_ == PORT_SERVICE
		&& m_online_status & SERVICE_UNLOCK && link_status == LINK_SUCCESS)//确定当前服务器是解锁状态
	{
		if (!ServiceWaitDatrStatus())//服务器等待数据
			return FALSE;
		BOOL client_unlock_ = FALSE;//用于查看客户端是否上锁
		if (!CheckStatusEnable(CLIENT_UNLOCK, client_unlock_))//查看客户端是否上锁
			return FALSE;
		if (client_unlock_)//如果客户端没有上锁则服务端才可以上锁
		{
			DWORD result = 0;
			Lock(PORT_SERVICE, result);//服务端的锁只有在客户端操作中才能解锁
			if (link_status != LINK_SUCCESS)//如果与客户端断开连接，服务端会在这里阻塞，针对该情况在这里进行判断，并进行处理
			{
				DeleteOnlineStatus(CLIENT_ONLINE);//将客户端的心跳状态清除
				heartbeat_delay_ = 0;
				link_status = CProcessOfCommunication::LINK_SUCCESS;
			}
		}

		if (!ServiceRunStatus())//服务器处理数据状态
			return FALSE;
		//运行到这里说明在客户端已经解除服务锁
		ServiceDataDispose();//数据处理操作
		//Sleep(1000);//模拟数据处理
		if (!ServiceRunOverStatus())//服务器处理数据完毕
			return FALSE;
		if (!WaitClientLock())//等待超时则返回FALSE
			return FALSE;
		Unlock(PORT_CLIENT);//解开客户端的锁
		return TRUE;
	}
	else
	{
		char buffer[MAX_PATH] = { 0 };
		if (m_port_ == PORT_CLIENT)
			sprintf_s(buffer, "%s__%d行 服务端企图使用客户端操作", __FILE__, __LINE__);
		else if (m_online_status & SERVICE_LOCK)
			sprintf_s(buffer, "%s__%d行 服务端已经上锁", __FILE__, __LINE__);
		UnusualRecode(buffer);
		return FALSE;
	}
}

BOOL CProcessOfCommunication::ClientDispose(char *filepath, DWORD wait_time)//客户端的操作
{
	DWORD result = 0;
	client_thread_lock_->Lock(wait_time, result);//为了保证同一时段只有一个线程与服务器通讯
	if (result == WAIT_TIMEOUT)//等待超时
	{
		ServiceDisposeError();
		goto unlock;
	}
	result = 0;
	if (GetOnlineStatus(m_online_status) && m_port_ == PORT_CLIENT
		&& m_online_status & CLIENT_UNLOCK && link_status == LINK_SUCCESS)
	{
		Data *a = m_filemapping_->GetData();
		if (!ClientReceiveDataStatus())//客户端接收数据状态
			goto unlock;
		PRINTF("将%s传给服务端\r\n", filepath);
		ClientDataDispose(filepath);//将数据写入共享内存中
		//Sleep(1000);
		if (!ClientReceiveOverStatus())//客户端接收数据状态成功
			goto unlock;
		if (!WaitServiceLock())//等待超时则返回FALSE
			goto unlock;
		Unlock(PORT_SERVICE);//解开服务端的锁
		if (!ClientWaitResultStatus())//客户端等待服务器相应
			goto unlock;
		BOOL service_unlock_ = FALSE;//用于查看客户端是否上锁
		if (!CheckStatusEnable(SERVICE_UNLOCK, service_unlock_))//查看客户端是否上锁
			goto unlock;
		if (service_unlock_)//如果客户端没有上锁则服务端才可以上锁
		{
			Lock(PORT_CLIENT, result);//客户端上锁
			//WAIT_OBJECT_0 等到信号量导致退出
			//WAIT_TIMEOUT 等待超时
			if (result == WAIT_TIMEOUT)//等待超时导致的退出
			{
				//这里实际上是没有处理完数据的，因为等待超时所以导致的退出
				PRINTF("等待超时,数据未处理\r\n");
				ServiceDisposeError();//数据在这里进行处理
			}
		}

		client_thread_lock_->Unlock();
		return TRUE;
	}
	else if (link_status != LINK_SUCCESS)//由于中断链接导致的无法处理
	{//通过其他方式进行处理
		PRINTF("中断连接%s\r\n", filepath);
		ServiceDisposeError();
	}
	else
	{
		char buffer[MAX_PATH] = { 0 };
		if (m_port_ == PORT_CLIENT)
			sprintf_s(buffer, "%s__%d行 客户端企图使用服务端操作", __FILE__, __LINE__);
		else if (m_online_status & CLIENT_LOCK)
			sprintf_s(buffer, "%s__%d行 客户端已经上锁", __FILE__, __LINE__);
		UnusualRecode(buffer);
		goto unlock;
	}
unlock:
	client_thread_lock_->Unlock();
	return FALSE;
}

BOOL CProcessOfCommunication::ServiceDataDispose()
{
#if DISCERN
	Main_ServiceDataDispose();
#else
	ReadMapping();
	Data *send_data_ = m_filemapping_->GetSharedData();
	PRINTF("%s\r\n",send_data_->communication_head_.client_to_server_.file_name_);
	//std::vector<char> vec_data(
	//	send_data_->communication_head_.client_to_server_.data_,
	//	send_data_->communication_head_.client_to_server_.data_ +
	//	send_data_->communication_head_.client_to_server_.data_size_);
	////*img = cv::imdecode(vec_data, 1);
	////float prediction = classifier->Classify(*img);
	//PRINTF("%s", send_data_->communication_head_.client_to_server_.file_name_);
	//if (prediction < 0.1)
	//{
	//	send_data_->communication_head_.server_to_client_.result_ =
	//		Data::CommunicationData::ServerToClient::RESULT_NORMAL;
	//	PRINTF("是正常图片\r\n");
	//}
	//else if (prediction < 0.8)
	//{
	//	send_data_->communication_head_.server_to_client_.result_ =
	//		Data::CommunicationData::ServerToClient::RESULT_SEX;
	//	PRINTF("是性感图片\r\n");
	//}
	//else
	//{
	//	send_data_->communication_head_.server_to_client_.result_ =
	//		Data::CommunicationData::ServerToClient::RESULT_PORNOGRAPHIC;
	//	PRINTF("是色情图片\r\n");
	//}
	//cv::waitKey();
#endif
	return TRUE;
}

BOOL CProcessOfCommunication::ClientDataDispose(char *filepath)
{
	if (!PathFileExistsA(filepath))
		return FALSE;
	filebuf *pbuf;
	ifstream filestr;
	Data *send_data_ = m_filemapping_->GetSharedData();


	//stringstream sstr;
	//sstr.clear();
	//sstr << filepath;
	//sstr >> send_data_->communication_head_.client_to_server_.file_name_;//将文件路径保存

	strcpy_s(send_data_->communication_head_.client_to_server_.file_name_, filepath);
	// 要读入整个文件，必须采用二进制打开   
	filestr.open(filepath, ios::binary);
	// 获取filestr对应buffer对象的指针   
	pbuf = filestr.rdbuf();
	// 调用buffer对象方法获取文件大小  
	send_data_->communication_head_.client_to_server_.data_size_ = pbuf->pubseekoff(0, ios::end, ios::in);//写入文件大小
	pbuf->pubseekpos(0, ios::in);
	// 分配内存空间  
	// 获取文件内容 
	pbuf->sgetn(send_data_->communication_head_.client_to_server_.data_,
		send_data_->communication_head_.client_to_server_.data_size_);//写入文件数据
	filestr.close();

	return true;
}

BOOL CProcessOfCommunication::ServiceDisposeError()
{
	static int i = 0;
	PRINTF("由于等待超时，无法处理:%d\r\n", i++);
	return TRUE;
}
////////////////////////////////////////////////////////////////////////////////////////