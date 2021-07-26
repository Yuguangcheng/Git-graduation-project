#ifndef ALGORITHM_BY_RF_H_
#define ALGORITHM_BY_RF_H_
#include "sys.h"
/*
*可设置参数

*如果您的电路和硬件设置与默认设置匹配，请不要使用这些设置

*在这个代码中描述的。通常，不同的采样率

*和/或样本长度需要调整这些参数。
 */
 //改FS即采样率也要改max30102_Bus_Write(REG_FIFO_CONFIG,0x2f);  	//sample avg = 2, fifo rollover=false, fifo almost full = 17
//这个函数在max30102.c那
//建议不要修改，50hz采用率比较好
#define ST 3     // 采样时间以s为单位。警告：如果更改ST，则必须重新计算下面的sum_X2参数！
#define FS 50    // 采样频率（Hz）。警告：如果更改FS，则必须重新计算下面的sum_X2参数！
// 从-mean_X（见下文）到+mean_X的ST*FS数的平方和为1。例如，给定ST=4和FS=25
// 总数由100项组成: (-49.5)^2 + (-48.5)^2 + (-47.5)^2 + ... + (47.5)^2 + (48.5)^2 + (49.5)^2
// 和是对称的，所以你可以用它的正半乘以2来计算它。在这里预先计算是为了增强性能
//修改st即采样时间则需要修改sum_X2的值，这个在.c那，计算方法上面有说，不懂也可以问我，83325为采样时间st=2的情况，但建议3s最佳


#define MAX_HR 180//125  //最大心率。为消除错误信号，计算出的HR不得大于该数值。
#define MIN_HR 40   // 最小心率。为消除错误信号，计算出的HR不得低于该数值

void rf_heart_rate_and_oxygen_saturation(uint32_t *pun_ir_buffer, int32_t n_ir_buffer_length, uint32_t *pun_red_buffer, float *pn_spo2, int8_t *pch_spo2_valid, int32_t *pn_heart_rate, 
                                        int8_t *pch_hr_valid, float *ratio, float *correl);
float rf_linear_regression_beta(float *pn_x, float xmean, float sum_x2);
float rf_autocorrelation(float *pn_x, int32_t n_size, int32_t n_lag);
float rf_rms(float *pn_x, int32_t n_size, float *sumsq);
float rf_Pcorrelation(float *pn_x, float *pn_y, int32_t n_size);
void rf_signal_periodicity(float *pn_x, int32_t n_size, int32_t *p_last_periodicity, int32_t n_min_distance, int32_t n_max_distance, float min_aut_ratio, float aut_lag0, float *ratio);
void rf_initialize_periodicity_search(float *pn_x, int32_t n_size, int32_t *p_last_periodicity, int32_t n_max_distance, float min_aut_ratio, float aut_lag0);
#endif /* ALGORITHM_BY_RF_H_ */

