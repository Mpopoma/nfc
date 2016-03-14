// Copyright 2016 by nemik, see LICENSE.txt

#include <esp8266.h>
#include "cgi.h"
#include "config.h"
#include "cginfc.h"

#ifdef CGINFC_DBG
#define DBG(format, ...) do { os_printf(format, ## __VA_ARGS__); } while(0)
#else
#define DBG(format, ...) do { } while(0)
#endif

// Cgi to return NFC settings
int ICACHE_FLASH_ATTR cgiNFCGet(HttpdConnData *connData) {
  char buff[1024];
  int len;

  if (connData->conn==NULL) return HTTPD_CGI_DONE;

  len = os_sprintf(buff, "{ "
      "\"nfc-url\":\"%s\", "
      "\"nfc-device-id\":\"%s\", "
      "\"nfc-device-secret\":\"%s\", "
      "\"nfc-counter\":%d }",
      flashConfig.nfc_url,
			flashConfig.nfc_device_id,
      flashConfig.nfc_device_secret,
      flashConfig.nfc_counter);

  jsonHeader(connData, 200);
  httpdSend(connData, buff, len);
  return HTTPD_CGI_DONE;
}

int ICACHE_FLASH_ATTR cgiNFCSet(HttpdConnData *connData) {
  if (connData->conn==NULL) return HTTPD_CGI_DONE;

  // handle NFC server settings
  int8_t nfc_server = 0; // accumulator for changes/errors
  nfc_server |= getStringArg(connData, "nfc-url",
      flashConfig.nfc_url, sizeof(flashConfig.nfc_url));
  if (nfc_server < 0) return HTTPD_CGI_DONE;
  nfc_server |= getStringArg(connData, "nfc-device-id",
      flashConfig.nfc_device_id, sizeof(flashConfig.nfc_device_id));

  if (nfc_server < 0) return HTTPD_CGI_DONE;
  nfc_server |= getStringArg(connData, "nfc-device-secret",
      flashConfig.nfc_device_secret, sizeof(flashConfig.nfc_device_secret));
  if (nfc_server < 0) return HTTPD_CGI_DONE;

  char buff[16];

  // handle nfc counter
  if (httpdFindArg(connData->getArgs, "nfc-counter", buff, sizeof(buff)) > 0) {
    uint32_t counter = atoi(buff);
    if (counter > 0 && counter < 4294967295) {
      flashConfig.nfc_counter = counter;
      nfc_server |= 1;
    } else {
      errorResponse(connData, 400, "Invalid NFC counter!");
      return HTTPD_CGI_DONE;
    }
  }

	DBG("Saving config\n");

  if (configSave()) {
    httpdStartResponse(connData, 200);
    httpdEndHeaders(connData);
  } else {
    httpdStartResponse(connData, 500);
    httpdEndHeaders(connData);
    httpdSend(connData, "Failed to save config", -1);
  }
  return HTTPD_CGI_DONE;
}

int ICACHE_FLASH_ATTR cgiNFC(HttpdConnData *connData) {
  if (connData->requestType == HTTPD_METHOD_GET) {
    return cgiNFCGet(connData);
  } else if (connData->requestType == HTTPD_METHOD_POST) {
    return cgiNFCSet(connData);
  } else {
    jsonHeader(connData, 404);
    return HTTPD_CGI_DONE;
  }
}
