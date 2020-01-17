#include <math.h>
//#include <stdio.h>

#define MILLIAMPS   1     // index in the 2 dimensional LUT
#define PROG_V      0     // index in the 2 dimensional LUT

#define LOAD_RESISTOR 200.0

#define MA_NUM_ELEMENTS 72

// format of the table is [Programmed Voltage][mA output]
float mA_Cal_LUT[74][2] = {
  {0.03,4.83},
  {0.035,6.13},
  {0.04,7.02},
  {0.05,8.45},
  {0.06,9.64},
  {0.07,10.6},
  {0.08,11.41},
  {0.087,11.9},
  {0.086,11.8},
  {0.088,20.4},
  {0.09,20.8},
  {0.1,23.1},
  {0.13,29.5},
  {0.12,27.5},
  {0.14,31.47},
  {0.15,32.3},
  {0.16,32.9},
  {0.17,33.9},
  {0.181,34.6},
  {0.19,36.5},
  {0.2,37.6},
  {0.21,38.7},
  {0.221,39.0},
  {0.23,40.8},
  {0.24,41.9},
  {0.25,42.7},
  {0.26,43.9},
  {0.278,44.3},
  {0.28,45.15},
  {0.29,46.5},
  {0.3,47.1},
  {0.305,46.35},
  {0.32,47.6},
};

float Calc_mA_to_Volt(float target_mA);
int Find_Index_Upper_Bound(float target_mA);
float mW_Setting_to_mA(double target_mW);

float Calc_mA_to_Volt(float target_mA)
{
    int index_lower_bound=0, index_upper_bound=1;
    float prog_val_lower_bound, prog_val_upper_bound, mA_val_lower_bound, mA_val_upper_bound;
    float slope, mA_Offset, delta_Prog_Val, delta_mA_Val;
    float interpolated_prog_val, interpolated_mA;
    float distance_to_interpolate = 0;
    float mA_Volt_Setting = 0;

    if(target_mA == 0) return 0;
    index_upper_bound = Find_Index_Upper_Bound(target_mA);
    if(index_upper_bound == 0) // target_mA is less than one milliamp.
    {
      return 1;
        // case of minimum current
        // use minimum val in LUT and calculate slope relative to 0
    }
    else index_lower_bound = index_upper_bound-1;

    mA_val_lower_bound = mA_Cal_LUT[index_lower_bound][MILLIAMPS]; // ADC returned value
    prog_val_lower_bound = mA_Cal_LUT[index_lower_bound][PROG_V];  // actual measured value

    mA_val_upper_bound = mA_Cal_LUT[index_upper_bound][MILLIAMPS];  // higher temp
    prog_val_upper_bound= mA_Cal_LUT[index_upper_bound][PROG_V]; // lower voltage

        // find closest data point, this is point to interpolate from
    float distance_to_upper_bound = mA_val_upper_bound - target_mA;
    float distance_to_lower_bound = target_mA - mA_val_lower_bound;

    if(distance_to_upper_bound <= distance_to_lower_bound)
    {
        //printf("\t\tUPPER IS CLOSER\n");
        distance_to_interpolate = distance_to_upper_bound;

        mA_Offset = mA_Cal_LUT[index_upper_bound][PROG_V];

        delta_Prog_Val = mA_Cal_LUT[index_upper_bound][PROG_V] - mA_Cal_LUT[index_lower_bound][PROG_V];
        delta_mA_Val =  mA_Cal_LUT[index_upper_bound][MILLIAMPS] - mA_Cal_LUT[index_lower_bound][MILLIAMPS];

        slope = delta_Prog_Val / delta_mA_Val;

        mA_Volt_Setting = mA_Offset - (slope*distance_to_interpolate); // subtracting because offset is at upper bound
    }
    else
    {
        //printf("\t\tLOWER IS CLOSER\n");
        distance_to_interpolate = distance_to_lower_bound;
        mA_Offset = mA_Cal_LUT[index_lower_bound][PROG_V];

        delta_Prog_Val = mA_Cal_LUT[index_upper_bound][PROG_V] - mA_Cal_LUT[index_lower_bound][PROG_V];
        delta_mA_Val =  mA_Cal_LUT[index_upper_bound][MILLIAMPS] - mA_Cal_LUT[index_lower_bound][MILLIAMPS];

        slope = delta_Prog_Val / delta_mA_Val;
        mA_Volt_Setting = mA_Offset + (slope*distance_to_interpolate); // adding because offset is at lower bound
    }

    return mA_Volt_Setting;
}

float mW_Setting_to_mA(double target_mW)
{
  // Power = i*i*R
  // i = sqrt(Power/R)
  double target_W = target_mW/((double)1000.0);
  double target_A = 0;
  double target_mA = 0;
  double ratio = (float)( target_W/((float)200.0 ) );

  target_A = sqrt( (float)ratio );
  target_mA = target_A*1000;
    //target_mA = sqrt( ((double)9) );
  return target_mA;
}

// Returns index of bounding value in LUT for a given voltage
int Find_Index_Upper_Bound(float target_mA)
{
  int index = 0;
  bool found = false;
  float voltage_at_index;

  while(!found)
  {
      voltage_at_index = mA_Cal_LUT[index][MILLIAMPS];
      if(voltage_at_index >= target_mA)
      { return index; }
      else
      {
        index += 1;
        if( index >= MA_NUM_ELEMENTS){ return (MA_NUM_ELEMENTS -1); }
        continue;
      }
  }
}
