#include <esp8266.h>
#include <config.h>
#include "osapi.h"

#include "espfs.h"

#include "stdout.h"
#include "gpio16.h"

#include "mfrc522.h"
#include "httpclient.h"
#include "jsmn.h"
#include "hmac_sha1.h"

#define PERIPHS_IO_MUX_PULLDWN          BIT6
#define PIN_PULLDWN_DIS(PIN_NAME)             CLEAR_PERI_REG_MASK(PIN_NAME, PERIPHS_IO_MUX_PULLDWN)
#define sleepms(x) os_delay_us(x*1000);

#define RFID_LOOP_DELAY 200 
#define LAST_TAG_TIME_THRESHOLD 5 // 5 seconds

//0 is lowest priority
#define procTaskPrio        0
#define procTaskQueueLen    1

static os_timer_t rfid_timer;
static bool did_sync_once = false;

static uint32_t rtc_delta_secs = 0;
static char last_tag [64];
static uint32_t last_tag_time = 0;

static char secret [50];
static char device_id [30];

static bool led_toggle = false;

uint32_t atoint32(char *instr)
{
  uint32_t retval;
  int i;

  retval = 0;
  for (; *instr; instr++) {
    retval = 10*retval + (*instr - '0');
  }
  return retval;
}

void ICACHE_FLASH_ATTR create_json(char * output, char * dev_id, char * tag_uid, uint32_t counter, char * hash)
{
	os_sprintf(output, "{\"device_id\": \"%s\",\r\n\"tag_uid\": \"%s\",\r\n\"counter\": %ld,\r\n\"hash\": \"%s\"}", dev_id, tag_uid, counter, hash);
}

void strip_newlines(char * inbuff, int len, char * outbuff)
{
	if( inbuff[len-1] == '\n' )
	{
		    inbuff[len-1] = 0;
	}
	strcpy(outbuff, inbuff);
}

bool is_connected()
{
	int x=wifi_station_get_connect_status();
	if (x != STATION_GOT_IP) {
		return false;
	}
	return true;
}

void ICACHE_FLASH_ATTR get_serial_secret()
{
	strcpy(secret, flashConfig.nfc_device_secret);
	strcpy(device_id, flashConfig.nfc_device_id);
}

uint32_t get_time_secs()
{
	uint32_t out = (system_get_time() / 1000000) + rtc_delta_secs;
	return out;
}

os_event_t    procTaskQueue[procTaskQueueLen];

static void ICACHE_FLASH_ATTR
procTask(os_event_t *events)
{
	system_os_post(procTaskPrio, 0, 0 );
	if( events->sig == 0 && events->par == 0 )
	{
		//Idle Event.
	}
}

void ICACHE_FLASH_ATTR hex2str(char *out, unsigned char *s, size_t len)
{
	char tmp_str[3];
	memset(out, 0, 40);
	memset(tmp_str, 0, 3);
	os_sprintf(tmp_str, "%02x", s[0]);
	strcpy(out, tmp_str);
	int ii;
	for(ii=1; ii<len; ii++)
	{
		memset(tmp_str, 0, 3);
		os_sprintf(tmp_str,"%02x", s[ii]);
		strcat(out, tmp_str);
	}
}

void up_count() {
	uint32_t counter = flashConfig.nfc_counter;
	counter++;
	flashConfig.nfc_counter = counter;
	configSave();
}

/*
 * JSON parsing stuff
 */

jsmn_parser parser;
jsmntok_t tokens[30];

char * json_token_tostr(char *js, jsmntok_t *t)
{
	js[t->end] = '\0';
	return js + t->start;
}

void rfid_http_cb(char * response_body, int http_status, char * full_response)
{
	os_printf("got http response status %d \r\nbody: '%s'\r\n", http_status, response_body);
	if(http_status == 200)
	{
		// parse the return body with jsmn
		jsmn_init(&parser);
		int jp = jsmn_parse(&parser, response_body, strlen(response_body), tokens, 30);

		//try to find color r/g/b's
		int r, g, b = 0;
		int delay = 1000; //milliseconds
		for(int i=1; i<30; i++)
		{
			// red
			if(strcmp(json_token_tostr(response_body, &tokens[i]), "r") == 0)
			{
				r = atoi(json_token_tostr(response_body, &tokens[i+1]));
			}

			// green
			if(strcmp(json_token_tostr(response_body, &tokens[i]), "g") == 0)
			{
				g = atoi(json_token_tostr(response_body, &tokens[i+1]));
			}
			
			// blue
			if(strcmp(json_token_tostr(response_body, &tokens[i]), "b") == 0)
			{
				b = atoi(json_token_tostr(response_body, &tokens[i+1]));
			}
			
			// delay
			if(strcmp(json_token_tostr(response_body, &tokens[i]), "delay") == 0)
			{
				delay = atoi(json_token_tostr(response_body, &tokens[i+1]));
			}
		}

		up_count();
  	set_led(r, g, b, delay);
	}
	else 
	{
  	set_led(255, 0, 0, 900);
	}
	gpio16_output_set(1);
}

void ICACHE_FLASH_ATTR send_tag( char* tag_uid )
{
	static unsigned char hash[HMAC_SHA1_DIGEST_LENGTH];
	static char tosign[200];
	static char json_send[500];
	static char hashstr[40];
	HMAC_SHA1_CTX c;

	strcpy(tosign, device_id);
	strcat(tosign, tag_uid);
	uint32_t counter = flashConfig.nfc_counter;
	char countstr[80];
	os_sprintf(countstr, "%ld", counter);
	strcat(tosign, countstr);
	
	//HMAC_SHA1_Init(&c);
	HMAC_SHA1_Init(&c);
	HMAC_SHA1_UpdateKey(&c, secret, 3);
	HMAC_SHA1_UpdateKey(&c, &(secret[3]), strlen(secret) - 3);
	HMAC_SHA1_EndKey(&c);
  HMAC_SHA1_StartMessage(&c);
	HMAC_SHA1_UpdateMessage(&c, tosign, 7);
	HMAC_SHA1_UpdateMessage(&c, &(tosign[7]), strlen(tosign) - 7);
	HMAC_SHA1_EndMessage(&(hash[0]), &c);
	HMAC_SHA1_Done(&c);

	hex2str(hashstr, hash, 20);
  //os_printf("hashstr with secret %s is %s\n\r", secret, hashstr);
	create_json(json_send, device_id, tag_uid, counter, hashstr);
	//http_get("http://wtfismyip.com/text", rfid_http_cb);
	os_printf("SENDING TAG json: %s \n\r", json_send);
	http_post(flashConfig.nfc_url, json_send, rfid_http_cb);
	
	wait_ms(700);
	gpio16_output_set(0);
}

void ICACHE_FLASH_ATTR rfid_loop()
{
	// Look for new cards
	uint8_t status = PICC_IsNewCardPresent();
	os_printf("status %d - %d - %d\r\n",status, system_get_time() / 1000000, get_time_secs());
	if ( ! (status == STATUS_OK || status == STATUS_COLLISION))
	{
		//wait_ms(500);
		//continue;
		return;
	}

	// Select one of the cards
	if ( ! PICC_ReadCardSerial())
	{
		//wait_ms(500);
		//continue;
		return;
	}

	wait_ms(100);
	
	// short little dance when got a tag
  set_led(233, 90, 99, 150);

	// Print Card UID
	os_printf("Card UID: ");
	char tag_uid [64];
	char tag_tmp [4];
	strcpy(tag_uid, "");
	for (uint8_t i = 0; i < uid.size; i++)
	{
		//os_printf("%02X", uid.uidByte[i]);
		os_sprintf(tag_tmp, "%02x", uid.uidByte[i]);
		strcat(tag_uid, tag_tmp);
	}
	os_printf("%s \n\r", tag_uid);
	// Print Card type
	uint8_t piccType = PICC_GetType(uid.sak);
	os_printf("PICC Type: %s \n\r", PICC_GetTypeName(piccType));

	// use the internal clock for this because time is synced on every request
	// the compensation for drift is often enough to throw off this short re-try period and 
	// send the tag up too often
	uint32_t time_tmp = system_get_time() / 1000000;
	if(strcmp(last_tag, tag_uid) != 0 || time_tmp - last_tag_time > LAST_TAG_TIME_THRESHOLD )
	{
		send_tag(tag_uid);
		strcpy(last_tag, tag_uid);
		last_tag_time = system_get_time() / 1000000;
		//wait_ms(500);
	}
}

//Timer event.
static void ICACHE_FLASH_ATTR rfidTimer(void *arg)
{
	//os_printf("doing rfid loop\r\n");
	rfid_loop();
}

// initialize the custom stuff that goes beyond esp-link
void app_init() {
	stdoutInit();
	get_serial_secret();

	// init status LED
	gpio16_output_conf();

	os_printf("\nReady\n");
	os_printf("\nGOING TO TRY RFID\n");
  PCD_Init();

	// purple startup
  set_led(155, 0, 155, 0);
  sleepms(100);
  set_led(0, 0, 0, 0);
  sleepms(100);
  set_led(155, 0, 155, 0);
  sleepms(100);
  set_led(0, 0, 0, 0);
  sleepms(100);
  set_led(155, 0, 155, 0);
  sleepms(100);
  set_led(0, 0, 0, 0);
  sleepms(100);
  set_led(155, 0, 155, 0);
  sleepms(100);
  set_led(0, 0, 0, 0);	

	//Add a process
	system_os_task(procTask, procTaskPrio, procTaskQueue, procTaskQueueLen);

	//RFID Timer 
	os_timer_disarm(&rfid_timer);
	os_timer_setfn(&rfid_timer, (os_timer_func_t *)rfidTimer, NULL);
	os_timer_arm(&rfid_timer, RFID_LOOP_DELAY, 1);
	
	system_os_post(procTaskPrio, 0, 0 );

	//wifi_set_sleep_type(LIGHT_SLEEP_T);

}
