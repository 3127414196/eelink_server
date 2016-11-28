/*
 * msg_proc_http.c
 *
 *  Created on: 2016/11/28
 *      Author: lc
 */
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "log.h"
#include "cJSON.h"
#include "msg_http.h"
#include "phone_alarm.h"
#include "telephone_http.h"

static void telephone_deleteTelNumber(struct evhttp_request *req, const char *imeiName)
{
    int rc = db_deleteTelNumber(imeiName);
    if(!rc)
    {
        http_okMsg(req);
        return;
    }

    http_errorMsg(req);
    return;
}

static void telephone_replaceTelNumber(struct evhttp_request *req, const char *imeiName, const char *telNumber)
{
    int rc = db_replaceTelNumber(imeiName, telNumber);
    if(!rc)
    {
        http_okMsg(req);
        return;
    }

    http_errorMsg(req);
    return;
}

static void telephone_getTelNumber(struct evhttp_request *req, const char *imeiName)
{
    char telNumber[TELNUMBER_LENGTH + 1] = {0};

    cJSON *json = cJSON_CreateObject();
    if(!json)
    {
        LOG_FATAL("failed to alloc memory");
        http_errorMsg(req);
        return;
    }

    int rc = db_getTelNumber(imeiName, telNumber);
    if(rc)
    {
        cJSON_AddNumberToObject(json, "code", 101);
    }
    else
    {
        cJSON_AddStringToObject(json, "telephone", telNumber);
    }

    char *msg = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);

    LOG_DEBUG("%s",msg);

    http_rspMsg(req, msg);
    free(msg);
    return;
}

void http_replyTelephone(struct evhttp_request *req)
{
    int rc;
    char imei[IMEI_LENGTH + 1] = {0};
    char telNumber[TELNUMBER_LENGTH + 1] = {0};
    LOG_INFO("%s",req->uri);
    switch(req->type)
    {
        case EVHTTP_REQ_GET:
            rc = sscanf(req->uri, "/v1/telephone/%15s%*s", imei);
            if(rc == 1)
            {
                telephone_getTelNumber(req, imei);
                return;
            }
            break;

        case EVHTTP_REQ_PUT:
        case EVHTTP_REQ_POST:
            rc = sscanf(req->uri, "/v1/telephone/%15s?telephone=%11s%*s", imei, telNumber);
            if(rc == 2)
            {
                LOG_INFO("open call alarm server imei(%s) telNumber(%s)", imei, telNumber);
                telephone_replaceTelNumber(req, imei, telNumber);
                return;
            }
            break;

        case EVHTTP_REQ_DELETE:
            rc = sscanf(req->uri, "/v1/telephone/%15s%*s", imei);
            if(rc == 1)
            {
                LOG_INFO("close call alarm server imei(%s)", imei);
                telephone_deleteTelNumber(req, imei);
                return;
            }
            break;

         default:
            LOG_ERROR("unkown http type: %d",req->type);
            break;

    }

    http_errorMsg(req);
    return;
}

void http_replyCall(struct evhttp_request *req)
{
    int rc;
    char number[TELNUMBER_LENGTH + 1] = {0};

    LOG_INFO("%s",req->uri);
    switch(req->type)
    {
        case EVHTTP_REQ_GET:
        case EVHTTP_REQ_PUT:
        case EVHTTP_REQ_POST:
        case EVHTTP_REQ_DELETE:
            rc = sscanf(req->uri, "/v1/test/%11s%*s", number);
            if(rc == 1)
            {
                 phone_alarm(number);
                 http_okMsg(req);
                 return;
            }
            break;

        default:
            LOG_ERROR("unkown http type: %d",req->type);
            break;
    }

    http_errorMsg(req);
    return;
}


