//---------------------------------------------------------------------------

#pragma hdrstop

#include "RailLocatorTools.h"
#include <IdTCPClient.hpp>
#include <IdTCPConnection.hpp>
#include <IdIOHandler.hpp>
#include <IdIOHandlerSocket.hpp>
#include <IdIOHandlerStack.hpp>
#include <IdSSL.hpp>
#include <IdSSLOpenSSL.hpp>
#include <DBXJSON.hpp>
#include <IdURI.hpp>
#include <algorithm>
#include <CustomIdHTTP.h>
//---------------------------------------------------------------------------
#pragma package(smart_init)

void RLResponse::ClearData()
{
	switch(this->respType)
	{
		case RLType::BALANCE:
		{
			RLBalanceData *t_data = static_cast<RLBalanceData*>(Data);
			if (t_data != NULL && dataCount == 0)
				delete t_data;
			else
			if (dataCount > 0 && t_data != NULL)
				delete[] t_data;
			break;
		}
		case RLType::DECREASE:
		{
			RLDecreaseData *t_data = static_cast<RLDecreaseData*>(Data);
			if (t_data != NULL && dataCount == 0)
				delete t_data;
			else
			if (dataCount > 0 && t_data != NULL)
				delete[] t_data;
			break;
		}
		case RLType::LIST_CONTAINER_TRACKING:
		{
			RLListContainerTrackingData *t_data = static_cast<RLListContainerTrackingData*>(Data);
			if (t_data != NULL && dataCount == 0)
				delete t_data;
			else
			if (dataCount > 0 && t_data != NULL)
				delete[] t_data;
			break;
		}
		case RLType::CONTAINER_DELETE:
		{
			RLContainerDeletedData *t_data = static_cast<RLContainerDeletedData*>(Data);
			if (t_data != NULL && dataCount == 0)
				delete t_data;
			else
			if (dataCount > 0 && t_data != NULL)
				delete[] t_data;
			break;
		}
		case RLType::CONTAINER_EXTENDED_DATA:
		{
			RLExtendedTrackingData *t_data = static_cast<RLExtendedTrackingData*>(Data);
			if (t_data != NULL && dataCount == 0)
				delete t_data;
			else
			if (dataCount > 0 && t_data != NULL)
				delete[] t_data;
			break;
		}
		case RLType::REGULATION_TRACKING_DAILY:
		{
			RLTrackingDailyData *t_data = static_cast<RLTrackingDailyData*>(Data);
			if (t_data != NULL && dataCount == 0)
				delete t_data;
			else
			if (dataCount > 0 && t_data != NULL)
				delete[] t_data;
			break;
		}
	}
	this->Data = NULL;
}

void RLRequest::SetHeaders(TStringList *params)
{
	if (version == 1)
	{
		switch(reqType)
		{
			case RLType::REGULATION_TRACKING_DAILY:
			{
				if (
					this->containerItemCollection &&
					this->containerItemCollection->Count() > 0 &&
					this->containerItemCollection->Items[0]->track_type != "" &&
					this->containerItemCollection->Items[0]->vagens != ""
				   )
				{
					params->Text = "{\"track_type\": \""+this->containerItemCollection->Items[0]->track_type+"\",\"vagens\":\""+this->containerItemCollection->Items[0]->vagens+"\"}";
				}
				break;
			}
			case RLType::CONTAINER_DELETE:
			{
				if (track_ids != "")
				{
					params->Text = "{\"track_ids\": \""+track_ids+"\"}";
				}
				break;
			}
			case RLType::CONTAINER_EXTENDED_DATA:
			{
				if (track_ids != "" && last != 0)
				{
					params->Text = "{\"track_ids\": \""+track_ids+"\",\"last\":"+IntToStr(last)+"}";
				}
				break;
			}
		}
	}
	else
	{
		switch(reqType)
		{
			case RLType::REGULATION_TRACKING_DAILY:
			{
				String headerArray = "";
				for (unsigned i = 0; i < this->containerItemCollection->Count(); i++)
				{
					if (
						this->containerItemCollection &&
						this->containerItemCollection->Count() > 0 &&
						this->containerItemCollection->Items[i]->container_name != "" &&
						this->containerItemCollection->Items[i]->track_type != "" &&
						this->containerItemCollection->Items[i]->railway_code != "" &&
						this->containerItemCollection->Items[i]->is_cross_country_tracking != ""
					   )
					{
						if (headerArray != "")
							headerArray += ",";

						if (headerArray.Pos("[") == 0)
							headerArray += "[ ";

						headerArray += L"  { "
									   L"	 \"is_validate_vehicle\": true, "
									   L"	 \"cargo_transport_unit_number\": \"" + this->containerItemCollection->Items[i]->container_name + "\", "
									   L"	 \"tracking_type\": " + this->containerItemCollection->Items[i]->track_type + ", "
									   L"    \"current_location_iso_country_code\": \"" + this->containerItemCollection->Items[i]->railway_code + "\", "
									   L"	 \"is_cross_country_tracking\": " + this->containerItemCollection->Items[i]->is_cross_country_tracking + " "
									   L" " + ((!this->containerItemCollection->Items[i]->platform_number.IsEmpty()) ? String(", \"platform_number\": \"" + this->containerItemCollection->Items[i]->platform_number + "\"") : String(""))+
									   L"  } ";
					}
				}
				if (headerArray != "")
					headerArray += " ]";
				params->Text = headerArray;
				break;
			}
			case RLType::CONTAINER_DELETE:
			{
				if (track_ids != "")
				{
					params->Text = "{\"track_ids\": ["+track_ids+"]}";
				}
				break;
			}
			case RLType::CONTAINER_EXTENDED_DATA:
			{
				if (track_ids != "")
				{
					params->Text = "{\"track_ids\": ["+track_ids+"],\"operations_count\": 1}";
				}
				break;
			}
		}
	}
}

void RLContainerItemCollection::AddItem(RLContainerItem *item)
{
	items.push_back(item);
}

void RLContainerItemCollection::AddItem(String trackType, String isCrossCountryTracking, String railwayCode, String platformNumber, String containerName)
{
	RLContainerItem *item = new RLContainerItem(trackType, isCrossCountryTracking, railwayCode, platformNumber, containerName);
	items.push_back(item);
}

void RLContainerItemCollection::Clear()
{
	std::for_each(items.begin(), items.end(), std::default_delete<RLContainerItem>());
	items.clear();
}

void RLContainerItemCollection::AddItems(std::vector<RLContainerItem*> &items)
{
	Clear();
	this->items = std::move(items);
}

String RailLocatorTools::GetResponse(TStringList *params, TIdHTTP *http, String &request)
{
	String response = "";
	TStringStream *contentStream = new TStringStream("", TEncoding::UTF8, true);
	TStringStream *paramStream   = new TStringStream("", TEncoding::UTF8, true);
	try
	{
		try
		{
			switch(rType)
			{
				case RLType::BALANCE:
				{
					request += "balance";
					response = http->Get(request);
					break;
				}
				case RLType::DECREASE:
				{
					request += "decrease-history";
					response = http->Get(request);
					break;
				}
				case RLType::LIST_CONTAINER_TRACKING:
				{
					request += "tracks";
					response = http->Get(request);
					break;
				}
				case RLType::CONTAINER_DELETE:
				{
					request += "tracks";
					paramStream->WriteString(params->Text);
					//http->HTTPOptions = (http->HTTPOptions << hoKeepOrigProtocol);
					TCustomIdHTTP *httpAccess = (TCustomIdHTTP*)http;
					httpAccess->DeleteRequest(request, paramStream, contentStream);
					//http->Delete(request, contentStream);
					response = contentStream->DataString;
					break;
				}
				case RLType::CONTAINER_EXTENDED_DATA:
				{
					if (version == 1)
					{
						request += "tracks/extended-data";
						http->Post(request, params, contentStream);
					}
					else
					{
						request += "tracks/dislocation/history";
						paramStream->WriteString(params->Text);
						TCustomIdHTTP *httpAccess = (TCustomIdHTTP*)http;
						httpAccess->GetParamRequest(request, paramStream, contentStream);
					}

					response = contentStream->DataString;
					break;
				}
				case RLType::REGULATION_TRACKING_DAILY:
				{
					if (version == 1)
						request += "tracks";
					else
						request += "tracks/dislocation";
					http->Post(request, params, contentStream);
					response = contentStream->DataString;
					break;
				}
			}
		}
		catch(EIdHTTPProtocolException *ex)
		{
			throw EIdHTTPProtocolException(TIdURI::URLDecode(ex->ErrorMessage));
		}
	}
	__finally
	{
		if (contentStream)
			delete contentStream;
		if (paramStream)
			delete paramStream;
	}

	return response;
}

bool RailLocatorTools::CheckResponseCode(int code)
{
	if (this->version == 1)
		return (code == 200);
	else
		return (code == 207);
}

bool RailLocatorTools::GetResponseInfo(String track_ids, RLContainerItemCollection *containerItemCollection)
{
	TFormatSettings fs;
	fs.DateSeparator = '-';
	fs.ShortDateFormat = "yyyy-mm-dd";
	fs.TimeSeparator = ':';
	fs.ShortTimeFormat = "hh:mm";
	fs.LongTimeFormat = "hh:mm:ss";

	TJSONObject *json = NULL;
	TIdHTTP *http = NULL;
	TIdSSLIOHandlerSocketOpenSSL *ssl = NULL;
	TIdCookieManager * cookieMgr = NULL;
	//TIdMultiPartFormDataStream *params = NULL;
	TStringList *params = NULL;
	String tracks_id = "";
	if (this->request)
	{
		delete this->request;
		this->request = NULL;
	}
	if (this->response)
	{
		delete this->response;
		this->response = NULL;
	}
	this->request = new RLRequest(this->rType);
	this->response = new RLResponse(this->rType);
	try
	{
		try
		{
			http = new TIdHTTP(NULL);
			ssl = new TIdSSLIOHandlerSocketOpenSSL(NULL);
			cookieMgr = new TIdCookieManager(NULL);
			//params = new TIdMultiPartFormDataStream();
			params = new TStringList();
			http->CookieManager = cookieMgr;
			http->AllowCookies = true;
			http->HandleRedirects = true;
			http->ConnectTimeout = 40000;
			http->ReadTimeout = 40000;
			http->IOHandler = ssl;
			request->containerItemCollection = containerItemCollection;
			request->track_ids = track_ids;
			request->version = this->version;
			request->SetHeaders(params);
			http->Request->Clear();
			http->Request->ContentType = "application/json";
			http->Request->ContentEncoding = "gzip,deflate,br,zstd";
			http->Request->AcceptCharSet = "windows-1251,utf-8";
			http->Request->Accept = "text/html,application/xhtml+xml,application/xml,application/json;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7";
			http->Request->Connection = "keep-alive";
			http->Request->UserAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/111.0.0.0 Safari/537.36";
			http->Request->BasicAuthentication = true;
			String requestText = "";
            if (version == 1)
			{
				http->Request->AcceptLanguage = "ru-RU,ru;q=0.9,en-US;q=0.8,en;q=0.7";
				http->Request->Host = "soap.ctm.ru:8112";
				http->Request->Referer = "http://soap.ctm.ru:8112";
				requestText = "http://soap.ctm.ru:8112/api/v1.0/";
			}
			else
			{
				http->Request->AcceptLanguage = "ru";
				http->Request->Host = "rail-scan.com";
				http->Request->Referer = "https://rail-scan.com/";
				requestText = "https://rail-scan.com/api/v2.0/";
			}
			if (userName == "" || userPassword == "")
			{
				response->result = "LOGIN_OR_PASSWORD_ARE_EMPTY";
                return false;
            }
			http->Request->Username = userName;
			http->Request->Password = userPassword;
			String responseText = "";
			String getErrorMessage = "";
			try
			{
				responseText = GetResponse(params, http, requestText);
			}
			catch(Exception *ex)
            {
				getErrorMessage = ex->Message;
            }
			//String cookieStr = "Cookie:\n";
			/*for (int i = 0; i < cookieMgr->CookieCollection->Count; i++)
				cookieStr += cookieMgr->CookieCollection->Cookies[i]->CookieText; */
			if (CheckResponseCode(http->Response->ResponseCode))
			{
				response->responseBody = responseText;
				json = static_cast<TJSONObject*>(TJSONObject::ParseJSONValue(responseText));
				if (json == NULL)
				{
					response->result = "WRONG_REQUEST";
					return false;
				}
				if (version == 1)
				{
					response->result = json->Get("result")->JsonValue->Value();
					if (response->result.UpperCase() != "SUCCESS")
					{
						return false;
					}
				}
				else
					response->result = "SUCCESS";

				TJSONArray *data;
				try
				{
					data = static_cast<TJSONArray*>(json->Get("data")->JsonValue);
				}
				catch(...)
				{
					data = NULL;
				}
				if (data == NULL)
				{
					response->result = "WRONG_DATA";
					return false;
				}
				switch(this->rType)
				{
					case RLType::BALANCE:  //{"result":"success","data":[{"balance":"398431.08"}]}
					{
						try
						{
                            if (data->Count == 0)
							{
								response->result = "NO_DATA";
								return false;
							}
                            TJSONObject *dataItem = static_cast<TJSONObject*>(data->Items[0]);
							RLBalanceData *respData = new RLBalanceData();
							String temp = dataItem->Get("balance")->JsonValue->Value();
							if (temp.Pos('.') != 0)
								temp = StringReplace(temp, ".", ",", TReplaceFlags() << rfReplaceAll);
							respData->balance = StrToFloat(temp);
							response->Data = respData;
                        }
						catch(Exception *ex)
						{
							response->result = ex->Message.UpperCase();
                            return false;
						}
						break;
					}
					case RLType::DECREASE:
					{
						try
						{
							if (data->Count == 0)
							{
								response->result = "NO_DATA";
								return false;
							}
							response->dataCount = data->Count;
							RLDecreaseData *respData = new RLDecreaseData[data->Count];
							for (int i = 0; i < data->Count; i++)
							{
								TJSONObject *dataItem = static_cast<TJSONObject*>(data->Items[i]);
								respData[i].track_id = StrToInt(dataItem->Get("track_id")->JsonValue->Value());
								respData[i].vagen_num = dataItem->Get("vagen_num")->JsonValue->Value();
								respData[i].nss = dataItem->Get("nss")->JsonValue->Value();
								String temp = dataItem->Get("balance")->JsonValue->Value();
								if (temp.Pos('.') != 0)
									temp = StringReplace(temp, ".", ",", TReplaceFlags() << rfReplaceAll);
								respData[i].balance = StrToFloat(temp);
								temp = dataItem->Get("cost_ops")->JsonValue->Value();
                                if (temp.Pos('.') != 0)
									temp = StringReplace(temp, ".", ",", TReplaceFlags() << rfReplaceAll);
								respData[i].cost_ops = StrToFloat(temp);
								try
								{
									String tempDate = dataItem->Get("insertred_at")->JsonValue->Value();
									if (tempDate != "")
									{
										tempDate[5] = '-';
										tempDate[8] = '-';
									}
									respData[i].insertred_at = StrToDateTime(tempDate, fs);
                                }
								catch(...){}
							}
							response->Data = respData;
						}
                        catch(Exception *ex)
						{
							response->result = ex->Message.UpperCase();
                            return false;
						}
						break;
					}
					case RLType::LIST_CONTAINER_TRACKING:
					{
						try
						{
							if (data->Count == 0)
							{
								response->result = "NO_DATA";
								return false;
							}
							response->dataCount = data->Count;
							RLListContainerTrackingData *respData = new RLListContainerTrackingData[data->Count];
							for (int i = 0; i < data->Count; i++)
							{
								TJSONObject *dataItem = static_cast<TJSONObject*>(data->Items[i]);
								respData[i].track_id = StrToInt(dataItem->Get("track_id")->JsonValue->Value());
								respData[i].vagen_num = dataItem->Get("vagen_num")->JsonValue->Value();
								respData[i].track_descr = StrToInt(dataItem->Get("track_descr")->JsonValue->Value());
								try
								{
									respData[i].track_create = StrToDateTime(dataItem->Get("track_create")->JsonValue->Value(), fs);
								}
								catch(...){}
								try
								{
									respData[i].date_of_update = StrToDateTime(dataItem->Get("date_of_update")->JsonValue->Value(), fs);
								}
								catch(...){}
								respData[i].comment = dataItem->Get("comment")->JsonValue->Value();
							}
							response->Data = respData;
						}
						catch(Exception *ex)
						{
							response->result = ex->Message.UpperCase();
							return false;
                        }
						break;
					}
					case RLType::CONTAINER_DELETE:
					{
						try
						{
							if (data->Count == 0)
							{
								response->result = "NO_DATA";
								return false;
							}
							response->dataCount = data->Count;
							RLContainerDeletedData *respData = new RLContainerDeletedData[data->Count];
							for (int i = 0; i < data->Count; i++)
							{
								TJSONObject *dataItem = static_cast<TJSONObject*>(data->Items[i]);
								if (this->version == 1)
								{
									respData[i].track_id = StrToInt(dataItem->Get("track_id")->JsonValue->Value());
									respData[i].vagen_num = dataItem->Get("vagen_num")->JsonValue->Value();
								}
								else // version 2.0
								{
                                    respData[i].track_id = StrToInt(dataItem->Get("track_id")->JsonValue->Value());
									respData[i].status_code = StrToInt(dataItem->Get("status_code")->JsonValue->Value());
									try
									{
										respData[i].msg = dataItem->Get("description")->JsonValue->Value();
									}
									catch(...){ respData[i].msg = ""; }
								}
							}
							response->Data = respData;
						}
						catch(Exception *ex)
						{
							response->result = ex->Message.UpperCase();
							return false;
                        }
						break;
					}
					case RLType::CONTAINER_EXTENDED_DATA:
					{
						try
						{
							if (data->Count == 0)
							{
								response->result = "NO_DATA";
								return false;
							}
							response->dataCount = data->Count;
							RLExtendedTrackingData *respData = new RLExtendedTrackingData[data->Count];
							for (int i = 0; i < data->Count; i++)
							{
								TJSONObject *dataItem = static_cast<TJSONObject*>(data->Items[i]);
								if (version == 1)
								{
									try
									{
										respData[i].track_id = StrToInt(dataItem->Get("track_id")->JsonValue->Value());
									}
									catch(...){}
									try
									{
										respData[i].cont_num = dataItem->Get("cont_num")->JsonValue->Value();
									}
									catch(...)
									{
										respData[i].msg = "NO_CONTAINER_IN_THIS_COUNTRY";
									}
									try
									{
										String tempDate = dataItem->Get("datearrive")->JsonValue->Value();
										if (tempDate != "")
										{
											tempDate[5] = '-';
											tempDate[8] = '-';
										}
										respData[i].datearrive = StrToDateTime(tempDate, fs);
									}
									catch(...){}
									try
									{
										String tempDate = dataItem->Get("approx_date_of_arrival")->JsonValue->Value();
										if (tempDate != "")
										{
											tempDate[5] = '-';
											tempDate[8] = '-';
										}
										respData[i].approx_date_of_arrival = StrToDateTime(tempDate, fs);
									}
									catch(...){}
									try
									{
										String tempDate = dataItem->Get("cont_load_to_wagon_date")->JsonValue->Value();
										if (tempDate != "")
										{
											tempDate[5] = '-';
											tempDate[8] = '-';
										}
										respData[i].cont_load_to_wagon_date = StrToDateTime(tempDate, fs);
									}
									catch(...){}
									try
									{
										String tempDate = dataItem->Get("date_of_departure")->JsonValue->Value();
										if (tempDate != "")
										{
											tempDate[5] = '-';
											tempDate[8] = '-';
										}
										respData[i].date_of_departure = StrToDateTime(tempDate, fs);
									}
									catch(...){}
									try
									{
										respData[i].rest_range = StrToInt(dataItem->Get("rest_range")->JsonValue->Value());
									}
									catch(...) { respData[i].rest_range = 0; }
									try
									{
										respData[i].rest_days = StrToInt(dataItem->Get("rest_days")->JsonValue->Value());
									}
									catch(...) { respData[i].rest_days = 0; }
									try
									{
										respData[i].passed_distance = StrToInt(dataItem->Get("passed_distance")->JsonValue->Value());
									}
									catch(...) { respData[i].passed_distance = 0; }
									try
									{
										respData[i].distance = StrToInt(dataItem->Get("distance")->JsonValue->Value());
									}
									catch(...) { respData[i].distance = 0; }
									try
									{
										String tempDate = dataItem->Get("date_rec")->JsonValue->Value();
										if (tempDate != "")
										{
											tempDate[5] = '-';
											tempDate[8] = '-';
										}
										respData[i].date_rec = StrToDateTime(tempDate, fs);
									}
									catch(...){}
									try
									{
										respData[i].train_num = dataItem->Get("train_num")->JsonValue->Value();
									}
									catch(...){}
									/*String temp = dataItem->Get("cargo_weight")->JsonValue->Value();
										if (temp.Pos('.') != 0)
											temp = StringReplace(temp, ".", ",", TReplaceFlags() << rfReplaceAll);
									respData[i].cargo_weight = StrToFloat(temp);   */
									try
									{
										respData[i].dest_point_name = dataItem->Get("dest_point_name")->JsonValue->Value();
									}
									catch(...){}
									try
									{
										respData[i].from_station = dataItem->Get("from_station")->JsonValue->Value();
									}
									catch(...){}
									try
									{
										respData[i].name_station = dataItem->Get("name_station")->JsonValue->Value();
									}
									catch(...){}
									try
									{
										respData[i].platform_number = dataItem->Get("car_number")->JsonValue->Value();
									}
									catch(...){}
								}
								else  // version 2.0
								{
									try
									{
										respData[i].track_id = StrToInt(dataItem->Get("track_id")->JsonValue->Value());
									}
									catch(...){}
                                    try
									{
										respData[i].status_code = StrToInt(dataItem->Get("status_code")->JsonValue->Value());
									}
									catch(...){}
									try
									{
										respData[i].msg = dataItem->Get("description")->JsonValue->Value();
									}
									catch(...){ respData[i].msg = ""; }
									if (respData[i].status_code > 200)
									{
										respData[i].msg = "NO_CONTENT" + ((respData[i].msg != "") ? " " + respData[i].msg : String(""));
										continue;
									}
									TJSONArray *localData = static_cast<TJSONArray*>(dataItem->Get("data")->JsonValue);
									// ¬сегда берем первую запись с операцией по треку
									if (localData->Count > 0)
									{
										TJSONObject *localDataItem = static_cast<TJSONObject*>(localData->Items[0]);
                                        try
										{
											respData[i].cont_num = localDataItem->Get("cargo_transport_unit_number")->JsonValue->Value();
										}
										catch(...){}
										try
										{
											String tempDate = localDataItem->Get("delivery_time")->JsonValue->Value();
											tempDate = StringReplace(tempDate, "T", " ", TReplaceFlags() << rfReplaceAll);
											if (tempDate.Pos(".") > 0)
												tempDate = tempDate.SubString(1, tempDate.Pos(".") - 1);
											respData[i].datearrive = StrToDateTime(tempDate, fs);
										}
										catch(...){}
										try
										{
											String tempDate = localDataItem->Get("approximal_arrival_time")->JsonValue->Value();
											tempDate = StringReplace(tempDate, "T", " ", TReplaceFlags() << rfReplaceAll);
                                            if (tempDate.Pos(".") > 0)
												tempDate = tempDate.SubString(1, tempDate.Pos(".") - 1);
											respData[i].approx_date_of_arrival = StrToDateTime(tempDate, fs);
										}
										catch(...){}
                                        try
										{
											String tempDate = localDataItem->Get("departure_time")->JsonValue->Value();
											tempDate = StringReplace(tempDate, "T", " ", TReplaceFlags() << rfReplaceAll);
                                            if (tempDate.Pos(".") > 0)
												tempDate = tempDate.SubString(1, tempDate.Pos(".") - 1);
											respData[i].date_of_departure = StrToDateTime(tempDate, fs);
										}
										catch(...){}
										respData[i].cont_load_to_wagon_date = respData[i].date_of_departure;
										try
										{
											respData[i].rest_range = StrToInt(localDataItem->Get("route_left_distance")->JsonValue->Value());
										}
										catch(...) { respData[i].rest_range = 0; }
                                        char sep = System::Sysutils::FormatSettings.DecimalSeparator;
										System::Sysutils::FormatSettings.DecimalSeparator = '.';
										try
										{
											double fdays = StrToFloat(localDataItem->Get("route_left_distance_days")->JsonValue->Value());
											respData[i].rest_days = (int)fdays;
										}
										catch(...) { respData[i].rest_days = 0; }
										System::Sysutils::FormatSettings.DecimalSeparator = sep;
										try
										{
											respData[i].passed_distance = StrToInt(localDataItem->Get("route_passed_distance")->JsonValue->Value());
										}
										catch(...) { respData[i].passed_distance = 0; }
										try
										{
											respData[i].distance = StrToInt(localDataItem->Get("route_distance")->JsonValue->Value());
										}
										catch(...) { respData[i].distance = 0; }
										try
										{
											String tempDate = localDataItem->Get("track_create_time")->JsonValue->Value();
											tempDate = StringReplace(tempDate, "T", " ", TReplaceFlags() << rfReplaceAll);
											if (tempDate.Pos(".") > 0)
												tempDate = tempDate.SubString(1, tempDate.Pos(".") - 1);
											respData[i].date_rec = StrToDateTime(tempDate, fs);
										}
										catch(...){}
                                        try
										{
											respData[i].train_num = localDataItem->Get("train_number")->JsonValue->Value();
										}
										catch(...){}
										try
										{
											respData[i].dest_point_name = localDataItem->Get("destination_station_name")->JsonValue->Value();
										}
										catch(...){}
										try
										{
											respData[i].from_station = localDataItem->Get("departure_station_name")->JsonValue->Value();
										}
										catch(...){}
										try
										{
											respData[i].name_station = localDataItem->Get("operation_station_name")->JsonValue->Value();
										}
										catch(...){}
										try
										{
											respData[i].platform_number = localDataItem->Get("platform_number")->JsonValue->Value();
										}
										catch(...){}
									}
								}
							}
							response->Data = respData;
						}
						catch(Exception *ex)
						{
							response->result = ex->Message.UpperCase();
							return false;
                        }
						break;
					}
					case RLType::REGULATION_TRACKING_DAILY:
					{
						try
						{
							if (data->Count == 0)
							{
								response->result = "NO_DATA";
								return false;
							}
							response->dataCount = data->Count;
							String error;
							RLTrackingDailyData *respData = new RLTrackingDailyData[data->Count];
							for (int i = 0; i < data->Count; i++)
							{
								TJSONObject *dataItem = static_cast<TJSONObject*>(data->Items[i]);
								if (version == 1)
								{
									try
									{
										error = dataItem->Get("error")->JsonValue->Value();
									}
									catch(...){ error = ""; }
									if (error != "")
									{
										respData[i].msg = error;
										//continue;
									}
									try
									{
										respData[i].track_id = StrToInt(dataItem->Get("track_id")->JsonValue->Value());
									}
									catch(...){ respData[i].track_id = 0; }
									try
									{
										respData[i].vagen_num = dataItem->Get("vagen_num")->JsonValue->Value();
									}
									catch(...)
									{
										respData[i].vagen_num = "";
									}
								}
								else // version 2.0
								{
									try
									{
										respData[i].track_id = StrToInt(dataItem->Get("track_id")->JsonValue->Value());
									}
									catch(...){ respData[i].track_id = 0; }
                                    try
									{
										respData[i].status_code = StrToInt(dataItem->Get("status_code")->JsonValue->Value());
									}
									catch(...){ respData[i].status_code = 0; }
									try
									{
										respData[i].msg = dataItem->Get("description")->JsonValue->Value();
									}
									catch(...){ respData[i].msg = ""; }
									TJSONObject *request = static_cast<TJSONObject*>(dataItem->Get("request")->JsonValue);
									if (request != NULL)
									{
										try
										{
											respData[i].vagen_num = request->Get("cargo_transport_unit_number")->JsonValue->Value();
										}
										catch(...)
										{
											respData[i].vagen_num = "";
										}

									}
								}
							}
							response->Data = respData;
						}
						catch(Exception *ex)
						{
							response->result = ex->Message.UpperCase();
							return false;
                        }
						break;
					}
				}
			}
			else
			{
				if (getErrorMessage.LowerCase().Pos("description") > 0)
				{
					TJSONObject *json = static_cast<TJSONObject*>(TJSONObject::ParseJSONValue(getErrorMessage));
					if (json != NULL)
					{
						UnicodeString jmsg;
						try
						{
							jmsg = json->Get("description")->JsonValue->Value();
						}
						catch(...){}
						if (!jmsg.IsEmpty())
							getErrorMessage = jmsg;
					}
				}
				response->result = "WRONG_RESPONSE_CODE: " + IntToStr(http->Response->ResponseCode) + ". " + getErrorMessage;
				return false;
			}
		}
		catch(Exception *ex)
		{
			response->result = ex->Message.UpperCase();
			return false;
		}
	}
	__finally
	{
		if (json)
			delete json;
		if (ssl != NULL)
			delete ssl;
		if (http != NULL)
			delete http;
		if (params != NULL)
			delete params;
		if (cookieMgr != NULL)
			delete cookieMgr;
	}
    return true;
}

String RailLocatorTools::GetVagensToString(TStringList *containerNames, TStringList *railwayCodes)
{
	String tempVagens = "";
	if (containerNames->Count < 1) {
		return tempVagens;
	}
	if (containerNames->Count == 1)
	{
		tempVagens = containerNames->Strings[0] + "," + railwayCodes->Strings[0];
	}
	else if (containerNames->Count > 1)
		{
			for (int i = 0; i < containerNames->Count; i++)
			{
				tempVagens += containerNames->Strings[i] + "," + railwayCodes->Strings[i] + ";";
			}
			tempVagens.Delete(tempVagens.Length(), 1);
		}

	return tempVagens;
}

String RailLocatorTools::GetTracksToString(TStringList *trackIds)
{
	String tempTracks = "";
	String separator = ";";
	if (this->version == 2)
		separator = ",";
	if (trackIds->Count < 1) {
		return tempTracks;
	}
	if (trackIds->Count == 2)
	{
		tempTracks = trackIds->Strings[0] + separator + trackIds->Strings[1];
	}
	else if (trackIds->Count > 2)
		{
			for (int i = 0; i < trackIds->Count; i++)
			{
				tempTracks += trackIds->Strings[i] + separator;
			}
			tempTracks.Delete(tempTracks.Length(), 1);
		}
		else
			tempTracks = trackIds->Strings[0];
	return tempTracks;
}

bool RailLocatorTools::GetMultipleExtendedData(TStringList *trackIds)
{
	String tempTracks = GetTracksToString(trackIds);
	if (tempTracks == "")
		return false;
	this->rType = RLType::CONTAINER_EXTENDED_DATA;

	return GetResponseInfo(tempTracks);
}

bool RailLocatorTools::DeleteMultipleData(TStringList *trackIds)
{
	String tempTracks = GetTracksToString(trackIds);
	if (tempTracks == "")
		return false;
	this->rType = RLType::CONTAINER_DELETE;

	return GetResponseInfo(tempTracks);
}

bool RailLocatorTools::SetMultipleTrackingData(RLContainerItemCollection *containerItemCollection)
{
	if (!containerItemCollection)
		return false;

	if (containerItemCollection->Count() < 1)
		return false;

	this->rType = RLType::REGULATION_TRACKING_DAILY;

	return GetResponseInfo("", containerItemCollection);
}



