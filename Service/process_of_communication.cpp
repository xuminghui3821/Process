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


void UnusualRecode(char *Info)//�쳣��¼
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

VOID CALLBACK  HeartbeatDetection(void *lpParam, DWORD, DWORD)//�������
{
	CProcessOfCommunication *pro = (CProcessOfCommunication *)lpParam;
	if (pro->m_port_ == CProcessOfCommunication::PORT_CLIENT)
	{
		pro->AddOnlineStatus(CLIENT_ONLINE);//�������������ȴ����������ղ����
		BOOL enable = FALSE;
		if (pro->CheckStatusEnable(SERVICE_ONLINE, enable))//�鿴�������Ƿ�����
		{
			if (enable == TRUE)//˵������������
			{
				pro->DeleteOnlineStatus(SERVICE_ONLINE);//��ո�״̬���µ�������
				pro->heartbeat_delay_ = 0;
				pro->link_status = CProcessOfCommunication::LINK_SUCCESS;
			}
			else
			{
				pro->heartbeat_delay_++;
				if (pro->heartbeat_delay_ >= 6)//�Ѿ���1��������Ӧ
				{
					PRINTF("�������ʧ��");
					if (pro->heartbeat_delay_ == 6)
					{
						char buffer[MAX_PATH] = { 0 };
						sprintf_s(buffer, "%s__%d�� �ͻ����������ʧ��", __FILE__, __LINE__, GetLastError());
						UnusualRecode(buffer);
					}
					pro->link_status = CProcessOfCommunication::SERVICE_DISCONNECT;//�������ʧ��
				}
			}
		}

	}
	else
	{
		pro->AddOnlineStatus(SERVICE_ONLINE);//���÷����������
		BOOL enable = FALSE;
		if (pro->CheckStatusEnable(CLIENT_ONLINE, enable))//�鿴�ͻ����Ƿ�����
		{
			if (enable == TRUE)//˵���ͻ�����
			{
				pro->DeleteOnlineStatus(CLIENT_ONLINE);//��ո�״̬���µ�������
				pro->heartbeat_delay_ = 0;
				if (pro->link_status = CProcessOfCommunication::CLIENT_DISCONNECT)//�ͻ�����������
				{
					pro->client_agaim_coupling = TRUE;//�ͻ����ٴ�����
				}
				pro->link_status = CProcessOfCommunication::LINK_SUCCESS;
			}
			else
			{
				pro->heartbeat_delay_++;
				if (pro->heartbeat_delay_ >= 6)//�Ѿ���1��������Ӧ
				{
					PRINTF("��ͻ���ʧ��\r\n");
					if (pro->heartbeat_delay_ == 6)
					{
						char buffer[MAX_PATH] = { 0 };
						sprintf_s(buffer, "%s__%d�� ��������ͻ���ʧ��", __FILE__, __LINE__, GetLastError());
						UnusualRecode(buffer);
					}
					pro->link_status = CProcessOfCommunication::CLIENT_DISCONNECT;//��ͻ���ʧ��
				}
			}
		}
	}
}

//Mutex������
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

		if (m_mutex_lock == NULL)//����ʧ��
		{
			init_success_ = FALSE;
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d�� ����������ʧ��,������:%d", __FILE__, __LINE__, GetLastError());
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
			sprintf_s(buffer, "%s__%d�� ��������ʼ��ʧ���޷���������", __FILE__, __LINE__);
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
			sprintf_s(buffer, "%s__%d�� ��������ʼ��ʧ���޷���������", __FILE__, __LINE__);
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
	BOOL   init_success_;//��ʼ���ɹ���־
	HANDLE m_mutex_lock;
	WCHAR *mutex_lock_Name_;
};
///////////////////////////////////////////////////////////////////


//CEventLock
//////////////////////////////////////////////////////////////////////////
//�¼�����  
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

		if (m_event_lock == NULL)//����ʧ��
		{
			init_success_ = FALSE;
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d�� �¼�������ʧ��,������:%d", __FILE__, __LINE__, GetLastError());
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
			sprintf_s(buffer, "%s__%d�� �¼�����ʼ��ʧ���޷���������", __FILE__, __LINE__);
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
			sprintf_s(buffer, "%s__%d�� �¼�����ʼ��ʧ���޷���������", __FILE__, __LINE__);
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
	BOOL   init_success_;//��ʼ���ɹ���־
	HANDLE m_event_lock;
	WCHAR *event_lock_Name_;
};
/////////////////////////////////////////////////////////////////////////////

//CFileMapping
//////////////////////////////////////////////////////////////////////////
//����������� 
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
			h_file_mapping_ = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, file_mapping_name_);//����Ѿ����ڸ��ڴ�ӳ�� ���
		if (h_file_mapping_ == NULL)//�����е��ڴ�ӳ��ʧ�ܣ����Դ���
			h_file_mapping_ = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,
			PAGE_READWRITE, 0, STRUCT_SIZE, file_mapping_name_);//�����ڴ�ӳ��

		if (h_file_mapping_ != NULL)//�����ɹ�
			shared_data_ = MapViewOfFile(h_file_mapping_, FILE_MAP_ALL_ACCESS, 0, 0, STRUCT_SIZE);//���ж���ӳ��
		else//����ʧ��
		{
			init_success_ = FALSE;
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d�� �ڴ�ӳ������ʧ�ܣ����Ϊ��,������:%d", __FILE__, __LINE__, GetLastError());
			UnusualRecode(buffer);
			return;
		}

		if (shared_data_ == NULL)
		{
			init_success_ = FALSE;
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d�� MapViewOfFile()ʧ��,������:%d", __FILE__, __LINE__, GetLastError());
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
				// ж��ӳ���ڴ���ͼ.
				UnmapViewOfFile(shared_data_);
				shared_data_ = NULL;
			}
			// �ر�ӳ���ļ�����
			CloseHandle(h_file_mapping_);
			h_file_mapping_ = NULL;
		}
		delete file_mapping_name_;
		delete m_read_data_;
	}
	//���������
	BOOL ReadMapping()//��ȡӳ���ļ�
	{
		if (init_success_ == FALSE)
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d�� ��ʼ��ʧ���޷���������", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
		m_read_data_ = (Data *)shared_data_;//��ȡ�ڴ��е�����
		return TRUE;
	}
	BOOL WriteMapping(Data *m_write_data)//д��ӳ���ļ�
	{
		if (init_success_ == FALSE)
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d�� ��ʼ��ʧ���޷���������", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
		memcpy_s(shared_data_, STRUCT_SIZE, m_write_data, STRUCT_SIZE);//������д�뵽�ڴ���
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

	BOOL SetOnlineStatus(int status)//���ڸ�������״̬
	{
		if (init_success_ == FALSE)
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d�� ��ʼ��ʧ���޷���������", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
		((Data *)shared_data_)->communication_head_.status_ = status;
		return TRUE;
	}
	BOOL GetOnlineStatus(int &status)//���ڻ�ȡ��������״̬
	{
		if (init_success_ == FALSE)
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d�� ��ʼ��ʧ���޷���������", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
		status = ((Data *)shared_data_)->communication_head_.status_;//��ȡ�ڴ��е�����
		return TRUE;
	}
	Data *GetSharedData()//��ȡ��������ָ��
	{
		return (Data *)shared_data_;
	}
private:
	Data *m_read_data_;
	BOOL   init_success_;//��ʼ���ɹ���־
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
	m_filemapping_ = new CFileMapping(file_mapping_name);//��ʼ���ڴ�ӳ�����
	WCHAR w_buffer[MAX_PATH] = { 0 };
	lstrcpy(w_buffer, file_mapping_name);
	lstrcat(w_buffer, L"_client");
	client_event_lock_ = new CEventLock(w_buffer);//��ʼ���ͻ��˻������
	lstrcpy(w_buffer, file_mapping_name);
	lstrcat(w_buffer, L"_service");
	service_event_lock_ = new CEventLock(w_buffer);//��ʼ���������������
	if (m_port_ == PORT_CLIENT)
		client_thread_lock_ = new CMutexLock(L"Thread_Lock");//�߳���
	_timer = new CTimerThread();//��ʱ��
	if (m_port_ == PORT_CLIENT)
	{
		DeleteOnlineStatus(CLIENT_ALL_STATUS);
		if (client_event_lock_->GetInitSuccess())
			m_online_status |= CLIENT_CLIENT_MUTEX;//��¼��ǰ�ĳ�ʼ��״̬
		if (service_event_lock_->GetInitSuccess())
			m_online_status |= CLIENT_SERVICE_MUTEX;//��¼��ǰ�ĳ�ʼ��״̬
		if (m_filemapping_->GetInitSuccess())
			m_online_status |= CLIENT_MAPPING;//��¼��ǰ�ĳ�ʼ��״̬
		if (client_event_lock_->GetInitSuccess()
			&& service_event_lock_->GetInitSuccess()
			&& m_filemapping_->GetInitSuccess())
			AddOnlineStatus(m_online_status);//��״̬ͬ��
	}
	else
	{
		DeleteOnlineStatus(SERVER_ALL_STATUS);
		if (client_event_lock_->GetInitSuccess())
			m_online_status |= SERVICE_CLIENT_MUTEX;//��¼��ǰ�ĳ�ʼ��״̬
		if (service_event_lock_->GetInitSuccess())
			m_online_status |= SERVICE_SERVICE_MUTEX;//��¼��ǰ�ĳ�ʼ��״̬
		if (m_filemapping_->GetInitSuccess())
			m_online_status |= SERVICE_MAPPING;//��¼��ǰ�ĳ�ʼ��״̬
		if (client_event_lock_->GetInitSuccess()
			&& service_event_lock_->GetInitSuccess()
			&& m_filemapping_->GetInitSuccess())
			AddOnlineStatus(m_online_status);//��״̬ͬ��
	}
	//�����ʼ��ͼ��ʶ��ģ��
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

BOOL CProcessOfCommunication::ReadMapping()//��ȡӳ���ļ�
{
	return m_filemapping_->ReadMapping();
}

BOOL CProcessOfCommunication::WriteMapping(Data *m_write_data)//д��ӳ���ļ�
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

Data* CProcessOfCommunication::GetSharedData()//��ȡ��������ָ��
{
	if (m_filemapping_->GetInitSuccess())
		return m_filemapping_->GetSharedData();
	else
		return FALSE;
}
BOOL CProcessOfCommunication::HandscakeWait()//׼�����������������ֲ���
{
	GetOnlineStatus(m_online_status);
	if (m_port_ == PORT_CLIENT)
	{
		if ((m_online_status & CLIENT_CLIENT_MUTEX) && (m_online_status & CLIENT_SERVICE_MUTEX) && (m_online_status & CLIENT_MAPPING))
		{
			AddOnlineStatus(CLIENT_UNLOCK);
			AddOnlineStatus(CLIENT_HANDSCAKE_WAIT);
		}
		else//����ʧ��
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d�� �ͻ�������ʧ��", __FILE__, __LINE__);
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
		else//����ʧ��
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d�� ���������ʧ��", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
	}

	UINT time_m = 0;
	while (true)//�ȴ�����ˣ��ͻ��ˣ���������Э����������������
	{

		GetOnlineStatus(m_online_status);//��ȡ���ϵ�״̬
		if ((m_online_status & CLIENT_HANDSCAKE_WAIT) && (m_online_status & SERVICE_HANDSCAKE_WAIT))//���˫���������ֳɹ���������ѭ��
			break;
		Sleep(1000);
		time_m += 1000;
		if (time_m >= handscake_wait_time_)//�ȴ���ʱ
		{
			char buffer[MAX_PATH] = { 0 };
			if (m_port_ == PORT_CLIENT)
				sprintf_s(buffer, "%s__%d�� �ͻ��˵ȴ����ֳ�ʱ", __FILE__, __LINE__);
			else
				sprintf_s(buffer, "%s__%d�� ����˵ȴ����ֳ�ʱ", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
	}

	link_status = LINK_SUCCESS;//���ӳɹ�
	_timer->CreateTimer(HEARTBEAT_TIME, HeartbeatDetection, this);
	return TRUE;
}
BOOL CProcessOfCommunication::AddOnlineStatus(int status)//���ڸ�������״̬
{
	int online_status = 0;
	if (GetOnlineStatus(online_status))
	{
		online_status |= status;//�����Լ���
		m_filemapping_->SetOnlineStatus(online_status);
		return TRUE;
	}
	else
		return FALSE;
}

BOOL CProcessOfCommunication::DeleteOnlineStatus(int status)//���ڻ�ȡ��������״̬
{
	int online_status = 0;
	if (GetOnlineStatus(online_status))
	{
		online_status &= ~status;//������ɾ��
		m_filemapping_->SetOnlineStatus(online_status);
		return TRUE;
	}
	else
		return FALSE;
}

BOOL CProcessOfCommunication::GetOnlineStatus(int &status)//��ȡ����״̬
{
	if (m_filemapping_->GetInitSuccess())
	{
		m_filemapping_->GetOnlineStatus(status);
		return TRUE;
	}
	else
	{
		char buffer[MAX_PATH] = { 0 };
		sprintf_s(buffer, "%s__%d�� ��ȡ����״̬ʧ�ܣ�ʧ��ԭ��ӳ���ʼ��ʧ��", __FILE__, __LINE__);
		UnusualRecode(buffer);
		return FALSE;
	}
}

BOOL CProcessOfCommunication::CheckStatusEnable(int status, BOOL &enable)//�鿴ĳһ״̬�Ƿ�ʹ��
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
		sprintf_s(buffer, "%s__%d�� ��ȡ����״̬ʧ�ܣ�ʧ��ԭ��ӳ���ʼ��ʧ��", __FILE__, __LINE__);
		UnusualRecode(buffer);
		return FALSE;
	}
}

BOOL CProcessOfCommunication::ClientReceiveDataStatus()//�ͻ��˽�������״̬
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
			sprintf_s(buffer, "%s__%d�� �������ͼ�޸Ŀͻ���״̬", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
	}
	else
	{
		char buffer[MAX_PATH] = { 0 };
		sprintf_s(buffer, "%s__%d�� ��ȡ����״̬ʧ�ܣ�ʧ��ԭ��ӳ���ʼ��ʧ��", __FILE__, __LINE__);
		UnusualRecode(buffer);
		return FALSE;
	}
}

BOOL CProcessOfCommunication::ClientReceiveOverStatus()//�ͻ��˽����������״̬
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
			sprintf_s(buffer, "%s__%d�� �������ͼ�޸Ŀͻ���״̬", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
	}
	else
	{
		char buffer[MAX_PATH] = { 0 };
		sprintf_s(buffer, "%s__%d�� ��ȡ����״̬ʧ�ܣ�ʧ��ԭ��ӳ���ʼ��ʧ��", __FILE__, __LINE__);
		UnusualRecode(buffer);
		return FALSE;
	}
}

BOOL CProcessOfCommunication::ClientWaitResultStatus()//�ͻ��˵ȴ���������Ӧ
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
			sprintf_s(buffer, "%s__%d�� �������ͼ�޸Ŀͻ���״̬", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
	}
	else
	{
		char buffer[MAX_PATH] = { 0 };
		sprintf_s(buffer, "%s__%d�� ��ȡ����״̬ʧ�ܣ�ʧ��ԭ��ӳ���ʼ��ʧ��", __FILE__, __LINE__);
		UnusualRecode(buffer);
		return FALSE;
	}
}

BOOL CProcessOfCommunication::ServiceWaitDatrStatus()//�������ȴ�����
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
			sprintf_s(buffer, "%s__%d�� �ͻ�����ͼ�޸ķ����״̬", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
	}
	else
	{
		char buffer[MAX_PATH] = { 0 };
		sprintf_s(buffer, "%s__%d�� ��ȡ����״̬ʧ�ܣ�ʧ��ԭ��ӳ���ʼ��ʧ��", __FILE__, __LINE__);
		UnusualRecode(buffer);
		return FALSE;
	}
}

BOOL CProcessOfCommunication::ServiceRunStatus()//��������������
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
			sprintf_s(buffer, "%s__%d�� �ͻ�����ͼ�޸ķ����״̬", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
	}
	else
	{
		char buffer[MAX_PATH] = { 0 };
		sprintf_s(buffer, "%s__%d�� ��ȡ����״̬ʧ�ܣ�ʧ��ԭ��ӳ���ʼ��ʧ��", __FILE__, __LINE__);
		UnusualRecode(buffer);
		return FALSE;
	}
}

BOOL CProcessOfCommunication::ServiceRunOverStatus()//�����������������
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
			sprintf_s(buffer, "%s__%d�� �ͻ�����ͼ�޸ķ����״̬", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
	}
	else
	{
		char buffer[MAX_PATH] = { 0 };
		sprintf_s(buffer, "%s__%d�� ��ȡ����״̬ʧ�ܣ�ʧ��ԭ��ӳ���ʼ��ʧ��", __FILE__, __LINE__);
		UnusualRecode(buffer);
		return FALSE;
	}
}

BOOL  CProcessOfCommunication::WaitClientLock()//�ȴ��ͻ�������
{
	UINT wait_time = 0;
	do{//��ѭ���жϿͻ����Ƿ�����
		GetOnlineStatus(m_online_status);
		if (m_online_status & CLIENT_LOCK)//����ͻ���������
			return TRUE;
		if (client_agaim_coupling)
		{
			client_agaim_coupling = FALSE;//�ͻ����ٴ����ӣ���ֹ����
			return FALSE;
		}
		Sleep(100);
		wait_time += 100;
		if (wait_time >= WAIT_LOCK_MAX_TIME)
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d�� �ȴ��ͻ���������ʱ", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
	} while (TRUE);
	return TRUE;
}

BOOL  CProcessOfCommunication::WaitServiceLock()//�ȴ����������
{
	UINT wait_time = 0;
	do{//��ѭ���жϿͻ����Ƿ�����
		GetOnlineStatus(m_online_status);
		if (m_online_status & SERVICE_LOCK)//��������������
			return TRUE;
		Sleep(100);
		wait_time += 100;
		if (wait_time >= WAIT_LOCK_MAX_TIME)
		{
			char buffer[MAX_PATH] = { 0 };
			sprintf_s(buffer, "%s__%d�� �ȴ������������ʱ", __FILE__, __LINE__);
			UnusualRecode(buffer);
			return FALSE;
		}
	} while (TRUE);
	return TRUE;
}

BOOL CProcessOfCommunication::ServiceDispose()//�������Ĳ���
{
	if (GetOnlineStatus(m_online_status) && m_port_ == PORT_SERVICE
		&& m_online_status & SERVICE_UNLOCK && link_status == LINK_SUCCESS)//ȷ����ǰ�������ǽ���״̬
	{
		if (!ServiceWaitDatrStatus())//�������ȴ�����
			return FALSE;
		BOOL client_unlock_ = FALSE;//���ڲ鿴�ͻ����Ƿ�����
		if (!CheckStatusEnable(CLIENT_UNLOCK, client_unlock_))//�鿴�ͻ����Ƿ�����
			return FALSE;
		if (client_unlock_)//����ͻ���û�����������˲ſ�������
		{
			DWORD result = 0;
			Lock(PORT_SERVICE, result);//����˵���ֻ���ڿͻ��˲����в��ܽ���
			if (link_status != LINK_SUCCESS)//�����ͻ��˶Ͽ����ӣ�����˻���������������Ը��������������жϣ������д���
			{
				DeleteOnlineStatus(CLIENT_ONLINE);//���ͻ��˵�����״̬���
				heartbeat_delay_ = 0;
				link_status = CProcessOfCommunication::LINK_SUCCESS;
			}
		}

		if (!ServiceRunStatus())//��������������״̬
			return FALSE;
		//���е�����˵���ڿͻ����Ѿ����������
		ServiceDataDispose();//���ݴ������
		//Sleep(1000);//ģ�����ݴ���
		if (!ServiceRunOverStatus())//�����������������
			return FALSE;
		if (!WaitClientLock())//�ȴ���ʱ�򷵻�FALSE
			return FALSE;
		Unlock(PORT_CLIENT);//�⿪�ͻ��˵���
		return TRUE;
	}
	else
	{
		char buffer[MAX_PATH] = { 0 };
		if (m_port_ == PORT_CLIENT)
			sprintf_s(buffer, "%s__%d�� �������ͼʹ�ÿͻ��˲���", __FILE__, __LINE__);
		else if (m_online_status & SERVICE_LOCK)
			sprintf_s(buffer, "%s__%d�� ������Ѿ�����", __FILE__, __LINE__);
		UnusualRecode(buffer);
		return FALSE;
	}
}

BOOL CProcessOfCommunication::ClientDispose(char *filepath, DWORD wait_time)//�ͻ��˵Ĳ���
{
	DWORD result = 0;
	client_thread_lock_->Lock(wait_time, result);//Ϊ�˱�֤ͬһʱ��ֻ��һ���߳��������ͨѶ
	if (result == WAIT_TIMEOUT)//�ȴ���ʱ
	{
		ServiceDisposeError();
		goto unlock;
	}
	result = 0;
	if (GetOnlineStatus(m_online_status) && m_port_ == PORT_CLIENT
		&& m_online_status & CLIENT_UNLOCK && link_status == LINK_SUCCESS)
	{
		Data *a = m_filemapping_->GetData();
		if (!ClientReceiveDataStatus())//�ͻ��˽�������״̬
			goto unlock;
		PRINTF("��%s���������\r\n", filepath);
		ClientDataDispose(filepath);//������д�빲���ڴ���
		//Sleep(1000);
		if (!ClientReceiveOverStatus())//�ͻ��˽�������״̬�ɹ�
			goto unlock;
		if (!WaitServiceLock())//�ȴ���ʱ�򷵻�FALSE
			goto unlock;
		Unlock(PORT_SERVICE);//�⿪����˵���
		if (!ClientWaitResultStatus())//�ͻ��˵ȴ���������Ӧ
			goto unlock;
		BOOL service_unlock_ = FALSE;//���ڲ鿴�ͻ����Ƿ�����
		if (!CheckStatusEnable(SERVICE_UNLOCK, service_unlock_))//�鿴�ͻ����Ƿ�����
			goto unlock;
		if (service_unlock_)//����ͻ���û�����������˲ſ�������
		{
			Lock(PORT_CLIENT, result);//�ͻ�������
			//WAIT_OBJECT_0 �ȵ��ź��������˳�
			//WAIT_TIMEOUT �ȴ���ʱ
			if (result == WAIT_TIMEOUT)//�ȴ���ʱ���µ��˳�
			{
				//����ʵ������û�д��������ݵģ���Ϊ�ȴ���ʱ���Ե��µ��˳�
				PRINTF("�ȴ���ʱ,����δ����\r\n");
				ServiceDisposeError();//������������д���
			}
		}

		client_thread_lock_->Unlock();
		return TRUE;
	}
	else if (link_status != LINK_SUCCESS)//�����ж����ӵ��µ��޷�����
	{//ͨ��������ʽ���д���
		PRINTF("�ж�����%s\r\n", filepath);
		ServiceDisposeError();
	}
	else
	{
		char buffer[MAX_PATH] = { 0 };
		if (m_port_ == PORT_CLIENT)
			sprintf_s(buffer, "%s__%d�� �ͻ�����ͼʹ�÷���˲���", __FILE__, __LINE__);
		else if (m_online_status & CLIENT_LOCK)
			sprintf_s(buffer, "%s__%d�� �ͻ����Ѿ�����", __FILE__, __LINE__);
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
	//	PRINTF("������ͼƬ\r\n");
	//}
	//else if (prediction < 0.8)
	//{
	//	send_data_->communication_head_.server_to_client_.result_ =
	//		Data::CommunicationData::ServerToClient::RESULT_SEX;
	//	PRINTF("���Ը�ͼƬ\r\n");
	//}
	//else
	//{
	//	send_data_->communication_head_.server_to_client_.result_ =
	//		Data::CommunicationData::ServerToClient::RESULT_PORNOGRAPHIC;
	//	PRINTF("��ɫ��ͼƬ\r\n");
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
	//sstr >> send_data_->communication_head_.client_to_server_.file_name_;//���ļ�·������

	strcpy_s(send_data_->communication_head_.client_to_server_.file_name_, filepath);
	// Ҫ���������ļ���������ö����ƴ�   
	filestr.open(filepath, ios::binary);
	// ��ȡfilestr��Ӧbuffer�����ָ��   
	pbuf = filestr.rdbuf();
	// ����buffer���󷽷���ȡ�ļ���С  
	send_data_->communication_head_.client_to_server_.data_size_ = pbuf->pubseekoff(0, ios::end, ios::in);//д���ļ���С
	pbuf->pubseekpos(0, ios::in);
	// �����ڴ�ռ�  
	// ��ȡ�ļ����� 
	pbuf->sgetn(send_data_->communication_head_.client_to_server_.data_,
		send_data_->communication_head_.client_to_server_.data_size_);//д���ļ�����
	filestr.close();

	return true;
}

BOOL CProcessOfCommunication::ServiceDisposeError()
{
	static int i = 0;
	PRINTF("���ڵȴ���ʱ���޷�����:%d\r\n", i++);
	return TRUE;
}
////////////////////////////////////////////////////////////////////////////////////////