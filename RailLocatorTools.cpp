//---------------------------------------------------------------------------

#pragma hdrstop

#include "RailLocatorTools.h"
#include <IdHTTP.hpp>
#include <IdTCPClient.hpp>
#include <IdTCPConnection.hpp>
#include <IdIOHandler.hpp>
#include <IdIOHandlerSocket.hpp>
#include <IdIOHandlerStack.hpp>
#include <IdSSL.hpp>
#include <IdSSLOpenSSL.hpp>
#include <DBXJSON.hpp>
//---------------------------------------------------------------------------
#pragma package(smart_init)


void RLRequest::SetHeaders(TStringList *params)
{
	switch(reqType)
	{
		case RLType::REGULATION_TRACKING_DAILY:
		{
			if (track_type != "" && vagens != "")
			{
				params->Text = "{\"track_type\": \""+track_type+"\",\"vagens\":\""+vagens+"\"}";
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

String RailLocatorTools::GetResponse(TStringList *params, TIdHTTP *http, String &request)
{
	String response = "";
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
			TStringStream *paramStream = new TStringStream();
            paramStream->WriteString(params->Text);
			TStringStream *respStream = new TStringStream();
			//http->HTTPOptions = (http->HTTPOptions << hoKeepOrigProtocol);
			TLocalHTTP *httpAccess = (TLocalHTTP*)http;
			httpAccess->DeleteRequest(request, paramStream, respStream);
			//http->Delete(request, respStream);
			response = respStream->DataString;
			break;
		}
		case RLType::CONTAINER_EXTENDED_DATA:
		{
			request += "tracks/extended-data";
			response = http->Post(request, params);
			break;
		}
		case RLType::REGULATION_TRACKING_DAILY:
		{
			request += "tracks";
			response = http->Post(request, params);
			break;
		}
	}
	return response;
}

bool RailLocatorTools::GetResponseInfo(String track_ids, String containerName, String track_type, String railwayCode)
{
	TFormatSettings fs;
	fs.DateSeparator = '-';
	fs.ShortDateFormat = "yyyy-mm-dd";
    fs.TimeSeparator = ':';
	fs.ShortTimeFormat = "hh:mm";
	fs.LongTimeFormat = "hh:mm:ss";

	TIdHTTP *http = NULL;
	TIdSSLIOHandlerSocketOpenSSL *ssl = NULL;
	TIdCookieManager * cookieMgr = NULL;
	//TIdMultiPartFormDataStream *params = NULL;
	TStringList *params = NULL;
	String tracks_id = "";
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
			request->track_type = track_type;
			request->track_ids = track_ids;
			request->vagens = containerName + "," + railwayCode;
			request->SetHeaders(params);
			http->Request->Clear();
			http->Request->Host = "soap.ctm.ru:8112";
			http->Request->ContentType = "application/json";
			http->Request->ContentEncoding = "gzip,deflate,br";
			http->Request->AcceptLanguage = "ru-RU,ru;q=0.9,en-US;q=0.8,en;q=0.7";
			http->Request->AcceptCharSet = "windows-1251,utf-8";
			http->Request->Accept = "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9";
			http->Request->Connection = "keep-alive";
			http->Request->Referer = "http://soap.ctm.ru:8112";
			http->Request->UserAgent = "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/88.0.4324.190 Safari/537.36";
			http->Request->BasicAuthentication = true;
			if (userName == "" || userPassword == "")
			{
				response->result = "LOGIN_OR_PASSWORD_ARE_EMPTY";
                return false;
            }
			http->Request->Username = userName;
			http->Request->Password = userPassword;
			String requestText = "http://soap.ctm.ru:8112/api/v1.0/";
			String responseText = "";
			try
			{
				responseText = GetResponse(params, http, requestText);
			}
			catch(...){}
			//String cookieStr = "Cookie:\n";
			/*for (int i = 0; i < cookieMgr->CookieCollection->Count; i++)
				cookieStr += cookieMgr->CookieCollection->Cookies[i]->CookieText; */
			if (http->Response->ResponseCode == 200)
			{
				TJSONObject *json = static_cast<TJSONObject*>(TJSONObject::ParseJSONValue(responseText));
				if (json == NULL)
				{
					response->result = "WRONG_REQUEST";
					return false;
				}
				response->responseBody = responseText;
				response->result = json->Get("result")->JsonValue->Value();
				if (response->result.UpperCase() != "SUCCESS")
				{
					return false;
				}
				TJSONArray *data = static_cast<TJSONArray*>(json->Get("data")->JsonValue);
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
								respData[i].track_id = StrToInt(dataItem->Get("track_id")->JsonValue->Value());
								respData[i].vagen_num = dataItem->Get("vagen_num")->JsonValue->Value();
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
								respData[i].track_id = StrToInt(dataItem->Get("track_id")->JsonValue->Value());
								respData[i].cont_num = dataItem->Get("cont_num")->JsonValue->Value();
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
								respData[i].rest_range = StrToInt(dataItem->Get("rest_range")->JsonValue->Value());
								respData[i].rest_days = StrToInt(dataItem->Get("rest_days")->JsonValue->Value());
								respData[i].passed_distance = StrToInt(dataItem->Get("passed_distance")->JsonValue->Value());
								respData[i].distance = StrToInt(dataItem->Get("distance")->JsonValue->Value());
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
								respData[i].train_num = dataItem->Get("train_num")->JsonValue->Value();
								/*String temp = dataItem->Get("cargo_weight")->JsonValue->Value();
									if (temp.Pos('.') != 0)
										temp = StringReplace(temp, ".", ",", TReplaceFlags() << rfReplaceAll);
								respData[i].cargo_weight = StrToFloat(temp);   */
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
							RLTrackingDailyData *respData = new RLTrackingDailyData[data->Count];
							for (int i = 0; i < data->Count; i++)
							{
								TJSONObject *dataItem = static_cast<TJSONObject*>(data->Items[i]);
								respData[i].track_id = StrToInt(dataItem->Get("track_id")->JsonValue->Value());
								respData[i].vagen_num = dataItem->Get("vagen_num")->JsonValue->Value();
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
				response->result = "WRONG_RESPONSE_CODE: " + http->Response->ResponseCode;
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
	if (trackIds->Count < 1) {
		return tempTracks;
	}
	if (trackIds->Count == 2)
	{
		tempTracks = trackIds->Strings[0] + ";" + trackIds->Strings[1];
	}
	else if (trackIds->Count > 2)
		{
			for (int i = 0; i < trackIds->Count; i++)
			{
				tempTracks += trackIds->Strings[i] + ";";
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
	if (GetResponseInfo(tempTracks))
		return true;
	else
		return false;
}

bool RailLocatorTools::DeleteMultipleData(TStringList *trackIds)
{
	String tempTracks = GetTracksToString(trackIds);
	if (tempTracks == "")
		return false;
	this->rType = RLType::CONTAINER_DELETE;
	if (GetResponseInfo(tempTracks))
		return true;
	else
		return false;
}



