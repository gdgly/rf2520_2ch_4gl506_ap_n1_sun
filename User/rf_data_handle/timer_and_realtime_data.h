#ifndef TIMER_AND_REALTIME_DATA_H_
#define TIMER_AND_REALTIME_DATA_H_


#include "stm32f4xx_hal.h"


#pragma pack(1)



//ǧ��ʵʱ���ݽṹ
typedef struct _qianfang_realtime_data
{
	uint32_t head;
	uint32_t len;
	uint8_t serial_number;
	uint32_t n1_id;
	uint8_t cmd;               // 2:ʵʱ����  3:״̬����
	uint32_t serial_number1;
	uint16_t lane_number;
	uint8_t year;
	uint8_t month;
	uint8_t day;	
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint8_t speed;
	uint8_t cartype;
	uint8_t car_len;
	uint8_t car_head_distance_sec;
	uint16_t car_car_distance;
	uint32_t car_on_time;
}struct_qianfang_realtime_data;




//ǧ��״̬����ͷ
typedef struct _qianfang_stat_data_head
{
	uint32_t head;
	uint32_t len;
	uint8_t serial_number;
	uint16_t n1_id;
	uint8_t cmd;               // 2:ʵʱ����  3:״̬����
	uint8_t year;
	uint8_t month;
	uint8_t day;	
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint8_t sensor_num;

}struct_qianfang_stat_data_head;




//sensor ���ͷ�����ͨ��Э��ͷ
typedef	struct _sensor_stat_timer_head
{
		uint32_t head;
		uint32_t ap_id;
		uint16_t len;
		uint16_t cmd; 
		uint8_t year;
		uint8_t month;
		uint8_t day;	
		uint8_t hour;
		uint8_t min;
		uint8_t sec;	
		uint8_t sensor_num;
}struct_to_server_head;


//sensor ��ʱ״̬
typedef	struct _sensor_stat_timer
{
	uint16_t sensor_id;
	int8_t rssi;
	uint16_t battay;
	uint8_t lost_bit;
	uint16_t packet_count;
	int8_t stat;
}struct_sensor_stat_timer;


//rp ��ʱ״̬
typedef	struct _rp_stat_timer
{
	uint16_t rp_id;
	int8_t rssi;
	uint16_t battay;
	uint8_t b1;
	uint16_t b2;
	int8_t stat;
}struct_rp_stat_timer;

//ap ��ʱ״̬
typedef	struct _ap_stat_timer
{
	uint32_t ap_stat;
	uint8_t a1;
	uint8_t a2;
	uint8_t a3;
	uint8_t a4;
}struct_ap_stat_timer;

//sensor ��ʱ�������ݽṹ
typedef	struct _sensor_data_timer
{
	uint16_t sensor_id;
	uint16_t car1_count;
	uint16_t car2_count;
	uint16_t car3_count;
	uint16_t car4_count;
	uint16_t car5_count;	
	uint8_t occupancy;         //ռ����
	uint16_t avg_speed;
	uint16_t max_speed;
	uint16_t min_speed;
	uint8_t avg_car_head_time_distance; //��ͷʱ�� MS
	uint16_t avg_car_car_time_distance;   //����� 	
	uint8_t avg_car_length;             //���� MS
	uint8_t b0;  //����
}struct_sensor_data_timer;

//����ʵʱ���ݽṹ
typedef struct _dezhou_realtime_data
{
	uint32_t head;
	uint32_t ap_id;
	uint16_t len;
	uint16_t cmd;
	uint32_t seq;
	uint32_t sec;
	uint8_t lane_number;
	uint8_t car_pai[10];
	uint8_t car_colour;
	uint8_t car_type;
	uint8_t car_len;
	uint8_t car_width;
	uint8_t car_high;
	uint8_t car_speed;
	uint8_t car_zhou;
	uint32_t limit_weight;
	uint32_t weight;
	uint16_t crc;
}struct_dezhou_realtime_data;






#define QUEUE_QF  1
#define QUEUE_DZ  2
#define REALTIME_QUEUE_LENGH_QF ((32*1024)/sizeof(struct_qianfang_realtime_data))   //ʵʱ����ǧ��Э���ܴ�ŵ�����
#define	REALTIME_QUEUE_LENGH_DZ ((32*1024)/sizeof(struct_dezhou_realtime_data))   //ʵʱ����ǧ��Э���ܴ�ŵ�����

#define QF_ARRAY_MAX_COUNT (1450/sizeof(struct_qianfang_realtime_data))
#define DZ_ARRAY_MAX_COUNT (1450/sizeof(struct_dezhou_realtime_data))

#define TO_4G_RE_SEND_SEC 5            //�ط����











#pragma pack()






void write_realtime_data_to_queue(uint8_t lane);

void poll_to_4g_realtime_data();

void sensor_data_and_stat_timer_task();


int32_t init_realtime_data_queue(int32_t whitch_client);


#endif









