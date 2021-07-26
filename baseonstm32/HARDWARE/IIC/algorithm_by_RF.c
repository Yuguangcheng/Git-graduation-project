#include "algorithm_by_RF.h"
#include <math.h>
#include "sys.h"
#include "usart.h"
const double sum_X2 =281237.5;// 83325; // ���棺����������������ST��FS����������¼�����ܺͣ�

const float min_autocorrelation_ratio = 0.5;
//��ɫ�ͺ����ź�֮���Ƥ��ѷ��ء�
// �������źŵ����ϵ��������������Сֵ��
const float min_pearson_correlation = 0.8;

/*
*��������

*������Щ��
 * 
 */
const int32_t BUFFER_SIZE = FS*ST; // ���������е�������
const int32_t FS60 = FS*60;  // ���ʴ�bps��bpm��ת������
const int32_t LOWEST_PERIOD = FS60/MAX_HR; // �����С����
const int32_t HIGHEST_PERIOD = FS60/MIN_HR; // ���������
const float mean_X = (float)(BUFFER_SIZE-1)/2.0; // ��0����������С1����������ƽ��ֵ������ST=4��FS=25��������49.5��
///
typedef enum {false = 0,true = 1} bool;
void rf_heart_rate_and_oxygen_saturation(uint32_t *pun_ir_buffer, int32_t n_ir_buffer_length, uint32_t *pun_red_buffer, float *pn_spo2, int8_t *pch_spo2_valid, 
                int32_t *pn_heart_rate, int8_t *pch_hr_valid, float *ratio, float *correl)
/**

*\brief�������ʺ�SpO2ˮƽ

*\par��ϸ��Ϣ

*ͨ�����PPGѭ����ֵ����Ӧ�ĺ�/�����źŵ�AC/DC�������SPO2��xy_�ȡ�


*\param[in]*pun_ir_buffer-���⴫�������ݻ�����

*\param[in]n_ir_buffer_length-���⴫�������ݻ���������

*\param[in]*pun_red_buffer-��ɫ���������ݻ�����

*\param[out]*pn_spo2-�������spo2ֵ

*\param[out]*pch_spo2_valid-1��������spo2ֵ��Ч

*\param[out]*pn_heart_rate-���������ֵ

*\param[out]*pch_hr_valid-1������������ֵ��Ч


*\retval��

*/
{
  int32_t k;  
  static int32_t n_last_peak_interval=LOWEST_PERIOD;
  float f_ir_mean,f_red_mean,f_ir_sumsq,f_red_sumsq;
  float f_y_ac, f_x_ac, xy_ratio;
  float beta_ir, beta_red, x;
  float an_x[BUFFER_SIZE], *ptr_x; //ir
  float an_y[BUFFER_SIZE], *ptr_y; //red

  //����DCƽ��ֵ����ir��red�м�ȥDC
  f_ir_mean=0.0; 
  f_red_mean=0.0;
  for (k=0; k<n_ir_buffer_length; ++k) {
    f_ir_mean += pun_ir_buffer[k];
    f_red_mean += pun_red_buffer[k];
  }
  f_ir_mean=f_ir_mean/n_ir_buffer_length ;
  f_red_mean=f_red_mean/n_ir_buffer_length ;
  
  // �Ƴ� DC 
  for (k=0,ptr_x=an_x,ptr_y=an_y; k<n_ir_buffer_length; ++k,++ptr_x,++ptr_y) {
    *ptr_x = pun_ir_buffer[k] - f_ir_mean;
    *ptr_y = pun_red_buffer[k] - f_red_mean;
  }

  //ȥ����Ư��
  beta_ir = rf_linear_regression_beta(an_x, mean_X, sum_X2);
  beta_red = rf_linear_regression_beta(an_y, mean_X, sum_X2);
  for(k=0,x=-mean_X,ptr_x=an_x,ptr_y=an_y; k<n_ir_buffer_length; ++k,++x,++ptr_x,++ptr_y) {
    *ptr_x -= beta_ir*x;
    *ptr_y -= beta_red*x;
  }
  
    //��������sporm2�Ľ����źš����⣬����������Ҫ������׵�ԭʼƽ����
  f_y_ac=rf_rms(an_y,n_ir_buffer_length,&f_red_sumsq);
  f_x_ac=rf_rms(an_x,n_ir_buffer_length,&f_ir_sumsq);

  // ����red and IR��Ƥ��ѷ���ϵ��
  *correl=rf_Pcorrelation(an_x, an_y, n_ir_buffer_length)/sqrt(f_red_sumsq*f_ir_sumsq);

  //���ź�����
  if(*correl>=min_pearson_correlation) {
    // ��Ѫ�����ͶȲⶨ��ʼʱ�����ʵ�׼ȷ��Χ��δ֪�ġ������Ҫ��ʼ��Ѱ����������
    // Ѱ������ص�һ���ֲ����ֵ.
    if(LOWEST_PERIOD==n_last_peak_interval) 
      rf_initialize_periodicity_search(an_x, BUFFER_SIZE, &n_last_peak_interval, HIGHEST_PERIOD, min_autocorrelation_ratio, f_ir_sumsq);
    // ��������ã�����������źŵ�ƽ�����ڡ�����Ƿ����ڵģ���������Ϊ0
    if(n_last_peak_interval!=0)
      rf_signal_periodicity(an_x, BUFFER_SIZE, &n_last_peak_interval, LOWEST_PERIOD, HIGHEST_PERIOD, min_autocorrelation_ratio, f_ir_sumsq, ratio);
  } else n_last_peak_interval=0;

  //��������Լ�����ɹ����������ʡ����򣬽���ֵ�������Ϊ��ʼֵ���������
  if(n_last_peak_interval!=0) {
    *pn_heart_rate = (int32_t)(FS60/n_last_peak_interval);
    *pch_hr_valid  = 1;
  } else {
    n_last_peak_interval=LOWEST_PERIOD;
    *pn_heart_rate = -999; // �޷����㣬��Ϊ�źſ�����������
    *pch_hr_valid  = 0;
    *pn_spo2 =  -999 ; // ��Ҫʹ��������źŵ�Ѫ�����Ͷ�
    *pch_spo2_valid  = 0; 
	  //printf("���ʳ���");
    return;
  }

  // ȥ�����ƺ�ƽ��ֵ����ֱ����ƽ
  xy_ratio= (f_y_ac*f_ir_mean)/(f_x_ac*f_red_mean);  //��ʽ��ʹ�����Źٷ��ṩ�� (f_y_ac*f_x_dc) / (f_x_ac*f_y_dc) ;
  if(xy_ratio>0.02 && xy_ratio<1.84) { // ������÷�Χ
    *pn_spo2 = (-45.060*xy_ratio + 30.354)*xy_ratio + 94.845;
    *pch_spo2_valid = 1;
  } else {
    *pn_spo2 =  -999 ; // ��Ҫʹ��Ѫ�����Ͷȣ���Ϊ�źű��ʳ�����Χ
    *pch_spo2_valid  = 0; 
	  //printf("Ѫ������");
  }
}

float rf_linear_regression_beta(float *pn_x, float xmean, float sum_x2)
/**
*\brief���Իع�ϵ����

*\par��ϸ��Ϣ

*����pn_x��ƽ��ֵ���ĵ����Իع�ķ���ϵ����

*������ֵ��0����������С1����xmean������ڣ�BUFFER_SIZE-1��/2����x2Ϊ

*ƽ����������ֵ��ƽ���͡�

*/
{
  float x,beta,*pn_ptr;
  beta=0.0;
  for(x=-xmean,pn_ptr=pn_x;x<=xmean;++x,++pn_ptr)
    beta+=x*(*pn_ptr);
  return beta/sum_x2;
}

float rf_autocorrelation(float *pn_x, int32_t n_size, int32_t n_lag) 
/**
*\brief����غ���
*\par��ϸ��Ϣ
*�����������pn_x����������е�n_lagԪ��
*\retval����غ�
*/
{
  int16_t i, n_temp=n_size-n_lag;
  float sum=0.0,*pn_ptr;
  if(n_temp<=0) return sum;
  for (i=0,pn_ptr=pn_x; i<n_temp; ++i,++pn_ptr) {
    sum += (*pn_ptr)*(*(pn_ptr+n_lag));
  }
  return sum/n_temp;
}

void rf_initialize_periodicity_search(float *pn_x, int32_t n_size, int32_t *p_last_periodicity, int32_t n_max_distance, float min_aut_ratio, float aut_lag0)
/**
*\brief������ʵ�ź����ڵķ�Χ

*\par��ϸ��Ϣ

*ͨ����λ

*����غ����ĵ�һ���塣�������ϵ�

*n_max_���������С����Сaut_���ʷ���

*�ͺ�=0ʱ������أ��������źŲ���

*�����Եģ����ܱ�ʾ�˶�αӰ��

*\rƽ��������
*/
{
  int32_t n_lag;
  float aut,aut_right;
  //��ʱ��*p_last_periodicity=��͵�_���ڡ���ʼ�����ߣ�
//һ��������ֱ���ͺ��ʴﵽ������׼���������
//�ѵ��
  n_lag=*p_last_periodicity;
  aut_right=aut=rf_autocorrelation(pn_x, n_size, n_lag);
  // ����Ƿ�����
  if(aut/aut_lag0 >= min_aut_ratio) {
   //Ҫô��������׼����Сaut�Wu����̫�ͣ�Ҫô�����ʹ��ߡ�
//������������ص�����б����������ǣ������ʹ�ñ�����Сֵ��
//���û�У�������һ��������
    do {
      aut=aut_right;
      n_lag+=2;
      aut_right=rf_autocorrelation(pn_x, n_size, n_lag);
    } while(aut_right/aut_lag0 >= min_aut_ratio && aut_right<aut && n_lag<=n_max_distance);
    if(n_lag>n_max_distance) {
     //�ⲻӦ�÷������������ķ���ʧ��
      *p_last_periodicity=0;
      return;
    }
    aut=aut_right;
  }
  // ����̽Ѱ
  do {
    aut=aut_right;
    n_lag+=2;
    aut_right=rf_autocorrelation(pn_x, n_size, n_lag);
  } while(aut_right/aut_lag0 < min_aut_ratio && n_lag<=n_max_distance);
  if(n_lag>n_max_distance) {
    // ��Ӧ�÷������������ķ���ʧ��
    *p_last_periodicity=0;
  } else
    *p_last_periodicity=n_lag;
}

void rf_signal_periodicity(float *pn_x, int32_t n_size, int32_t *p_last_periodicity, int32_t n_min_distance, int32_t n_max_distance, float min_aut_ratio, float aut_lag0, float *ratio)
/**
*\���ź�����
*\par��ϸ��Ϣ
*�ҳ����������������ʵĺ����źŵ������ԡ�
*��������غ����������ֵ����ؽ�С
*С��lag=0ʱ����ص���Сaut�W���ʷ�����Ȼ������
*�ź������Բ��㣬���ܱ�ʾ�˶�αӰ��
*ƽ��������
*/
{
  int32_t n_lag;
  float aut,aut_left,aut_right,aut_save;
  bool left_limit_reached=false;
  // �����һ�����ڿ�ʼ������Ӧ�������
  n_lag=*p_last_periodicity;
  aut_save=aut=rf_autocorrelation(pn_x, n_size, n_lag);
  // ���������һ���ͺ��Ƿ����
  aut_left=aut;
  do {
    aut=aut_left;
    n_lag--;
    aut_left=rf_autocorrelation(pn_x, n_size, n_lag);
  } while(aut_left>aut && n_lag>=n_min_distance);
  // ���aut�ָ��ͺ�
  if(n_lag<n_min_distance) {
    left_limit_reached=true;
    n_lag=*p_last_periodicity;
    aut=aut_save;
  } else n_lag++;
  if(n_lag==*p_last_periodicity) {
    // ������û�н�չ��������
    aut_right=aut;
    do {
      aut=aut_right;
      n_lag++;
      aut_right=rf_autocorrelation(pn_x, n_size, n_lag);
    } while(aut_right>aut && n_lag<=n_max_distance);
    // ���aut�ָ��ͺ�
    if(n_lag>n_max_distance) n_lag=0; // ָ��ʧ��
    else n_lag--;
    if(n_lag==*p_last_periodicity && left_limit_reached) n_lag=0; // ָ��ʧ��
  }
  *ratio=aut/aut_lag0;
  if(*ratio < min_aut_ratio) n_lag=0; // ʧ��
  *p_last_periodicity=n_lag;
}

float rf_rms(float *pn_x, int32_t n_size, float *sumsq) 
/**
*\��ʽ�������仯

*\par��ϸ��Ϣ

*�����������pn_x�ľ������仯


*\r���ؾ�����ֵ��ԭʼƽ����
*/
{
  int16_t i;
  float r,*pn_ptr;
  (*sumsq)=0.0;
  for (i=0,pn_ptr=pn_x; i<n_size; ++i,++pn_ptr) {
    r=(*pn_ptr);
    (*sumsq) += r*r;
  }
  (*sumsq)/=n_size; //���Ӧ��lag=0ʱ�������
  return sqrt(*sumsq);
}

float rf_Pcorrelation(float *pn_x, float *pn_y, int32_t n_size)
/**
*��Ҫ��ػ�
*��ϸ��Ϣ
*����*pn_x��*pn_y����֮��ı�����
*������ػ�
*/
{
  int16_t i;
  float r,*x_ptr,*y_ptr;
  r=0.0;
  for (i=0,x_ptr=pn_x,y_ptr=pn_y; i<n_size; ++i,++x_ptr,++y_ptr) {
    r+=(*x_ptr)*(*y_ptr);
  }
  r/=n_size;
  return r;
}

