#ifndef _AP_PARAM_H_
#define _AP_PARAM_H_


#include "stm32f4xx_hal.h"
#include "rf_data_handle.h"
#include "gprs_4g_app.h"


//#define AP_VERSION  0X8888                 //��Ϊ��װʹ��   �����Զ�У׼sensor
//#define AP_VERSION  0X8889               //��ӡ����syn�Ĺ�װ  ��������rssi����
#define AP_VERSION  0X9228



//0006 ����4����Ƶͨ����N1 WEB��ʾ�ߵ���bug
//0007 RF -18db
//0008 �����ж�rfͬ��������ʧ������RF rf120sû�н��յ���������RF
//0009 �������Ϊ26M
//000A ��0009����һ�� ��������ʹ��
//000c 26m ����ÿ֡��λRF
//000d 26m ���Ӵ�ӡsensor�¼��� ʹ�ܹ���
//000e 26m ����LED crc ��ʾ3·���ڽ���
//000F 26M ������3·���ڽ����ж� �������
//0010 26M RF -18DB
//0011 26M RF -4DB
//0012 26M ͨ������29 �ر�rf
//0013 26m ���������з���FFFF��־��ʱ�� �ظ�����20��
//0003 8M ��ͬ 0013
//0014 26M ����rf����С���� �����쳣�� ����Dģʽ
//00A4 26M �������¼���ʱ��� AB 47
//0015 26M RP�����ֽ�ȡ��6λ
//0016 26M ���ֻ��2·RF�����޸�
//0017 rp sensor �̼��ж���û���ظ���
//0118 �������RP ��ʧ״̬����� ����rf�����ж����������  �յ�����crc����fulsh rx
//0119 �������е�CRC_NUM ���ú�N1ͨ�ŵ�crc����
//011A ����RP������ʱ���sensor_mode�ֶ�Ҳ�޸���
//011b ���ӿ���ͬ�������͵�ָ��
//011c ����Ĭ��ͨ����Ϊ31

//0X9228   ����4Gģ���Դ�޷��رյ�bug


#pragma pack(1)



typedef struct _ap_param{

	uint32_t ap_id;                                                           //     N1ֻ��
	uint16_t ap_version;                                                      //     N1ֻ��
	uint16_t rp_version;                //eeprom ��rp�̼��汾��                       N1ֻ��
	uint16_t sensor_version;						   //eeprom ��sensor�̼��汾��                N1ֻ��	
	struct _gps_date rtc_date_time; 
	uint16_t band_id;	
	uint32_t ap_channel;
	uint8_t ap_syn_param[6];
	float gps_n_e[2];
	
}struct_ap_param;



typedef struct{
	
 uint8_t uiCmd;  //����2-�������У׼ 3-�����ģʽ����  11 -����ȫ���� 
 uint16_t uiPoll;   //Ŀ��ID
 
 uint16_t uiBindId; //��ID
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
 
}rp_param;




typedef struct{

 uint8_t ucSensorMode;
 rp_param ParaFram;
 
}struct_sensor_rp_param;



extern struct_sensor_rp_param sensor_rp_param ;



typedef	struct{
		_dot_info sensor_param1;
		_dot_info_ext sensor_param2;
		uint8_t level;
		uint8_t slot;
}s_sensor_pram;

	
typedef	struct{
	 _RP_info rp_param;
	 struct{
		uint16_t uimySlot:8,     //8λ-����ʱ���
			 uiSlotStateE:8; //8λ-����ʱ�����չ
		
	 }paraB; 
	 uint16_t uiSlotStateL;//����ʱ��۵�16
	 uint16_t uiSlotStateM;//����ʱ�����16
	 uint16_t uiSlotStateH;//����ʱ��۸�16
	 struct{  
		uint8_t uiGrade:3, //����ͬ��������0-3
			uiChannel:5;//���õ�ͨ��0-31   down
	 }paraA;		
}s_rp_pram;

typedef struct _sys_flash_param
{
	struct_ap_param ap_param;
	struct_global_cfg_param global_cfg_param;  //���ò�����
	uint8_t sensor_num;
	uint8_t rp_num;	
	s_sensor_pram sensor[64];
	s_rp_pram rp[16];
}struct_sys_flash_param;






typedef struct _sensor_rp_updata_stat
{
	uint16_t sensor_id;
	uint16_t version;
	uint16_t timeout_sec;
	int16_t updata_seq;		
}struct__sensor_updata_stat;








#pragma pack()



extern struct_sys_flash_param sys_flash_param ;

void init_ap_param(void);



#endif


