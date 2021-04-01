//---------------------------------------------------------------------------

#ifndef RailLocatorToolsH
#define RailLocatorToolsH
#include <System.Classes.hpp>
#include <IdMultiPartFormData.hpp>
#include <IdHTTP.hpp>
#include <IdTCPClient.hpp>
#include <IdTCPConnection.hpp>
#include <IdIOHandler.hpp>
#include <IdIOHandlerSocket.hpp>
#include <IdIOHandlerStack.hpp>
#include <IdSSL.hpp>
#include <IdSSLOpenSSL.hpp>
#include <DBXJSON.hpp>
#include <process.h>
//---------------------------------------------------------------------------

class TLocalHTTP : public TIdHTTP
{
	public:
		void __fastcall DeleteRequest(String request, TStringStream *params, TStringStream *response)
        {
			DoRequest("DELETE", request, params, response, NULL, -1);
		}
};


enum class RLType
{
	NONE,
	BALANCE,
	DECREASE,
	LIST_CONTAINER_TRACKING,
	CONTAINER_DELETE,
	CONTAINER_DATA,
	CONTAINER_EXTENDED_DATA,
	OPERATIONAL_TRACKING,
	REGULATION_TRACKING_DAILY,
	REGULATION_TRACKING_ROUTE
};


//������ ��������
class RLBalanceData     //[get]
{
	public:
		float balance;      //������� ������
};

//�������� �� �����������
class RLDecreaseData    //[get]
{
	public:
		int track_id;       //����� �����
		String vagen_num;   //����� ����������
		String nss;         //���
		float balance;      //������� ������
		float cost_ops;     //��������� ��������
		TDate insertred_at; //���� ��������
};

//������ �������� ����������� ����������� �� ��������
class RLListContainerTrackingData  //[get]
{
	public:
		int track_id;           //����� �����
		String vagen_num;       //����� ����������
		int track_descr;    	//��� ������� (��� ������)
		TDate track_create;     //���� ������ ���������� �� ��������
		TDate date_of_update;   //���� ���������� ����������
		String comment;         //�����������
};

//������ �� ��������
class RLContainerDeletedData       //[delete] params { "track_ids": "3*****4;3*****5" }
{
	public:
		int track_id;
		String vagen_num;
};

//���������� �� �������� ��������
class RLTrackingDailyData         //[post] {"track_type":"299","vagens": "TKRU######8,01;TKRU######7,01"}
{
	public:
		int track_id;
		String vagen_num;
};

class RLExtendedTrackingData
{
	public:
		int track_id;         //����� �����
		String cont_num;     //����� ����������
		TDate datearrive; // ���� ����. �������� �� ��.
		TDate approx_date_of_arrival; // ��������������� ��������
		int rest_range; // ���������� ��������� ��
		int rest_days;  // ���������� ��� � ����
		TDate cont_load_to_wagon_date; // ���� �������� ���������� �� �����-���������
		TDate date_of_departure;// ���� �����������
		int passed_distance;
		int distance;
		TDate date_rec; // ���� ���������� �� ��������
		String train_num;
        //float cargo_weight;
};

//����� �� ������
class RLResponse
{
	public:
		String result;
		int dataCount;
		void *Data;
		RLType respType;
		String responseBody;
		RLResponse(RLType rt)
		{
			Data = NULL;
			respType = rt;
			result = "";
			dataCount = 0;
		}
		~RLResponse()
		{
			if (dataCount > 0 && Data != NULL)
			{
				delete[] Data;
			}
			if (Data != NULL && dataCount == 0)
			{
				delete Data;
			}
		}
};

//���� �������
class RLRequest
{
	public:
		String track_ids;
		String track_type;
		String vagens;
		int last;
		RLType reqType;
		RLRequest(RLType rt)
		{
			reqType = rt;
			last = 1;
			track_ids = "";
			track_type = "";
			vagens = "";
		}
		void SetHeaders(TStringList *params);

};

class RailLocatorTools
{
	private:
		RLRequest *request;
		String GetResponse(TStringList *params, TIdHTTP *http, String &request);
		String GetTracksToString(TStringList *trackIds);
		String GetVagensToString(TStringList *containerNames, TStringList *railwayCodes);
	protected:
		String userName;
		String userPassword;
		
	public:
		RLType rType;
		RLResponse *response;
		RailLocatorTools(String userName, String userPassword)
		{
			this->userName	   = userName;
			this->userPassword = userPassword;
			this->rType 	   = RLType::NONE;
		}
		RailLocatorTools(String userName, String userPassword, RLType rType)
		{
			this->userName	   = userName;
			this->userPassword = userPassword;
			this->rType 	   = rType;
		}
		~RailLocatorTools()
		{   /*
			if (request != NULL)
			{
				delete request;
			}
			if (response != NULL)
			{
				delete response;
			}    */
		}
		bool GetResponseInfo(String track_ids = "", String containerName = "", String track_type = "", String railwayCode = "");
		bool GetMultipleExtendedData(TStringList *trackIds);
		bool DeleteMultipleData(TStringList *trackIds);
		//bool SetMultipleDataTrackingDaily(String track_type, TStringList *containerNames, TStringList *railwayCodes);
};


#endif
