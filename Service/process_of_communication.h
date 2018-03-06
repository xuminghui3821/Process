#pragma  once
#ifndef _process_of_communication_H  
#define _process_of_communication_H

#define STRUCT_SIZE  2097152 //���ݽ����ṹ���ܴ�С  
#define DATA_MAX_SIZE 2000000//�����ļ�������С
#define NAME_LENG  1024//�ļ�·������С
#define WAIT_LOCK_MAX_TIME  1000000//���ڼ�¼�ȴ��������ʱ�䵥λ����
#define HEARTBEAT_TIME      1000 // �������ʱ��

#define CLIENT_MAPPING				0x00001//�ͻ����ڴ�ӳ���ʼ�����
#define SERVICE_MAPPING				0x00002//�������ڴ�ӳ���ʼ�����
#define CLIENT_CLIENT_MUTEX			0x00004//�ͻ��˵Ŀͻ���������ʼ�����
#define CLIENT_SERVICE_MUTEX		0x00008//�ͻ��˵ķ��񻥳�����ʼ�����
#define SERVICE_CLIENT_MUTEX		0x00010//����˵Ŀͻ���������ʼ�����
#define SERVICE_SERVICE_MUTEX		0x00020//����˵ķ��񻥳�����ʼ�����
#define CLIENT_HANDSCAKE_WAIT       0x00040//�ͻ������ֵȴ�
#define SERVICE_HANDSCAKE_WAIT 		0x00080//���������ֵȴ�

#define CLIENT_RECE_DATA            0x00100//�ͻ��˽�������
#define CLIENT_RECE_OVER			0x00200//�ͻ������ݽ������
#define CLIENT_WAIT_RESULT          0x00400//�ͻ������ݽ�������ѽ�����д���ڴ�ӳ���У��ȴ��������������
#define SERVICE_WAIT_DATA           0x00800//�������ȴ�����
#define SERVICE_RUN                 0x01000//��������������
#define SERVICE_RUN_OVER            0x02000//���������ݴ������
#define CLIENT_UNLOCK               0x04000//�ͻ��˽���״̬
#define CLIENT_LOCK                 0x08000//�ͻ�������״̬

#define SERVICE_UNLOCK              0x10000//����������״̬
#define SERVICE_LOCK                0x20000//����������״̬
#define SERVICE_ONLINE              0x40000//���������ߣ�����������֮ǰ����ȷ�ϣ���Ȼ����ɽ�������
#define CLIENT_ONLINE               0x80000//�ͻ�������,ͬ��
#define SERVER_ALL_STATUS           0x738B2//���еķ��������
#define CLIENT_ALL_STATUS           0x8C74D//���еĿͻ�������


#define DISCERN  0//��������CProcessOfCommunication����ʹ��Classifier
#define CONSOLE  1//��ǰ�Ƿ��ڿ���̨������

#if CONSOLE
#define  PRINTF(...) printf(__VA_ARGS__)
#else
#define  PRINTF(...)
#endif
#pragma pack(1) //�ñ�����������ṹ��1�ֽڶ���
union Data
{
	struct CommunicationData//����˫��ͨѶ������
	{
		int status_;//��ǰ��״̬
		struct ClientToServer//�ͻ��˴�����������Ҫ���������
		{
			unsigned int data_size_;//���ڼ�¼���ݴ�С
			char  file_name_[NAME_LENG];//���ڼ�¼�ļ���
			char  data_[DATA_MAX_SIZE];//�������ݵ��������
		}client_to_server_;
		struct ServerToClient//����������������֮�������ص�����
		{
			enum RESULT//����Ľ��
			{
				RESULT_UNDISPOSED = 0,//δ����״̬
				RESULT_ERROR,//����������
				RESULT_NORMAL,//����
				RESULT_SEX,//�Ը�
				RESULT_PORNOGRAPHIC//ɫ��
			};
			RESULT result_;//��Ŵ�����
		}server_to_client_;
	}communication_head_;
	char StructSize_[STRUCT_SIZE];//���ڼ�¼�ýṹ�Ĵ�С
};

class CEventLock;//�¼���
class CMutexLock;//�¼���
class CFileMapping;//�ڴ�ӳ����
class CTimerThread;
#if DISCERN
class Classifier;
namespace cv { class Mat; }
#endif







class CProcessOfCommunication//����ͨѶ��
{

public:	
	enum PORT//��ǰ�ĳ���ʱ�ͻ��˻��Ƿ����
	{
		PORT_CLIENT,//�ͻ���
		PORT_SERVICE//�����
	};
	enum LINK_STATUS
	{
		WAIT_HANDSCAKE,//�ȴ�����
		CLIENT_DISCONNECT,//�ͻ��˶Ͽ�����
		SERVICE_DISCONNECT,//�������Ͽ�����
		LINK_SUCCESS//���ӳɹ�
	};
	CProcessOfCommunication(WCHAR *file_mapping_name, PORT port, unsigned int handscake_wait_time = 0xFFFFFFFF);
	//��һ���������ڴ�ӳ��ͻ�����������
	//�ڶ���������ָ��ǰ�Ķ��ǿͻ��˻��Ƿ����
	//����������������Э��ȴ�ʱ��Ĭ����һֱ�ȴ�
	~CProcessOfCommunication();
//���������
	BOOL Lock(PORT port, DWORD &result, DWORD wait_time = INFINITE);
	BOOL Unlock(PORT port);
	BOOL ReadMapping();//��ȡӳ���ļ�
	BOOL WriteMapping(Data *m_write_data);//д��ӳ���ļ�
	Data *GetSharedData();//��ȡ��������ָ��
	Data *GetData();//��ȡ���ݽṹ��
	BOOL HandscakeWait();//׼�����������������ֲ���

	//״̬��صĲ���
	BOOL AddOnlineStatus(int status);//���ڸ�������״̬
	BOOL DeleteOnlineStatus(int status);//���ڻ�ȡ��������״̬
	BOOL GetOnlineStatus(int &status);//��ȡ���ϵ�״̬
	BOOL CheckStatusEnable(int status,BOOL &enable);//�鿴ĳһ״̬�Ƿ�ʹ��
	BOOL ClientReceiveDataStatus();//�ͻ��˽�������״̬
	BOOL ClientReceiveOverStatus();//�ͻ��˽����������״̬
	BOOL ClientWaitResultStatus();//�ͻ��˵ȴ���������Ӧ
	BOOL ServiceWaitDatrStatus();//�������ȴ�����
	BOOL ServiceRunStatus();//��������������
	BOOL ServiceRunOverStatus();//�����������������

	//�ȴ��ͻ���/���������
	BOOL WaitClientLock();//�ȴ��ͻ�������
	BOOL WaitServiceLock();//�ȴ����������

	//�÷������Ϳͻ��˵Ĳ�������һ��һ��ķ�ʽ���� �ɸ���ʵ����������޸�
	BOOL ServiceDispose();//�������Ĳ���
	BOOL ClientDispose(char *filepath, DWORD wait_time = INFINITE);//�ͻ��˵Ĳ���

	//����ĺ��������ݴ�����صĲ���
	BOOL ServiceDataDispose();
	BOOL ClientDataDispose(char *filepath);
	BOOL ServiceDisposeError();//������δ��ͨ�������������д���
public:
	int m_online_status;//���ڱ����ȡ���ϵ�״̬
	PORT m_port_;//��ǰ�Ķ˿�
	CEventLock *service_event_lock_;//����˻��������÷���Ϊ�ڷ�����������ڿͻ��˽���
	CEventLock *client_event_lock_;//�ͻ��˻��������÷���Ϊ�ٿͻ����������ڿͻ��˽���
	CMutexLock *client_thread_lock_;//�ڿͻ��˴�������ʱ����֤ͬһʱ��ֻ��һ���ͻ����������ͨѶ
	CFileMapping *m_filemapping_;
	UINT handscake_wait_time_;//���ֵȴ�ʱ��
	LINK_STATUS link_status;//����״̬
	CTimerThread *_timer;
	int heartbeat_delay_;//�����ӳ٣��������һ����֮�����Ӳ��϶Զ�����Ϊ�Ͽ�����
	BOOL client_agaim_coupling;//�ͻ����������ӣ���Ҫ���ڽӴ��������ȴ��ͻ�����������
#if DISCERN
	cv::Mat *img;
	Classifier *classifier;
#endif
};



#endif