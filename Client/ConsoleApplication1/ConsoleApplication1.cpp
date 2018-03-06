// ConsoleApplication1.cpp : 定义控制台应用程序的入口点。
//
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <vector>
#include <string>
#include <direct.h>
#include <iostream>  
#include <fstream>  
#include <process.h>
#include "io.h"
#include <sstream>
#include <Shlwapi.h>
#include "process_of_communication.h"
#pragma comment(lib,"shlwapi.lib") 
using namespace std;



void cf_findFileFromDir(string mainDir, vector<string> &files)
{
	files.clear();
	const char *dir = mainDir.c_str();
	_chdir(dir);
	intptr_t hFile;
	_finddata_t fileinfo;

	if ((hFile = _findfirst("*.*", &fileinfo)) != -1)
	{
		do
		{
			if ((fileinfo.attrib & _A_SUBDIR))//找到文件夹  
			{
				if (strcmp(fileinfo.name, ".") != 0 && strcmp(fileinfo.name, "..") != 0)
				{
					char subdir[_MAX_PATH];
					strcpy_s(subdir, dir);
					strcat_s(subdir, "\\");
					strcat_s(subdir, fileinfo.name);
					string temDir = subdir;
					vector<string> temFiles;
					cf_findFileFromDir(temDir, temFiles);
					for (vector<string>::iterator it = temFiles.begin(); it < temFiles.end(); it++)
					{
						string str = *it;
						if (strcmp(PathFindExtensionA(str.c_str()), "png") ||
							strcmp(PathFindExtensionA(str.c_str()), "jpg"))
							files.push_back(*it);
					}
				}
			}
			else//直接找到文件  
			{
				char filename[_MAX_PATH];
				strcpy_s(filename, dir);
				strcat_s(filename, "\\");
				strcat_s(filename, fileinfo.name);
				string temfilename = filename;
				if (strcmp(PathFindExtensionA(temfilename.c_str()), "png") ||
					strcmp(PathFindExtensionA(temfilename.c_str()), "jpg"))
					files.push_back(temfilename);
			}
		} while (_findnext(hFile, &fileinfo) == 0);
		_findclose(hFile);
	}
}

//
//UINT  __stdcall DownBindSoftThreade(LPVOID prm)
//{
//
//	string filepath = *((string *)prm);
//	filebuf *pbuf;
//	ifstream filestr;
//	Data *send_data_ = new Data;
//	char * filebuffer = (char *)send_data_;
//	ZeroMemory(filebuffer, STRUCT_SIZE);
//
//	stringstream sstr;
//	sstr.clear();
//	sstr << filepath;
//	sstr >> send_data_->communication_head_.client_to_server_.file_name_;//将文件路径保存
//
//	// 要读入整个文件，必须采用二进制打开   
//	filestr.open(filepath, ios::binary);
//	// 获取filestr对应buffer对象的指针   
//	pbuf = filestr.rdbuf();
//	// 调用buffer对象方法获取文件大小  
//	send_data_->communication_head_.client_to_server_.data_size_ = pbuf->pubseekoff(0, ios::end, ios::in);
//	pbuf->pubseekpos(0, ios::in);
//	// 分配内存空间  
//	// 获取文件内容  
//	pbuf->sgetn(send_data_->communication_head_.client_to_server_.file_name_ + NAME_LENG,
//		send_data_->communication_head_.client_to_server_.data_size_);
//	filestr.close();
//
//	a.WriteMapping(send_data_);
//	//memcpy_s(a., VIEW_SIZE, filebuffer, VIEW_SIZE);
//	//getchar();
//	delete[]filebuffer;
//	return true;
//}

struct thread
{
	CProcessOfCommunication *process;
	char buffer[MAX_PATH];
};
UINT __stdcall Event_ThreadProc2(LPVOID pVoid)
{
	thread *_t = (thread *)pVoid;
	CProcessOfCommunication *process = _t->process;
	process->ClientDispose(_t->buffer);
	return 0;
}



int _tmain(int argc, _TCHAR* argv[])
{

	CProcessOfCommunication a(L"fanhuangzhidun", CProcessOfCommunication::PORT_CLIENT);
	if (a.HandscakeWait())
	{
		char buffer[MAX_PATH] = { 0 };
		string filepath = "";
		ZeroMemory(buffer, MAX_PATH);
		GetModuleFileNameA(NULL, buffer, MAX_PATH);
		PathRemoveFileSpecA(buffer);
		PathAppendA(buffer, "TestImage");
		filepath = buffer;
		vector<string> all_files;

		cf_findFileFromDir(filepath.c_str(), all_files);

		HANDLE *handle = NULL;
		handle = new HANDLE[all_files.size()];
		//while (true)
		{
			for (int i = 0; i < all_files.size(); i++)
			{
				thread *_t = new thread;
				strcpy_s(_t->buffer, (char *)all_files[i].c_str());
				_t->process = &a;
				handle[i] = (HANDLE)::_beginthreadex(NULL, 0, Event_ThreadProc2, (LPVOID)_t, 0, NULL);


				//a.ClientDispose((char *)all_files[i].c_str());
			}
			//WaitForMultipleObjects(all_files.size(), handle, FALSE, INFINITE);
		}





	}
	getchar();
	return 0;
}
//
//
//#include <WINDOWS.H>
//#include <stdio.h>
//#include <tchar.h>
//#include <Shlwapi.h>
////#include "process_of_communication.h"
//#include "../../file/process_of_communication.h"
//#pragma comment(lib,"shlwapi.lib") 
//
//
//int _tmain(int argc, _TCHAR* argv[])
//{
//
//	CProcessOfCommunication a(L"fanhuangzhidun", CProcessOfCommunication::PORT_SERVICE);
//	if (a.HandscakeWait())//双方握手成功之后才可以通讯
//	{
//		while (true)
//		{
//			a.ServiceDispose();
//		}
//	}
//
//}

