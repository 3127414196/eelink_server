/*
 * object_mc.c
 *
 *  Created on: Apr 19, 2015
 *      Author: jk
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <glib.h>

#include "log.h"
#include "object.h"

/* global mc hash table */
static GHashTable *g_table = NULL;


#ifdef __LOCAL_STORARY__
#define CONFIG_FILE "../conf/config.dat"

typedef struct
{
	char IMEI[IMEI_LENGTH];
	char DID[MAX_DID_LEN];
	char PWD[MAX_PWD_LEN];
    int device_id;
    int sensor_id;
}OBJ_SAVED;

static int mc_readConfig()
{
	int fd = open(CONFIG_FILE, O_RDONLY);
	if (fd == -1)
	{
        LOG_FATAL("open file for read fail");
        return -1;
	}

	while(1)
	{
		OBJ_SAVED objBuf;
		ssize_t readlen = read(fd, &objBuf, sizeof(OBJ_SAVED));
		if (readlen == sizeof(OBJ_SAVED))
		{
			OBJECT* obj = mc_obj_new();
			memcpy(obj->IMEI, objBuf.IMEI, IMEI_LENGTH);
			memcpy(obj->DID, objBuf.DID, MAX_DID_LEN);
			memcpy(obj->pwd, objBuf.PWD, MAX_DID_LEN);
            obj->device_id = objBuf.device_id;
            obj->sensor_id = objBuf.sensor_id;

            /* add to mc hash */
            mc_obj_add(obj);
		}
		else
		{
			break;
		}
	};

    close(fd);
    return 0;
}


void mc_writeConfig(gpointer key, gpointer value, gpointer user_data)
{
	int fd = *(int*)user_data;

    OBJECT* obj = (OBJECT*)value;

    OBJ_SAVED objBuf;
    memcpy(objBuf.IMEI, obj->IMEI, IMEI_LENGTH);
	memcpy(objBuf.DID, obj->DID, MAX_DID_LEN);
	memcpy(objBuf.PWD, obj->pwd, MAX_PWD_LEN);
    objBuf.device_id = obj->device_id;
    objBuf.sensor_id = obj->sensor_id;

	ssize_t written = write(fd, &objBuf, sizeof(OBJ_SAVED));
	if (written == -1)
	{
		LOG_ERROR("save config error");
		return;
	}

	return;
}


int mc_saveConfig()
{
    int fd = open(CONFIG_FILE, O_RDWR | O_CREAT, S_IRWXU);
    if(-1 == fd)
    {
        LOG_FATAL("open file for write fail");
        return -1;
    }

    /* foreach mc hash */
    g_hash_table_foreach(g_table, mc_writeConfig, &fd);

    close(fd);

    return 0;
}
#endif

void mc_freeKey(gpointer key)
{
    LOG_DEBUG("free key IMEI:%s", key);
    g_free(key);
}

void mc_freeValue(gpointer value)
{
    OBJECT * obj = (OBJECT *)value;

    LOG_DEBUG("free value IMEI:%s", get_IMEI_STRING(obj->IMEI));

    g_free(obj);
}

void obj_initial()
{
    /* create mc hash table */
    g_table = g_hash_table_new_full(g_str_hash, g_str_equal, mc_freeKey, mc_freeValue);

	//mc_readConfig();
}

void obj_destruct()
{
//	mc_saveConfig();

    g_hash_table_destroy(g_table);
}

static void make_pwd(char pwd[])
{
    srand(time(NULL));

    for(int i = 0; i < MAX_PWD_LEN; i++)
	{
        pwd[i] = 65 + rand() % (90 - 65);
	}
    pwd[MAX_PWD_LEN - 1] = '\0';
}

OBJECT *obj_new()
{
	OBJECT * obj = g_malloc(sizeof(OBJECT));
	memset(obj, 0, sizeof(OBJECT));

	make_pwd(obj->pwd);

	return obj;
}

/* add item into mc hash */
void obj_add(OBJECT *obj)
{
	const char* strIMEI = get_IMEI_STRING(obj->IMEI);
	gboolean rc = g_hash_table_insert(g_table, g_strdup(strIMEI), obj);
    if(rc != TRUE)
    {
        LOG_WARN("duplicate IMEI(%s)", get_IMEI_STRING(obj->IMEI));
    }
    else
    {
    	LOG_INFO("obj %s added", strIMEI);
    }
}


void mc_obj_del(OBJECT * obj)
{
    OBJECT * t_obj = obj_get(obj->IMEI);
    if(NULL != t_obj)
    {
        g_hash_table_remove(g_table, get_IMEI_STRING(obj->IMEI));
    }
}

OBJECT *obj_get(const char IMEI[])
{
    return g_hash_table_lookup(g_table, IMEI);
}

#ifdef __GIZWITS_SUPPORT__
int mc_obj_did_got(OBJECT* obj)
{
	return strlen(obj->DID) != 0;
}

const char* getMacFromIMEI(const unsigned char* IMEI)
{
	/*
	 *
	 * IMEI:  xx xx xx xx xx xx xx xx
	 * MAC:         ~~ ~~ ~~ ~~ ~~ ~~
	 */

	static char mac[MAC_MAC_LEN * 2 + 1] = {0};

    sprintf(mac,"%02X%02X%02X%02X%02X%02X", IMEI[2], IMEI[3],IMEI[4],IMEI[5],IMEI[6],IMEI[7]);


	return mac;
}

#endif

const char* get_IMEI_STRING(const unsigned char* IMEI)
{
	static char strIMEI[IMEI_LENGTH * 2 + 1];
	strcpy(strIMEI, "unknown imei");

	if (!IMEI)
	{
		return strIMEI;
	}

	for (int i = 0; i < IMEI_LENGTH; i++)
	{
		sprintf(strIMEI + i * 2, "%02x", IMEI[i]);
	}
	strIMEI[IMEI_LENGTH * 2] = 0;

	return strIMEI;
}

const unsigned char* get_IMEI(const char* strIMEI)
{
    static unsigned char IMEI[IMEI_LENGTH];
    unsigned char temp[2] = {0};
    int temp_a, temp_b;

    for (int i = 0; i < IMEI_LENGTH * 2; )
    {
        temp[0] = strIMEI[i];
        temp_a = atoi(temp);
        temp[0] = strIMEI[i + 1];
        temp_b = atoi(temp);
        IMEI[i / 2] = temp_a * 16 + temp_b;
        i += 2;
    }

    return IMEI;
}


int isYeelinkDeviceCreated(OBJECT * obj)
{
	return obj->device_id != 0 && obj->sensor_id != 0 ;
}
