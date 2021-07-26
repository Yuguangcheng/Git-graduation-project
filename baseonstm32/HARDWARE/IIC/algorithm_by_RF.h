#ifndef ALGORITHM_BY_RF_H_
#define ALGORITHM_BY_RF_H_
#include "sys.h"
/*
*�����ò���

*������ĵ�·��Ӳ��������Ĭ������ƥ�䣬�벻Ҫʹ����Щ����

*����������������ġ�ͨ������ͬ�Ĳ�����

*��/������������Ҫ������Щ������
 */
 //��FS��������ҲҪ��max30102_Bus_Write(REG_FIFO_CONFIG,0x2f);  	//sample avg = 2, fifo rollover=false, fifo almost full = 17
//���������max30102.c��
//���鲻Ҫ�޸ģ�50hz�����ʱȽϺ�
#define ST 3     // ����ʱ����sΪ��λ�����棺�������ST����������¼��������sum_X2������
#define FS 50    // ����Ƶ�ʣ�Hz�������棺�������FS����������¼��������sum_X2������
// ��-mean_X�������ģ���+mean_X��ST*FS����ƽ����Ϊ1�����磬����ST=4��FS=25
// ������100�����: (-49.5)^2 + (-48.5)^2 + (-47.5)^2 + ... + (47.5)^2 + (48.5)^2 + (49.5)^2
// ���ǶԳƵģ�����������������������2����������������Ԥ�ȼ�����Ϊ����ǿ����
//�޸�st������ʱ������Ҫ�޸�sum_X2��ֵ�������.c�ǣ����㷽��������˵������Ҳ�������ң�83325Ϊ����ʱ��st=2�������������3s���


#define MAX_HR 180//125  //������ʡ�Ϊ���������źţ��������HR���ô��ڸ���ֵ��
#define MIN_HR 40   // ��С���ʡ�Ϊ���������źţ��������HR���õ��ڸ���ֵ

void rf_heart_rate_and_oxygen_saturation(uint32_t *pun_ir_buffer, int32_t n_ir_buffer_length, uint32_t *pun_red_buffer, float *pn_spo2, int8_t *pch_spo2_valid, int32_t *pn_heart_rate, 
                                        int8_t *pch_hr_valid, float *ratio, float *correl);
float rf_linear_regression_beta(float *pn_x, float xmean, float sum_x2);
float rf_autocorrelation(float *pn_x, int32_t n_size, int32_t n_lag);
float rf_rms(float *pn_x, int32_t n_size, float *sumsq);
float rf_Pcorrelation(float *pn_x, float *pn_y, int32_t n_size);
void rf_signal_periodicity(float *pn_x, int32_t n_size, int32_t *p_last_periodicity, int32_t n_min_distance, int32_t n_max_distance, float min_aut_ratio, float aut_lag0, float *ratio);
void rf_initialize_periodicity_search(float *pn_x, int32_t n_size, int32_t *p_last_periodicity, int32_t n_max_distance, float min_aut_ratio, float aut_lag0);
#endif /* ALGORITHM_BY_RF_H_ */

