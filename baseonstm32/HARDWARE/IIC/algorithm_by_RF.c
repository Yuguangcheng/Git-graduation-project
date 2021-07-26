#include "algorithm_by_RF.h"
#include <math.h>
#include "sys.h"
#include "usart.h"
const double sum_X2 =281237.5;// 83325; // 警告：如果您更改了上面的ST或FS，则必须重新计算此总和！

const float min_autocorrelation_ratio = 0.5;
//红色和红外信号之间的皮尔逊相关。
// 高质量信号的相关系数必须大于这个最小值。
const float min_pearson_correlation = 0.8;

/*
*派生参数

*别碰这些！
 * 
 */
const int32_t BUFFER_SIZE = FS*ST; // 单个批次中的样本数
const int32_t FS60 = FS*60;  // 心率从bps到bpm的转换因子
const int32_t LOWEST_PERIOD = FS60/MAX_HR; // 峰间最小距离
const int32_t HIGHEST_PERIOD = FS60/MIN_HR; // 峰间最大距离
const float mean_X = (float)(BUFFER_SIZE-1)/2.0; // 从0到缓冲区大小1的整数集的平均值。对于ST=4和FS=25，它等于49.5。
///
typedef enum {false = 0,true = 1} bool;
void rf_heart_rate_and_oxygen_saturation(uint32_t *pun_ir_buffer, int32_t n_ir_buffer_length, uint32_t *pun_red_buffer, float *pn_spo2, int8_t *pch_spo2_valid, 
                int32_t *pn_heart_rate, int8_t *pch_hr_valid, float *ratio, float *correl)
/**

*\brief计算心率和SpO2水平

*\par详细信息

*通过检测PPG循环峰值和相应的红/红外信号的AC/DC，计算出SPO2的xy_比。


*\param[in]*pun_ir_buffer-红外传感器数据缓冲区

*\param[in]n_ir_buffer_length-红外传感器数据缓冲区长度

*\param[in]*pun_red_buffer-红色传感器数据缓冲区

*\param[out]*pn_spo2-计算出的spo2值

*\param[out]*pch_spo2_valid-1如果计算的spo2值有效

*\param[out]*pn_heart_rate-计算的心率值

*\param[out]*pch_hr_valid-1如果计算的心率值有效


*\retval无

*/
{
  int32_t k;  
  static int32_t n_last_peak_interval=LOWEST_PERIOD;
  float f_ir_mean,f_red_mean,f_ir_sumsq,f_red_sumsq;
  float f_y_ac, f_x_ac, xy_ratio;
  float beta_ir, beta_red, x;
  float an_x[BUFFER_SIZE], *ptr_x; //ir
  float an_y[BUFFER_SIZE], *ptr_y; //red

  //计算DC平均值并从ir和red中减去DC
  f_ir_mean=0.0; 
  f_red_mean=0.0;
  for (k=0; k<n_ir_buffer_length; ++k) {
    f_ir_mean += pun_ir_buffer[k];
    f_red_mean += pun_red_buffer[k];
  }
  f_ir_mean=f_ir_mean/n_ir_buffer_length ;
  f_red_mean=f_red_mean/n_ir_buffer_length ;
  
  // 移除 DC 
  for (k=0,ptr_x=an_x,ptr_y=an_y; k<n_ir_buffer_length; ++k,++ptr_x,++ptr_y) {
    *ptr_x = pun_ir_buffer[k] - f_ir_mean;
    *ptr_y = pun_red_buffer[k] - f_red_mean;
  }

  //去基线漂移
  beta_ir = rf_linear_regression_beta(an_x, mean_X, sum_X2);
  beta_red = rf_linear_regression_beta(an_y, mean_X, sum_X2);
  for(k=0,x=-mean_X,ptr_x=an_x,ptr_y=an_y; k<n_ir_buffer_length; ++k,++x,++ptr_x,++ptr_y) {
    *ptr_x -= beta_ir*x;
    *ptr_y -= beta_red*x;
  }
  
    //计算两个sporm2的交流信号。另外，脉冲检测器需要红外光谱的原始平方和
  f_y_ac=rf_rms(an_y,n_ir_buffer_length,&f_red_sumsq);
  f_x_ac=rf_rms(an_x,n_ir_buffer_length,&f_ir_sumsq);

  // 计算red and IR的皮尔逊相关系数
  *correl=rf_Pcorrelation(an_x, an_y, n_ir_buffer_length)/sqrt(f_red_sumsq*f_ir_sumsq);

  //求信号周期
  if(*correl>=min_pearson_correlation) {
    // 在血氧饱和度测定开始时，心率的准确范围是未知的。如果需要初始化寻找心率区间
    // 寻找自相关第一个局部最大值.
    if(LOWEST_PERIOD==n_last_peak_interval) 
      rf_initialize_periodicity_search(an_x, BUFFER_SIZE, &n_last_peak_interval, HIGHEST_PERIOD, min_autocorrelation_ratio, f_ir_sumsq);
    // 若相关良好，则求出红外信号的平均周期。如果是非周期的，返回周期为0
    if(n_last_peak_interval!=0)
      rf_signal_periodicity(an_x, BUFFER_SIZE, &n_last_peak_interval, LOWEST_PERIOD, HIGHEST_PERIOD, min_autocorrelation_ratio, f_ir_sumsq, ratio);
  } else n_last_peak_interval=0;

  //如果周期性检测器成功，计算心率。否则，将峰值间隔重置为初始值并报告错误。
  if(n_last_peak_interval!=0) {
    *pn_heart_rate = (int32_t)(FS60/n_last_peak_interval);
    *pch_hr_valid  = 1;
  } else {
    n_last_peak_interval=LOWEST_PERIOD;
    *pn_heart_rate = -999; // 无法计算，因为信号看起来不周期
    *pch_hr_valid  = 0;
    *pn_spo2 =  -999 ; // 不要使用这个损坏信号的血氧饱和度
    *pch_spo2_valid  = 0; 
	  //printf("心率出错");
    return;
  }

  // 去除趋势后，平均值代表直流电平
  xy_ratio= (f_y_ac*f_ir_mean)/(f_x_ac*f_red_mean);  //公式，使用美信官方提供的 (f_y_ac*f_x_dc) / (f_x_ac*f_y_dc) ;
  if(xy_ratio>0.02 && xy_ratio<1.84) { // 检查适用范围
    *pn_spo2 = (-45.060*xy_ratio + 30.354)*xy_ratio + 94.845;
    *pch_spo2_valid = 1;
  } else {
    *pn_spo2 =  -999 ; // 不要使用血氧饱和度，因为信号比率超出范围
    *pch_spo2_valid  = 0; 
	  //printf("血氧出错");
  }
}

float rf_linear_regression_beta(float *pn_x, float xmean, float sum_x2)
/**
*\brief线性回归系数β

*\par详细信息

*计算pn_x与平均值中心的线性回归的方向系数β

*点索引值（0到缓冲区大小1）。xmean必须等于（BUFFER_SIZE-1）/2！和x2为

*平均中心索引值的平方和。

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
*\brief自相关函数
*\par详细信息
*计算给定序列pn_x的自相关序列的n_lag元素
*\retval自相关和
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
*\brief搜索真实信号周期的范围

*\par详细信息

*通过定位

*自相关函数的第一个峰。如果真的拖到

*n_max_距离自相关小于最小aut_比率分数

*滞后=0时的自相关，则输入信号不足

*周期性的，可能表示运动伪影。

*\r平均峰间距离
*/
{
  int32_t n_lag;
  float aut,aut_right;
  //此时，*p_last_periodicity=最低的_周期。开始向右走，
//一次两步，直到滞后率达到质量标准或最高周期
//已到达。
  n_lag=*p_last_periodicity;
  aut_right=aut=rf_autocorrelation(pn_x, n_size, n_lag);
  // 检查是否正常
  if(aut/aut_lag0 >= min_aut_ratio) {
   //要么是质量标准，最小aut峎u比率太低，要么是心率过高。
//我们是在自相关的下行斜率上吗？如果是，则继续使用本地最小值。
//如果没有，继续下一个街区。
    do {
      aut=aut_right;
      n_lag+=2;
      aut_right=rf_autocorrelation(pn_x, n_size, n_lag);
    } while(aut_right/aut_lag0 >= min_aut_ratio && aut_right<aut && n_lag<=n_max_distance);
    if(n_lag>n_max_distance) {
     //这不应该发生，但如果真的返回失败
      *p_last_periodicity=0;
      return;
    }
    aut=aut_right;
  }
  // 向右探寻
  do {
    aut=aut_right;
    n_lag+=2;
    aut_right=rf_autocorrelation(pn_x, n_size, n_lag);
  } while(aut_right/aut_lag0 < min_aut_ratio && n_lag<=n_max_distance);
  if(n_lag>n_max_distance) {
    // 不应该发生，但如果真的返回失败
    *p_last_periodicity=0;
  } else
    *p_last_periodicity=n_lag;
}

void rf_signal_periodicity(float *pn_x, int32_t n_size, int32_t *p_last_periodicity, int32_t n_min_distance, int32_t n_max_distance, float min_aut_ratio, float aut_lag0, float *ratio)
/**
*\短信号周期
*\par详细信息
*找出可以用来计算心率的红外信号的周期性。
*利用自相关函数。如果峰值自相关较小
*小于lag=0时自相关的最小aut峎比率分数，然后输入
*信号周期性不足，可能表示运动伪影。
*平均峰间距离
*/
{
  int32_t n_lag;
  float aut,aut_left,aut_right,aut_save;
  bool left_limit_reached=false;
  // 从最后一个周期开始计算相应的自相关
  n_lag=*p_last_periodicity;
  aut_save=aut=rf_autocorrelation(pn_x, n_size, n_lag);
  // 自相关向左一个滞后是否更大？
  aut_left=aut;
  do {
    aut=aut_left;
    n_lag--;
    aut_left=rf_autocorrelation(pn_x, n_size, n_lag);
  } while(aut_left>aut && n_lag>=n_min_distance);
  // 最高aut恢复滞后
  if(n_lag<n_min_distance) {
    left_limit_reached=true;
    n_lag=*p_last_periodicity;
    aut=aut_save;
  } else n_lag++;
  if(n_lag==*p_last_periodicity) {
    // 向左走没有进展。向右走
    aut_right=aut;
    do {
      aut=aut_right;
      n_lag++;
      aut_right=rf_autocorrelation(pn_x, n_size, n_lag);
    } while(aut_right>aut && n_lag<=n_max_distance);
    // 最高aut恢复滞后
    if(n_lag>n_max_distance) n_lag=0; // 指向失败
    else n_lag--;
    if(n_lag==*p_last_periodicity && left_limit_reached) n_lag=0; // 指向失败
  }
  *ratio=aut/aut_lag0;
  if(*ratio < min_aut_ratio) n_lag=0; // 失败
  *p_last_periodicity=n_lag;
}

float rf_rms(float *pn_x, int32_t n_size, float *sumsq) 
/**
*\简式均方根变化

*\par详细信息

*计算给定序列pn_x的均方根变化


*\r返回均方根值和原始平方和
*/
{
  int16_t i;
  float r,*pn_ptr;
  (*sumsq)=0.0;
  for (i=0,pn_ptr=pn_x; i<n_size; ++i,++pn_ptr) {
    r=(*pn_ptr);
    (*sumsq) += r*r;
  }
  (*sumsq)/=n_size; //这对应于lag=0时的自相关
  return sqrt(*sumsq);
}

float rf_Pcorrelation(float *pn_x, float *pn_y, int32_t n_size)
/**
*简要相关积
*详细信息
*计算*pn_x和*pn_y向量之间的标量积
*返回相关积
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

