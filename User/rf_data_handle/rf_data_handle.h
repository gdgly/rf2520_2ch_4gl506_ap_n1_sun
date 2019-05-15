#ifndef RF_DATA_HANDLE_H_
#define RF_DATA_HANDLE_H_

#include "stm32f4xx_hal.h"
#include "snp.h"



#define LANE_SENSOR_MAX_COUNT 64         //�г�����sensor�������
#define NOLANE_SENSOR_MAX_COUNT 64      //�޳�����sensor�������
#define RP_MAX_COUNT 24                  //RP�б�
#define EVENT_MAX 10                    //�б�����󻺴���¼�����

#define BEFORE 1
#define AFTER 2

#define CAR_STAT_ON 1
#define CAR_STAT_OFF 0

#define SENSOR_EVENT_PACKET_HEAD_SIZE 10    //�¼��������¼��ĳ��ȣ����������¼�����

#define ROW_MAX 64
#define LINE_MAX 200


#pragma pack(1)


typedef struct 
{
	unsigned short m_usDotId;		/*�����ID*/
	unsigned char  m_ucLaneId;		/*������*/
	unsigned char  m_ucPosition;	/*λ��ǰ��0-����1*/
	unsigned short m_usDistance;	/*�������*/
	unsigned char  m_ucSectionId;	/*0 ·�� 1·��*/
	unsigned char  m_ucCcexChannel; /*ccex*/
	
}_dot_info;

typedef struct
{
	unsigned char  m_ucLaneType;  /*�������� 1��ת 2ֱ��3��ת4��ͷ�����ɷ���*/
	unsigned char  m_ucDirection;  /*����*/
	unsigned char  m_ucChannel;   /* RFͨ�� */
	unsigned short m_ucpid;      /*��ID */
	unsigned char  m_ucmode;     /*����ģʽ*/
	unsigned int   m_innum;		/*�������*/
	unsigned char  m_vLaneID;	/*���⳵���� ��Ӧweb�·��ĳ�����*/
	
}_dot_info_ext;

typedef struct
{
	unsigned short m_usRpId;              /*Rp id*/
	unsigned char  m_ucDirection;         /*����*/
	unsigned char  m_ucChannel;           /*RFͨ��   up*/  
	unsigned short m_ucpid;				  /*��ID*/
	unsigned int   m_innum;					/*�������*/
	
}_RP_info;


typedef struct _global_cfg_param
{
	uint16_t data_save_timer_time;       //��ʱ���ݶ�ʱʱ��   ��
	uint16_t m_usCarLimit;								/*�ֳ���ֵ ms*/
	uint16_t m_usOndelay;									/*ON delay*/
	uint16_t m_usDelayTime;                               /*��ʱʱ��*/
	uint16_t m_usMinThroughTime;							/*��Сͨ��ʱ��ms*/
	uint16_t m_arDelimiter[4];							/*�ֳ��� mm*/	
	uint8_t dev_reset_switch;             //ϵͳ��ʱ��λ����
	uint8_t dev_reset_hour;
	uint8_t dev_reset_min;          //��ʱ��λʱ���
	uint32_t server_ip[2];          //��˾������ip
	uint16_t server_port[2];        //��˾�������˿�
	uint8_t realtime_data_type[2];          //ǧ�� �� ����
	#define RDT_QF  1
	#define RDT_DZ  2
	uint8_t realtime_data_switch[2];        //ʵʱ���ݿ���
	uint8_t timer_data_switch[2];          //��ʱ���ݿ���
	uint8_t timer_stat_switch[2];          //��ʱ״̬����
}struct_global_cfg_param;

#pragma pack()






typedef struct _sensor_cfg
{

	uint16_t sensor_before_to_after_distance;        //ǰ���ü��������
	uint8_t road_end_or_in;      //·�ڻ���·��
		#define ROAD_END 0
		#define ROAD_IN 1
	uint8_t ccex;
	uint8_t lane_type;          //��������
		#define LANE_LEFT 1        //��ת
		#define LANE_DIRECT 2      //ֱ��
		#define LANE_RIGHT 4       //��ת
		#define LANE_TURN_ROUND 8   //��ͷ
	uint8_t lane_direction;    //��������
	uint8_t rf_ch;
	uint16_t up_rp_id;
	uint8_t work_mode;
	uint32_t   m_innum;		/*�������*/
	uint8_t  m_vLaneID;	/*���⳵���� ��Ӧweb�·��ĳ�����*/	
	uint8_t slot;
	uint8_t level;
}struct_sensor_cfg;


typedef struct _rp_cfg
{
	uint8_t lane_direction;    //��������
	uint8_t rf_ch;
	uint16_t up_rp_id;
	uint32_t  m_innum;					/*�������*/	
	struct{
	uint16_t uimySlot:8,     //8λ-����ʱ���
		 uiSlotStateE:8; //8λ-����ʱ�����չ

	}paraB; 
	uint16_t uiSlotStateL;//����ʱ��۵�16
	uint16_t uiSlotStateM;//����ʱ�����16
	uint16_t uiSlotStateH;//����ʱ��۸�16
	struct{  
	uint8_t uiGrade:3, //����ͬ��������0-3
		uiChannel:5;//���õ�ͨ��0-31
	}paraA;	
}struct_rp_cfg;





typedef	struct _event_and_info{
		SNP_EVENT_t event;
		uint32_t event_rev_time_slot;   //�¼��յ���ʱ�� ϵͳʱ��ۼ���
		uint8_t event_valid;    //�¼�����Ч��
			#define EVENT_HANDLE_ZERO 0      //δ�����
			#define EVENT_HANDLE_ONE 1      //�����һ�� ��Ҫ������һ���¼������
			#define EVENT_HANDLE_END 2      //���ս��
}struct_event_and_info;

typedef struct _sensor_event
{
	struct_event_and_info event[10];
	
	volatile uint8_t event_count;          //����sensor event����
	volatile uint8_t last_event_packet_seq;   //��ǰ�¼������	
	volatile int8_t has_event_no_handle;     //ָʾ����δ�����sensor�¼�
	uint8_t last_packet_event_count;   //��һ�����¼�����
	uint8_t car_zhou;   //sensor �ϱ��ĳ�����
	struct{   //������Ҫʹ�õĹ�������
		SNP_EVENT_t last_pcket_event[16];   //��һ�����¼�                  
		int8_t now_on_off;                 //��ǰ��ON OFF״̬
		uint16_t off_to_on_timeslot; //off-on
		uint16_t on_to_off_timeslot; //on-off
		struct_event_and_info on_event_info;   //�������㳵ͷʱ��
		SNP_EVENT_t car_onoff_event[2];  //һ������on off �¼���¼
	};
	struct{       //ʵʱ����
		uint16_t speed;                 //����  mm/S
		uint16_t car_length;             //���� MS
		uint32_t car_count;              //��������
		uint16_t car_head_time_distance; //��ͷʱ�� MS
		uint16_t car_car_time_distance;   //����� 
	};
	struct{
		uint32_t car_len_count[6];  //5�ֳ�������ͳ�� 5��ʾ����
		uint32_t occupancy;         //ռ����
		uint32_t avg_speed;
		uint32_t max_speed;
		uint32_t min_speed;
		uint32_t avg_car_length;             //���� MS
		uint32_t avg_car_head_time_distance; //��ͷʱ�� MS
		uint32_t avg_car_car_time_distance;   //����� 		
		}t[2];  // 0:��ʱ����   1��ͳ������
	
	uint8_t last_resend_times;		
	uint16_t resend[10]; //�ش�ͳ��
	uint32_t all_rev_event_count;
	uint32_t all_lost_event_count;	
}struct_sensor_event;

typedef struct _sensor_stat
{
	int8_t new_7fff_flag;
	uint32_t timeslot_7fff_event;
	int8_t rssi;
	int8_t luzl;
	uint8_t slot;
	int8_t avg_rssi1;
	int32_t avg_rssi;
	int32_t avg_volt;
	uint8_t lost_rate;
	uint16_t packet_count;  //�ܰ��� ���ڱ��� 
	uint32_t timer_lost_packet_num;   //��ʱʱ���ڶ��˵ı���
	uint32_t timer_packet_num;	   //��ʱʱ���ڰ���
	uint16_t t_resend[10];  //��ʱʱ�����ش�ͳ��
	SNP_STATE_PACKET_SENSOR_t sensor_stat_packet;
	
}struct_sensor_stat;

typedef struct _rp_stat
{
	int8_t new_7fff_flag;
	uint32_t timeslot_7fff_event;
	int8_t rssi;
	int8_t luzl;
	uint8_t slot;
	int32_t avg_rssi;
	int32_t avg_volt;
	uint32_t timer_packet_num;    //��ʱʱ���ڰ���
	uint32_t event_rev_time_slot;   //�¼��յ���ʱ�� ϵͳʱ��ۼ���
	SNP_STATE_PACKET_RP_t rp_stat_packet;
	
}struct_rp_stat;


typedef struct _lane_and_sensor{
			uint16_t sensor_id;
			uint32_t last_packet_time_slot;
			uint16_t set_param_times;
			uint8_t updata_enable;
			struct_sensor_cfg sensor_cfg;          //sensor������Ϣ
			struct_sensor_event sensor_event;
			struct_sensor_stat sensor_stat;
}struct_lane_and_sensor;


/*sensor ������Ϣ �¼� �Լ�����������*/
typedef struct _lane_to_sensor_info_and_result
{
	uint8_t has_lane_sensor_num;   //���õ�sensor����
	uint8_t no_lane_sensor_num;    //û�����õĸ���
	uint8_t cfg_rp_num;           //���õ�rp����
	uint8_t rp_num;        //�����ĸ���
	
	struct{
		struct_lane_and_sensor before;                                 //ǰ��sensor
		struct_lane_and_sensor after;		                              //����sensor
	}lane_and_sensor[LANE_SENSOR_MAX_COUNT];                        //�����õ�sensor
	
	struct{
		uint16_t rp_id;
		uint32_t last_packet_time_slot;
		uint16_t set_param_times;
		uint8_t updata_enable;
		struct_rp_cfg rp_cfg;
		struct_rp_stat rp_stat;
	}rp_cfg_and_stat[RP_MAX_COUNT];      //rp
	
	
	struct{
		uint16_t sensor_id;
		uint32_t last_packet_time_slot;
		struct_sensor_event sensor_event;
		struct_sensor_stat sensor_stat;
	}no_lane_sensor[NOLANE_SENSOR_MAX_COUNT];                  //û�����õ�sensor
	
}struct_lane_to_sensor_info_and_result;





extern int32_t print_event_handle_guocheng ;     //��ӡON OFF������ϸ����

extern int32_t print_one_car;   //��ӡʵʱ�������

int insert_sensor_event(SNP_SEN_MODE_B_PACKET_t *ptr_s_event,int8_t rssi,uint8_t luzl,uint8_t slot);
void sensor_event_and_stat_hanle();
void insert_sensor_stat_packet(SNP_STATE_PACKET_SENSOR_t *pstat,int8_t rssi,uint8_t luzl,uint8_t slot);
int32_t insert_rp_stat_packet(SNP_STATE_PACKET_RP_t *pstat,int8_t rssi,uint8_t luzl,uint8_t slot);
void add_sensor_cfg(uint8_t lane,uint16_t sid,uint8_t before_or_after);
void make_timer_statistics_data(uint32_t timer_time_ms);

extern struct_lane_to_sensor_info_and_result lane_to_sensor_info_and_result;


#endif


