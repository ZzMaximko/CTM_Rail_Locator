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
#include <vector>
//---------------------------------------------------------------------------
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

class RLData
{
	public:
		int track_id;     //номер трека
		int status_code;  //возвращаемый код ответа на запрос относительно одного контейнера
		String msg;       //описание ошибки
		RLData()
		{
			track_id = 0;
			status_code = 0;
		}
		virtual ~RLData(){};
};

//Баланс аккаунта
class RLBalanceData : public RLData     //[get]
{
	public:
		float balance;      //текущий баланс
};

//Списания по контейнерам
class RLDecreaseData : public RLData   //[get]
{
	public:
		String vagen_num;   //номер контейнера
		String nss;         //НСС
		float balance;      //текущий баланс
		float cost_ops;     //стоимость операции
		TDate insertred_at; //дата списания
};

//Список активных контейнеров находящихся на слежении
class RLListContainerTrackingData : public RLData  //[get]
{
	public:
		String vagen_num;       //номер контейнера
		int track_descr;    	//тип запроса (код страны)
		TDate track_create;     //дата первой постановки на слежение
		TDate date_of_update;   //дата последнего обновления
		String comment;         //комментарий
};

//Снятие со слежения
class RLContainerDeletedData : public RLData      //[delete] params { "track_ids": "3*****4;3*****5" }   version 1.0
{                                  				  //[delete] params {"data": [{"status_code": 204,"track_id": 123456},{"status_code": 403,"track_id": 123457}]} version 2.0
	public:
		String vagen_num;
};

//постановка на суточное слежение
class RLTrackingDailyData : public RLData        //[post] {"track_type":"299","vagens": "TKRU######8,01;TKRU######7,01"}
{
	public:
		String vagen_num;
};

class RLExtendedTrackingData : public RLData
{
	public:
		String cont_num;     //номер контейнера
		TDate datearrive; // Дата факт. прибытия на ст.
		TDate approx_date_of_arrival; // Ориентировочное прибытие
		int rest_range; // Оставшееся растояние км
		int rest_days;  // Оставшиеся дни в пути
		TDate cont_load_to_wagon_date; // Дата погрузки контейнера на вагон-платформу
		TDate date_of_departure;// Дата отправления
		int passed_distance;
		int distance;
		TDate date_rec; // Дата постановки на слежение
		String train_num; // номер поезда
		//float cargo_weight;
		String dest_point_name; // станция назначения
		String from_station;    // станция отправления
        String name_station;    // текущая станция операции
		String platform_number;		// номер платформы
};

//Ответ на запрос
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
		virtual ~RLResponse()
		{
			ClearData();
		}
		void ClearData();
};

struct RLContainerItem
{
	String track_type;
	String is_cross_country_tracking;
	String railway_code;
	String platform_number;
	String container_name;
	String vagens;
	RLContainerItem()
	{
		track_type = "";
		is_cross_country_tracking = "";
		railway_code = "";
		platform_number = "";
		container_name = "";
		vagens = "";
	}
	RLContainerItem(String trackType, String isCrossCountryTracking, String railwayCode, String platformNumber, String containerName) :
	track_type(trackType), is_cross_country_tracking(isCrossCountryTracking), railway_code(railwayCode), platform_number(platformNumber), container_name(containerName)
	{
		vagens = containerName + "," + railwayCode;
	}
};

class RLContainerItemCollection
{
	private:
		std::vector<RLContainerItem*> items;
		std::vector<RLContainerItem*>::iterator it;
		RLContainerItem* GetItem(unsigned index)
		{
			if (items.size() < index + 1)
				return NULL;
			return items.at(index);
		}
	public:
		int Count()
		{
			return items.size();
		}
		__property RLContainerItem* Items[unsigned Index] = {read=GetItem};
		RLContainerItem* operator[](unsigned Index)
		{
			return this->Items[Index];
		}
		RLContainerItemCollection(){};
		virtual ~RLContainerItemCollection()
		{
			Clear();
		}
		void AddItem(RLContainerItem *item);
		void AddItem(String trackType, String isCrossCountryTracking, String railwayCode, String platformNumber, String containerName);
		void Clear();
		void AddItems(std::vector<RLContainerItem*> &items);
};

//Тело запроса
class RLRequest
{
	public:
		RLContainerItemCollection *containerItemCollection;
		String track_ids;
		int last;
		RLType reqType;
		int version;
		RLRequest(RLType rt)
		{
            containerItemCollection = NULL;
			reqType = rt;
			last = 1;
			track_ids = "";
			version = 1;
		}
		virtual ~RLRequest()
		{
			if (containerItemCollection != NULL)
			{
				delete containerItemCollection;
                containerItemCollection = NULL;
			}
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
        bool CheckResponseCode(int code);
	protected:
		String userName;
		String userPassword;
		int version;
	public:
		RLType rType;
		RLResponse *response;
		RailLocatorTools(String userName, String userPassword, int version)
		{
			this->userName	   = userName;
			this->userPassword = userPassword;
			this->rType 	   = RLType::NONE;
			this->version      = version;
			this->request      = NULL;
			this->response     = NULL;
		}
		RailLocatorTools(String userName, String userPassword, RLType rType, int version = 1)
		{
			this->userName	   = userName;
			this->userPassword = userPassword;
			this->rType 	   = rType;
			this->version      = version;
            this->request      = NULL;
			this->response     = NULL;
		}
		virtual ~RailLocatorTools()
		{
			if (request != NULL)
			{
				delete request;
				request = NULL;
			}
			if (response != NULL)
			{
				delete response;
                response = NULL;
			}
		}
		bool GetResponseInfo(String track_ids = "", RLContainerItemCollection *containerItemCollection = NULL);
		bool GetMultipleExtendedData(TStringList *trackIds);
		bool DeleteMultipleData(TStringList *trackIds);
        bool SetMultipleTrackingData(RLContainerItemCollection *containerItemCollection);
		//bool SetMultipleDataTrackingDaily(String track_type, TStringList *containerNames, TStringList *railwayCodes);
};


#endif
